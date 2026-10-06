#include "app.h"

#include "app_state.h"
#include "radio/radio.h"
#include "radio/wifi_scanner.h"
#include "radio/scheduler.h"
#include "storage/history_store.h"
#include "demo_radio.h"
#if !CONFIG_RADIO_EXPLORER_SIMULATOR
#include "radio/ble_scanner.h"
#endif
#include "ui/ui_app.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"
#if CONFIG_RADIO_EXPLORER_SIMULATOR
#include "radio/mock_scanner.h"
#endif

static const char *TAG = "radio_explorer";
enum { RE_NEARBY_SELECTION_FREEZE_OBSERVATIONS = 180 };
enum { RADIO_BACKEND_EVENT_QUEUE_DEPTH = 32 };
_Static_assert(sizeof(radio_backend_event_t) * RADIO_BACKEND_EVENT_QUEUE_DEPTH <= 6u * 1024u,
               "Radio backend event queue exceeds the 6 KiB budget");
#if !CONFIG_RADIO_EXPLORER_SIMULATOR
_Static_assert(sizeof(radio_backend_event_t) * RADIO_BACKEND_EVENT_QUEUE_DEPTH +
               RADIO_BLE_REPORT_QUEUE_DEPTH *
                   (6u + 1u + 1u + 1u + RADIO_BLE_AD_SNAPSHOT_MAX + 3u) <= 6u * 1024u,
               "Combined backend event queues exceed the 6 KiB budget");
#endif
static re_app_state_t s_state;
#if CONFIG_RADIO_EXPLORER_SIMULATOR
static radio_backend_t s_backend;
#endif
static QueueHandle_t s_backend_events;
static bool s_wifi_started;
static bool s_wifi_failed;
#if !CONFIG_RADIO_EXPLORER_SIMULATOR
static bool s_ble_started;
#endif
#if CONFIG_RADIO_EXPLORER_SIMULATOR
static mock_scanner_t s_mock;
static bool s_mock_started;
#endif
static lv_timer_t *s_tick_timer;
static int s_battery = -1;
static radio_scheduler_t s_scheduler;
static uint32_t s_last_memory_log_ms;
static uint32_t s_last_history_save_ms;

static void log_memory_metrics(const char *task_name, uint32_t task_stack_free_bytes)
{
    ESP_LOGI(TAG, "memory task=%s free=%u min=%u largest=%u stack_free=%u",
             task_name,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
             (unsigned)task_stack_free_bytes);
}

#if CONFIG_RADIO_EXPLORER_SIMULATOR
static mock_scanner_scenario_t configured_mock_scenario(void)
{
#if CONFIG_RADIO_EXPLORER_MOCK_SCENARIO_EMPTY
    return MOCK_SCANNER_EMPTY;
#elif CONFIG_RADIO_EXPLORER_MOCK_SCENARIO_FAILURE
    return MOCK_SCANNER_FAILURE;
#elif CONFIG_RADIO_EXPLORER_MOCK_SCENARIO_OVERFLOW
    return MOCK_SCANNER_OVERFLOW;
#else
    return MOCK_SCANNER_NORMAL;
#endif
}

static void configure_mock_backend(void)
{
    mock_scanner_init(&s_mock, configured_mock_scenario());
    s_backend = (radio_backend_t){ .start = mock_scanner_start, .stop = mock_scanner_stop,
                                   .tick = mock_scanner_tick, .context = &s_mock };
}
#endif

static bool backend_event(void *context, const radio_backend_event_t *event)
{
    (void)context;
    return s_backend_events != NULL && event != NULL &&
        xQueueSend(s_backend_events, event, 0) == pdTRUE;
}

static void apply_backend_event(const radio_backend_event_t *event)
{
    if (event == NULL) return;
    if (event->kind == RADIO_BACKEND_EVENT_OBSERVATION) {
        re_app_state_observe(&s_state, &event->observation, 80, 240);
    } else if (event->kind == RADIO_BACKEND_EVENT_ERROR) {
        ESP_LOGE(TAG, "Radio backend failure: %d", event->error);
        re_app_state_set_backend(&s_state, RE_BACKEND_FAILED);
    } else if (event->kind == RADIO_BACKEND_EVENT_STATE) {
        switch (event->state) {
        case RADIO_BACKEND_STOPPED: re_app_state_set_backend(&s_state, RE_BACKEND_STOPPED); break;
        case RADIO_BACKEND_STARTING: re_app_state_set_backend(&s_state, RE_BACKEND_STARTING); break;
        case RADIO_BACKEND_SCANNING: re_app_state_set_backend(&s_state, RE_BACKEND_SCANNING); break;
        case RADIO_BACKEND_IDLE: re_app_state_set_backend(&s_state, RE_BACKEND_IDLE); break;
        case RADIO_BACKEND_FAILED: re_app_state_set_backend(&s_state, RE_BACKEND_FAILED); break;
        case RADIO_BACKEND_UNAVAILABLE: re_app_state_set_backend(&s_state, RE_BACKEND_UNAVAILABLE); break;
        }
    }
}

static void reconcile_scanners(radio_schedule_phase_t phase)
{
    const bool want_wifi = phase == RADIO_SCHEDULE_WIFI;
    if (want_wifi && !s_wifi_started) {
        s_wifi_started = radio_wifi_scanner_start(backend_event, NULL);
        s_wifi_failed = !s_wifi_started;
    } else if (!want_wifi && s_wifi_started) {
        radio_wifi_scanner_stop();
        s_wifi_started = false;
        s_wifi_failed = false;
    }

#if CONFIG_RADIO_EXPLORER_SIMULATOR
    const bool want_mock = phase == RADIO_SCHEDULE_BLE;
    if (want_mock && !s_mock_started) {
        configure_mock_backend();
        s_mock_started = s_backend.start(s_backend.context, backend_event, NULL);
    } else if (!want_mock && s_mock_started) {
        s_backend.stop(s_backend.context);
        s_mock_started = false;
    }
#endif

#if !CONFIG_RADIO_EXPLORER_SIMULATOR
    const bool want_ble = phase == RADIO_SCHEDULE_BLE;
    if (want_ble && !s_ble_started) {
        s_ble_started = radio_ble_scanner_start(backend_event, NULL);
    } else if (!want_ble && s_ble_started) {
        const esp_err_t stopped = radio_ble_scanner_stop();
        if (stopped == ESP_OK) s_ble_started = false;
    }
#endif

}

static void update_backend_state(re_mode_t mode, bool scan_paused,
                                 radio_schedule_phase_t phase)
{
    if (scan_paused) re_app_state_set_backend(&s_state, RE_BACKEND_IDLE);
#if CONFIG_RADIO_EXPLORER_SIMULATOR
    else if ((phase == RADIO_SCHEDULE_WIFI && s_wifi_started) ||
             (phase == RADIO_SCHEDULE_BLE && s_mock_started))
        re_app_state_set_backend(&s_state, RE_BACKEND_SCANNING);
#else
    else if (phase == RADIO_SCHEDULE_WIFI && s_wifi_failed)
        re_app_state_set_backend(&s_state, RE_BACKEND_FAILED);
    else if (phase == RADIO_SCHEDULE_BLE && radio_ble_scanner_failed())
        re_app_state_set_backend(&s_state, RE_BACKEND_FAILED);
    else if ((phase == RADIO_SCHEDULE_WIFI && s_wifi_started) ||
             (phase == RADIO_SCHEDULE_BLE && radio_ble_scanner_scanning()))
        re_app_state_set_backend(&s_state, RE_BACKEND_SCANNING);
    else if (phase == RADIO_SCHEDULE_BLE && s_ble_started)
        re_app_state_set_backend(&s_state, RE_BACKEND_STARTING);
    else if (mode == RE_MODE_BLE) re_app_state_set_backend(&s_state, RE_BACKEND_UNAVAILABLE);
#endif
    else if (s_wifi_failed) re_app_state_set_backend(&s_state, RE_BACKEND_FAILED);
    else re_app_state_set_backend(&s_state, RE_BACKEND_STOPPED);
}

static void scanner_worker(void *arg)
{
    (void)arg;
    for (;;) {
        re_mode_t mode;
        bool paused;
        uint32_t now_seq;
        radio_schedule_phase_t phase;
        if (!bsp_lvgl_lock(250)) {
            vTaskDelay(pdMS_TO_TICKS(125));
            continue;
        }
        mode = s_state.mode;
        paused = s_state.scan_paused;
        now_seq = s_state.sequence + 1u;
        bsp_lvgl_unlock();

        const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        phase = radio_scheduler_step(&s_scheduler, mode, paused, now_ms);
        reconcile_scanners(phase);
#if CONFIG_RADIO_EXPLORER_SIMULATOR
        if (phase == RADIO_SCHEDULE_BLE && s_mock_started && s_backend.tick != NULL)
            s_backend.tick(s_backend.context, now_seq);
#endif
        if (phase == RADIO_SCHEDULE_WIFI && s_wifi_started) radio_wifi_scanner_tick(now_seq);
#if !CONFIG_RADIO_EXPLORER_SIMULATOR
        if (phase == RADIO_SCHEDULE_BLE && s_ble_started) radio_ble_scanner_tick(now_seq);
#endif
        s_wifi_failed = radio_wifi_scanner_failed();

        if (bsp_lvgl_lock(250)) {
            update_backend_state(mode, paused, phase);
            const bool should_save = s_state.history_dirty &&
                (s_state.history_flush_now ||
                 (uint32_t)(now_ms - s_last_history_save_ms) >= 60000u);
            const bool save_immediate = s_state.history_flush_now;
            const size_t save_count = s_state.history_count;
            const uint32_t save_session = s_state.session_id;
            esp_err_t history_error = ESP_OK;
            uint32_t next_generation = s_state.history_generation;
            if (should_save) {
                next_generation++;
                history_error = radio_history_store_prepare(s_state.history,
                    s_state.history_count, s_state.session_id, next_generation);
                if (history_error == ESP_OK) {
                    s_state.history_generation = next_generation;
                    s_state.history_dirty = false;
                    s_state.history_flush_now = false;
                }
            }
            bsp_lvgl_unlock();
            if (should_save) {
                if (history_error == ESP_OK) {
                    history_error = radio_history_store_commit();
                    if (history_error == ESP_OK) {
                        s_last_history_save_ms = now_ms;
                        ESP_LOGI(TAG, "history commit trigger=%s session=%u generation=%u count=%u",
                                 save_immediate ? "immediate" : "checkpoint",
                                 (unsigned)save_session, (unsigned)next_generation,
                                 (unsigned)save_count);
                    }
                }
                if (history_error != ESP_OK) {
                    ESP_LOGE(TAG, "History persistence failed: %s", esp_err_to_name(history_error));
                    if (bsp_lvgl_lock(250)) {
                        s_state.history_dirty = true;
                        s_state.history_flush_now = true;
                        bsp_lvgl_unlock();
                    }
                }
            }
        }
        if ((uint32_t)(now_ms - s_last_memory_log_ms) >= 30000u) {
            s_last_memory_log_ms = now_ms;
            log_memory_metrics("radio_scanner", (uint32_t)uxTaskGetStackHighWaterMark(NULL));
#if !CONFIG_RADIO_EXPLORER_SIMULATOR
            if (phase == RADIO_SCHEDULE_BLE && s_ble_started)
                ESP_LOGI(TAG, "NimBLE host stack_free=%u",
                         (unsigned)radio_ble_scanner_stack_high_water_bytes());
#endif
        }
        vTaskDelay(pdMS_TO_TICKS(125));
    }
}

static void app_tick(lv_timer_t *timer)
{
    (void)timer;
    radio_backend_event_t event;
    while (xQueueReceive(s_backend_events, &event, 0) == pdTRUE) apply_backend_event(&event);
    if (s_wifi_failed
#if CONFIG_RADIO_EXPLORER_SIMULATOR
        && !s_mock_started
#endif
    ) re_app_state_set_backend(&s_state, RE_BACKEND_FAILED);
    if (s_state.backend == RE_BACKEND_SCANNING &&
        nearby_store_expire(&s_state.nearby, s_state.sequence, 240) != 0) s_state.dirty = true;
    if (s_state.dirty) {
        re_ui_render(&s_state, s_battery);
        s_state.dirty = false;
    }
}

esp_err_t radio_explorer_app_init(void)
{
    re_app_state_init(&s_state, RE_NEARBY_SELECTION_FREEZE_OBSERVATIONS);
    radio_scheduler_init(&s_scheduler);
    log_memory_metrics("app_startup", (uint32_t)uxTaskGetStackHighWaterMark(NULL));
    esp_err_t history_error = demo_radio_nvs_prepare();
    if (history_error == ESP_OK) {
        size_t count = 0;
        uint32_t stored_session = 0, generation = 0;
        history_error = radio_history_store_load(s_state.history,
            RE_HISTORY_VOLATILE_CAPACITY, &count, &stored_session, &generation);
        if (history_error == ESP_OK || history_error == ESP_ERR_NOT_FOUND) {
            re_app_state_restore_history(&s_state, s_state.history, count,
                                         stored_session, generation);
            ESP_LOGI(TAG, "history restore count=%u stored_session=%u active_session=%u generation=%u",
                     (unsigned)count, (unsigned)stored_session,
                     (unsigned)s_state.session_id, (unsigned)generation);
        }
    }
    if (history_error != ESP_OK && history_error != ESP_ERR_NOT_FOUND)
        ESP_LOGE(TAG, "History restore failed; unrelated NVS is preserved: %s",
                 esp_err_to_name(history_error));
#if CONFIG_RADIO_EXPLORER_SIMULATOR && CONFIG_RADIO_EXPLORER_SIMULATOR_SEED_HISTORY
    re_app_state_seed_history(&s_state);
#endif
    if (history_error == ESP_OK || history_error == ESP_ERR_NOT_FOUND) {
        const uint32_t generation = s_state.history_generation + 1u;
        history_error = radio_history_store_save(s_state.history, s_state.history_count,
                                                  s_state.session_id, generation);
        if (history_error == ESP_OK) {
            s_state.history_generation = generation;
            s_state.history_dirty = false;
            s_state.history_flush_now = false;
            ESP_LOGI(TAG, "history commit trigger=startup session=%u generation=%u count=%u",
                     (unsigned)s_state.session_id, (unsigned)generation,
                     (unsigned)s_state.history_count);
        } else {
            ESP_LOGE(TAG, "Initial session persistence failed: %s", esp_err_to_name(history_error));
        }
    }
    s_backend_events = xQueueCreate(RADIO_BACKEND_EVENT_QUEUE_DEPTH, sizeof(radio_backend_event_t));
    if (s_backend_events == NULL) return ESP_ERR_NO_MEM;
    s_battery = bsp_battery_soc();
    re_ui_init(&s_state);
    s_tick_timer = lv_timer_create(app_tick, 125, NULL);
    if (s_tick_timer == NULL) return ESP_ERR_NO_MEM;
    if (xTaskCreate(scanner_worker, "radio_scanner", 4096, NULL, 4, NULL) != pdPASS)
        return ESP_ERR_NO_MEM;
    radio_backend_event_t event;
    while (xQueueReceive(s_backend_events, &event, 0) == pdTRUE) apply_backend_event(&event);
    re_ui_render(&s_state, s_battery);
    s_state.dirty = false;
    return ESP_OK;
}

void radio_explorer_app_button(bsp_btn_t button, bsp_btn_ev_t event)
{
    re_button_t key = RE_BUTTON_OTHER;
    if (event == BSP_BTN_CLICK) {
        if (button == BSP_BTN_UP) key = RE_BUTTON_UP;
        else if (button == BSP_BTN_DOWN) key = RE_BUTTON_DOWN;
        else if (button == BSP_BTN_OK) key = RE_BUTTON_OK_CLICK;
    } else if (event == BSP_BTN_LONG && button == BSP_BTN_OK) key = RE_BUTTON_OK_LONG;
    if (key == RE_BUTTON_OTHER) return;
    if (s_state.modal == RE_MODAL_NONE && s_state.screen == RE_SCREEN_DETAIL &&
        (key == RE_BUTTON_UP || key == RE_BUTTON_DOWN)) {
        re_ui_scroll_detail(key == RE_BUTTON_UP ? -1 : 1);
        return;
    }
    re_app_state_handle_button(&s_state, key);
    re_ui_render(&s_state, s_battery);
    s_state.dirty = false;
}

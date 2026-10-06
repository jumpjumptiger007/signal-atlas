#include "ble_scanner.h"

#include "../demo_radio.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"

#include <string.h>

enum { BLE_STOP_TIMEOUT_MS = 2000, BLE_HOST_STACK_BYTES = NIMBLE_HS_STACK_SIZE };

typedef struct {
    uint8_t address[6];
    uint8_t address_type;
    int8_t rssi;
    uint8_t data_length;
    uint8_t data[RADIO_BLE_AD_SNAPSHOT_MAX];
} radio_ble_report_t;

_Static_assert(sizeof(radio_ble_report_t) * RADIO_BLE_REPORT_QUEUE_DEPTH <= 1024u,
               "BLE report queue exceeds its 1 KiB budget");

static const char *TAG = "radio_ble";
static radio_ble_report_t s_reports[RADIO_BLE_REPORT_QUEUE_DEPTH];
static uint8_t s_report_head;
static uint8_t s_report_count;
static uint32_t s_dropped_reports;
static portMUX_TYPE s_report_lock = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE s_state_lock = portMUX_INITIALIZER_UNLOCKED;
static SemaphoreHandle_t s_host_stopped;
static TaskHandle_t s_host_task;
static bool s_initialized;
static bool s_stop_in_progress;
static bool s_host_done;
static bool s_start_requested;
static bool s_scanning;
static bool s_failed;
static int s_error;
static radio_backend_emit_fn s_emit;
static void *s_emit_context;
static int gap_event(struct ble_gap_event *event, void *arg);

size_t radio_ble_scanner_queue_bytes(void)
{
    return sizeof(s_reports);
}

static void set_failure(int error)
{
    portENTER_CRITICAL(&s_state_lock);
    s_failed = true;
    s_error = error;
    s_scanning = false;
    portEXIT_CRITICAL(&s_state_lock);
}

static int start_passive_scan(void)
{
    uint8_t own_address_type = 0;
    struct ble_gap_disc_params params = {0};
    int rc = ble_hs_id_infer_auto(0, &own_address_type);
    if (rc != 0) return rc;

    /* Follow the pinned blecent passive-discovery path, but disable controller
     * duplicate filtering so the Nearby store can update RSSI and seen counts. */
    params.filter_duplicates = 0;
    params.passive = 1;
    params.itvl = 0;
    params.window = 0;
    params.filter_policy = 0;
    params.limited = 0;
    rc = ble_gap_disc(own_address_type, BLE_HS_FOREVER, &params, gap_event, NULL);
    if (rc == 0) {
        portENTER_CRITICAL(&s_state_lock);
        s_scanning = true;
        portEXIT_CRITICAL(&s_state_lock);
    }
    return rc;
}

static int gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    if (event == NULL) return 0;
    if (event->type == BLE_GAP_EVENT_DISC) {
        const uint8_t length = event->disc.length_data;
        if (length > RADIO_BLE_AD_SNAPSHOT_MAX || (length != 0 && event->disc.data == NULL)) {
            portENTER_CRITICAL(&s_report_lock);
            s_dropped_reports++;
            portEXIT_CRITICAL(&s_report_lock);
            return 0;
        }

        radio_ble_report_t report = {0};
        memcpy(report.address, event->disc.addr.val, sizeof(report.address));
        report.address_type = event->disc.addr.type == BLE_ADDR_PUBLIC ?
            RADIO_ADDRESS_PUBLIC : RADIO_ADDRESS_RANDOM;
        report.rssi = event->disc.rssi;
        report.data_length = length;
        if (length != 0) memcpy(report.data, event->disc.data, length);

        portENTER_CRITICAL(&s_report_lock);
        if (s_report_count < RADIO_BLE_REPORT_QUEUE_DEPTH) {
            const uint8_t tail = (uint8_t)((s_report_head + s_report_count) % RADIO_BLE_REPORT_QUEUE_DEPTH);
            s_reports[tail] = report;
            s_report_count++;
        } else {
            s_dropped_reports++;
        }
        portEXIT_CRITICAL(&s_report_lock);
        return 0;
    }
    if (event->type == BLE_GAP_EVENT_DISC_COMPLETE) {
        portENTER_CRITICAL(&s_state_lock);
        const bool restart = s_start_requested;
        s_scanning = false;
        portEXIT_CRITICAL(&s_state_lock);
        if (restart) {
            const int rc = start_passive_scan();
            if (rc != 0) set_failure(rc);
        }
    }
    return 0;
}

static void on_reset(int reason)
{
    set_failure(reason);
}

static void on_sync(void)
{
    portENTER_CRITICAL(&s_state_lock);
    const bool requested = s_start_requested;
    portEXIT_CRITICAL(&s_state_lock);
    if (!requested) return;
    const int rc = start_passive_scan();
    if (rc != 0) set_failure(rc);
}

static void host_task(void *arg)
{
    (void)arg;
    nimble_port_run();
    xSemaphoreGive(s_host_stopped);
    for (;;) vTaskSuspend(NULL);
}

bool radio_ble_scanner_start(radio_backend_emit_fn emit, void *emit_context)
{
    if (s_initialized) return s_host_task != NULL;
    s_emit = emit;
    s_emit_context = emit_context;
    s_start_requested = true;
    s_stop_in_progress = false;
    s_host_done = false;
    s_scanning = false;
    s_failed = false;
    s_error = 0;
    portENTER_CRITICAL(&s_report_lock);
    s_report_head = s_report_count = 0;
    s_dropped_reports = 0;
    portEXIT_CRITICAL(&s_report_lock);

    esp_err_t error = demo_radio_nvs_prepare();
    if (error != ESP_OK) goto failed_start;
    error = nimble_port_init();
    if (error != ESP_OK) goto failed_start;
    s_initialized = true;
    s_host_stopped = xSemaphoreCreateBinary();
    if (s_host_stopped == NULL) { error = ESP_ERR_NO_MEM; goto failed_start; }

    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    if (xTaskCreatePinnedToCore(host_task, "radio_nimble", BLE_HOST_STACK_BYTES,
                                NULL, configMAX_PRIORITIES - 4, &s_host_task,
                                NIMBLE_CORE) != pdPASS) {
        error = ESP_ERR_NO_MEM;
        goto failed_start;
    }
    return true;

failed_start:
    portENTER_CRITICAL(&s_state_lock);
    s_start_requested = false;
    portEXIT_CRITICAL(&s_state_lock);
    (void)radio_ble_scanner_stop();
    set_failure(error);
    ESP_LOGE(TAG, "NimBLE passive scanner start failed: %s", esp_err_to_name(error));
    return false;
}

esp_err_t radio_ble_scanner_stop(void)
{
    portENTER_CRITICAL(&s_state_lock);
    s_start_requested = false;
    s_scanning = false;
    portEXIT_CRITICAL(&s_state_lock);
    if (!s_initialized) {
        s_emit = NULL;
        s_emit_context = NULL;
        return ESP_OK;
    }

    if (s_host_task != NULL && !s_stop_in_progress) {
        (void)ble_gap_disc_cancel();
        const int rc = nimble_port_stop();
        if (rc != 0) {
            ESP_LOGE(TAG, "nimble_port_stop failed: %d", rc);
            set_failure(rc);
            return ESP_FAIL;
        }
        s_stop_in_progress = true;
    }
    if (s_host_task != NULL && !s_host_done) {
        if (s_host_stopped == NULL ||
            xSemaphoreTake(s_host_stopped, pdMS_TO_TICKS(BLE_STOP_TIMEOUT_MS)) != pdTRUE) {
            set_failure(ESP_ERR_TIMEOUT);
            return ESP_ERR_TIMEOUT;
        }
        s_host_done = true;
    }
    if (s_host_task != NULL) {
        vTaskDelete(s_host_task);
        s_host_task = NULL;
    }
    const esp_err_t error = nimble_port_deinit();
    if (error != ESP_OK) {
        set_failure(error);
        return error;
    }
    s_initialized = false;
    s_stop_in_progress = false;
    s_host_done = false;
    if (s_host_stopped != NULL) {
        vSemaphoreDelete(s_host_stopped);
        s_host_stopped = NULL;
    }
    s_emit = NULL;
    s_emit_context = NULL;
    portENTER_CRITICAL(&s_report_lock);
    s_report_head = s_report_count = 0;
    portEXIT_CRITICAL(&s_report_lock);
    return ESP_OK;
}

static bool take_report(radio_ble_report_t *report)
{
    bool available = false;
    portENTER_CRITICAL(&s_report_lock);
    if (s_report_count != 0) {
        *report = s_reports[s_report_head];
        s_report_head = (uint8_t)((s_report_head + 1u) % RADIO_BLE_REPORT_QUEUE_DEPTH);
        s_report_count--;
        available = true;
    }
    portEXIT_CRITICAL(&s_report_lock);
    return available;
}

void radio_ble_scanner_tick(uint32_t now_seq)
{
    radio_ble_report_t report;
    for (unsigned count = 0; count < RADIO_BLE_REPORT_QUEUE_DEPTH && take_report(&report); ++count) {
        radio_observation_t observation;
        if (!radio_ble_observation_from_ad(&observation, report.address,
                (radio_address_type_t)report.address_type, report.rssi, now_seq,
                report.data, report.data_length)) continue;
        if (s_emit != NULL) {
            const radio_backend_event_t event = {
                .kind = RADIO_BACKEND_EVENT_OBSERVATION,
                .observation = observation,
            };
            (void)s_emit(s_emit_context, &event);
        }
    }
}

bool radio_ble_scanner_started(void) { return s_initialized && s_host_task != NULL; }

bool radio_ble_scanner_scanning(void)
{
    portENTER_CRITICAL(&s_state_lock);
    const bool scanning = s_scanning;
    portEXIT_CRITICAL(&s_state_lock);
    return scanning;
}

bool radio_ble_scanner_failed(void)
{
    portENTER_CRITICAL(&s_state_lock);
    const bool failed = s_failed;
    portEXIT_CRITICAL(&s_state_lock);
    return failed;
}

uint32_t radio_ble_scanner_dropped_reports(void)
{
    portENTER_CRITICAL(&s_report_lock);
    const uint32_t dropped = s_dropped_reports;
    portEXIT_CRITICAL(&s_report_lock);
    return dropped;
}

uint32_t radio_ble_scanner_stack_high_water_bytes(void)
{
    return s_host_task == NULL ? 0u : (uint32_t)uxTaskGetStackHighWaterMark(s_host_task);
}

int radio_ble_scanner_error(void)
{
    portENTER_CRITICAL(&s_state_lock);
    const int error = s_error;
    portEXIT_CRITICAL(&s_state_lock);
    return error;
}

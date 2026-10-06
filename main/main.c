#include "app.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "radio_explorer_boot";
enum { INPUT_QUEUE_DEPTH = 8 };
typedef struct { bsp_btn_t button; bsp_btn_ev_t event; } input_event_t;
static QueueHandle_t s_input_queue;
static volatile bool s_ready;

static void on_button(bsp_btn_t button, bsp_btn_ev_t event, void *user)
{
    (void)user;
    if (!s_ready || s_input_queue == NULL) return;
    const input_event_t input = { .button = button, .event = event };
    (void)xQueueSend(s_input_queue, &input, 0);
}

static void input_task(void *arg)
{
    (void)arg;
    input_event_t input;
    for (;;) {
        if (xQueueReceive(s_input_queue, &input, portMAX_DELAY) != pdTRUE) continue;
        if (!bsp_lvgl_lock(500)) continue;
        radio_explorer_app_button(input.button, input.event);
        bsp_lvgl_unlock();
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Signal Atlas v0.1 boot");
    (void)bsp_battery_init();
    if (bsp_display_init() != ESP_OK || bsp_lvgl_init() == NULL) {
        ESP_LOGE(TAG, "Display/LVGL initialization failed");
        return;
    }
    bsp_display_backlight(100);
    s_input_queue = xQueueCreate(INPUT_QUEUE_DEPTH, sizeof(input_event_t));
    if (s_input_queue == NULL || xTaskCreate(input_task, "radio_input", 6144, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Button input worker allocation failed");
        return;
    }
    if (bsp_button_init(on_button, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "Button initialization failed");
        return;
    }
    if (!bsp_lvgl_lock(1000)) return;
    esp_err_t error = radio_explorer_app_init();
    bsp_lvgl_unlock();
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "Signal Atlas application initialization failed: %s", esp_err_to_name(error));
        return;
    }
    s_ready = true;
}

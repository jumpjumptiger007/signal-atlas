#include "wifi_scanner.h"

#include "../demo_radio.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include <string.h>

enum { RADIO_WIFI_MAX_AP_RECORDS = 32 };

static const char *TAG = "radio_wifi";
static esp_netif_t *s_sta_netif;
static esp_event_handler_instance_t s_scan_handler;
static bool s_wifi_initialized;
static bool s_wifi_started;
static bool s_handler_registered;
static bool s_scan_complete;
static bool s_failed;
static radio_backend_emit_fn s_emit;
static void *s_emit_context;
static portMUX_TYPE s_scan_lock = portMUX_INITIALIZER_UNLOCKED;
static wifi_ap_record_t s_records[RADIO_WIFI_MAX_AP_RECORDS];

static void emit_error(esp_err_t error)
{
    if (s_emit != NULL) {
        const radio_backend_event_t event = {
            .kind = RADIO_BACKEND_EVENT_ERROR,
            .error = error
        };
        (void)s_emit(s_emit_context, &event);
    }
}

static void scan_done(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)data;
    if (base != WIFI_EVENT || id != WIFI_EVENT_SCAN_DONE) return;
    portENTER_CRITICAL(&s_scan_lock);
    s_scan_complete = true;
    portEXIT_CRITICAL(&s_scan_lock);
}

static esp_err_t start_scan(void)
{
    if (!s_wifi_started) return ESP_ERR_INVALID_STATE;
    const wifi_scan_config_t config = {
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .show_hidden = true,
    };
    esp_err_t error = esp_wifi_scan_start(&config, false);
    if (error != ESP_OK) s_failed = true;
    return error;
}

static void wifi_stack_stop(void)
{
    if (s_wifi_started) {
        (void)esp_wifi_scan_stop();
        (void)esp_wifi_stop();
        s_wifi_started = false;
    }
    if (s_handler_registered) {
        (void)esp_event_handler_instance_unregister(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, s_scan_handler);
        s_handler_registered = false;
    }
    if (s_wifi_initialized) {
        (void)esp_wifi_deinit();
        s_wifi_initialized = false;
    }
    if (s_sta_netif != NULL) {
        esp_netif_destroy_default_wifi(s_sta_netif);
        s_sta_netif = NULL;
    }
    portENTER_CRITICAL(&s_scan_lock);
    s_scan_complete = false;
    portEXIT_CRITICAL(&s_scan_lock);
}

bool radio_wifi_scanner_start(radio_backend_emit_fn emit, void *emit_context)
{
    if (s_wifi_started) return true;
    if (s_sta_netif != NULL || s_wifi_initialized) return false;
    s_emit = emit;
    s_emit_context = emit_context;
    s_failed = false;

    esp_err_t error = demo_radio_nvs_prepare();
    if (error != ESP_OK) goto fail;
    error = demo_radio_network_prepare();
    if (error != ESP_OK) goto fail;

    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_WIFI_STA();
    s_sta_netif = esp_netif_new(&netif_config);
    if (s_sta_netif == NULL) { error = ESP_ERR_NO_MEM; goto fail; }
    error = esp_netif_attach_wifi_station(s_sta_netif);
    if (error != ESP_OK) goto fail;
    error = esp_wifi_set_default_wifi_sta_handlers();
    if (error != ESP_OK) goto fail;

    const wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    error = esp_wifi_init(&wifi_config);
    if (error != ESP_OK) goto fail;
    s_wifi_initialized = true;
    error = esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE,
                                                scan_done, NULL, &s_scan_handler);
    if (error != ESP_OK) goto fail;
    s_handler_registered = true;
    error = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (error != ESP_OK) goto fail;
    error = esp_wifi_set_mode(WIFI_MODE_STA);
    if (error != ESP_OK) goto fail;
    error = esp_wifi_start();
    if (error != ESP_OK) goto fail;
    s_wifi_started = true;
    if (start_scan() != ESP_OK) goto fail;
    ESP_LOGI(TAG, "STA scan-only flow started; association is disabled");
    return true;

fail:
    wifi_stack_stop();
    s_failed = true;
    ESP_LOGE(TAG, "Wi-Fi scanner start failed: %s", esp_err_to_name(error));
    emit_error(error);
    return false;
}

static uint8_t bounded_ssid_length(const uint8_t ssid[RADIO_WIFI_SSID_MAX])
{
    uint8_t length = 0;
    while (length < RADIO_WIFI_SSID_MAX && ssid[length] != 0) ++length;
    return length;
}

void radio_wifi_scanner_tick(uint32_t now_seq)
{
    (void)now_seq;
    if (!s_wifi_started || s_failed) return;
    portENTER_CRITICAL(&s_scan_lock);
    const bool completed = s_scan_complete;
    s_scan_complete = false;
    portEXIT_CRITICAL(&s_scan_lock);
    if (!completed) return;

    uint16_t total = 0;
    esp_err_t error = esp_wifi_scan_get_ap_num(&total);
    if (error != ESP_OK) goto fail;
    uint16_t count = total < RADIO_WIFI_MAX_AP_RECORDS ? total : RADIO_WIFI_MAX_AP_RECORDS;
    memset(s_records, 0, sizeof(s_records));
    if (count != 0) {
        error = esp_wifi_scan_get_ap_records(&count, s_records);
        if (error != ESP_OK) goto fail;
    }
    for (uint16_t i = 0; i < count; ++i) {
        radio_wifi_ap_snapshot_t snapshot = {0};
        snapshot.ssid_length = bounded_ssid_length(s_records[i].ssid);
        if (snapshot.ssid_length != 0)
            memcpy(snapshot.ssid, s_records[i].ssid, snapshot.ssid_length);
        memcpy(snapshot.bssid, s_records[i].bssid, sizeof(snapshot.bssid));
        snapshot.rssi = s_records[i].rssi;
        snapshot.channel = s_records[i].primary;
        snapshot.auth_mode = (uint8_t)s_records[i].authmode;
        radio_observation_t observation;
        if (!radio_wifi_observation_from_ap(&observation, &snapshot, now_seq)) continue;
        if (s_emit != NULL) {
            const radio_backend_event_t event = {
                .kind = RADIO_BACKEND_EVENT_OBSERVATION,
                .observation = observation,
            };
            (void)s_emit(s_emit_context, &event);
        }
    }
    error = start_scan();
    if (error != ESP_OK) { emit_error(error); return; }
    return;

fail:
    s_failed = true;
    ESP_LOGE(TAG, "Wi-Fi scan result retrieval failed: %s", esp_err_to_name(error));
    emit_error(error);
}

void radio_wifi_scanner_stop(void)
{
    wifi_stack_stop();
    s_failed = false;
    s_emit = NULL;
    s_emit_context = NULL;
}

bool radio_wifi_scanner_started(void) { return s_wifi_started; }
bool radio_wifi_scanner_failed(void) { return s_failed; }

#define demo_radio_nvs_prepare demo_radio_nvs_prepare_stub
#define demo_radio_network_prepare demo_radio_network_prepare_stub
#include "demo_stubs/demo_runtime.c"
#undef demo_radio_nvs_prepare
#undef demo_radio_network_prepare
#include "../main/radio/wifi_scanner.c"

#include <assert.h>
#include <stdio.h>

static unsigned step;
static unsigned fail_at;
static unsigned destroys;
static bool netif_live;
static bool driver_registered;
static bool defaults_live;
static bool wifi_live;
static bool wifi_running;
static bool scan_handler_live;
static bool scan_pending;
static unsigned scan_starts;
static esp_event_handler_instance_t scan_handler;
static void (*scan_callback)(void *, esp_event_base_t, int32_t, void *);
static esp_netif_t netif;
static radio_backend_event_t emitted[4];
static unsigned emitted_count;

static esp_err_t next_step(void)
{
    return ++step == fail_at ? ESP_ERR_NO_MEM : ESP_OK;
}

esp_err_t demo_radio_nvs_prepare(void) { return next_step(); }
esp_err_t demo_radio_network_prepare(void) { return next_step(); }

esp_netif_t *esp_netif_new(const esp_netif_config_t *config)
{
    (void)config;
    if (next_step() != ESP_OK) return NULL;
    assert(!netif_live);
    netif_live = true;
    return &netif;
}

esp_err_t esp_netif_attach_wifi_station(esp_netif_t *created)
{
    assert(created == &netif && netif_live);
    driver_registered = true;
    return next_step();
}

esp_err_t esp_wifi_set_default_wifi_sta_handlers(void)
{
    assert(driver_registered);
    esp_err_t result = next_step();
    defaults_live = result == ESP_OK;
    return result;
}

void esp_netif_destroy_default_wifi(void *created)
{
    assert(created == &netif && netif_live && !wifi_live && !scan_handler_live);
    netif_live = driver_registered = defaults_live = false;
    destroys++;
}

esp_err_t esp_wifi_init(const wifi_init_config_t *config)
{
    (void)config;
    assert(defaults_live);
    esp_err_t result = next_step();
    wifi_live = result == ESP_OK;
    return result;
}

esp_err_t esp_wifi_deinit(void)
{
    assert(wifi_live && !wifi_running && !scan_handler_live);
    wifi_live = false;
    return ESP_OK;
}

esp_err_t esp_event_handler_instance_register(
    esp_event_base_t base, int32_t id,
    void (*callback)(void *, esp_event_base_t, int32_t, void *), void *arg,
    esp_event_handler_instance_t *instance)
{
    (void)base;
    (void)id;
    (void)arg;
    esp_err_t result = next_step();
    if (result == ESP_OK) {
        scan_handler_live = true;
        scan_callback = callback;
        *instance = &scan_handler;
    }
    return result;
}

esp_err_t esp_event_handler_instance_unregister(esp_event_base_t base, int32_t id,
                                                 esp_event_handler_instance_t instance)
{
    (void)base;
    (void)id;
    assert(scan_handler_live && instance == &scan_handler);
    scan_handler_live = false;
    scan_callback = NULL;
    return ESP_OK;
}

esp_err_t esp_wifi_set_storage(int storage) { (void)storage; return next_step(); }
esp_err_t esp_wifi_set_mode(int mode) { (void)mode; return next_step(); }

esp_err_t esp_wifi_start(void)
{
    esp_err_t result = next_step();
    wifi_running = result == ESP_OK;
    return result;
}

esp_err_t esp_wifi_stop(void)
{
    assert(wifi_running);
    wifi_running = false;
    return ESP_OK;
}

esp_err_t esp_wifi_scan_start(const void *config, bool blocking)
{
    const wifi_scan_config_t *scan = config;
    assert(wifi_running && !blocking);
    assert(scan->scan_type == WIFI_SCAN_TYPE_ACTIVE && scan->show_hidden);
    scan_starts++;
    scan_pending = true;
    return next_step();
}

esp_err_t esp_wifi_scan_stop(void)
{
    assert(wifi_running);
    scan_pending = false;
    return ESP_OK;
}

esp_err_t esp_wifi_scan_get_ap_num(uint16_t *count)
{
    assert(scan_pending);
    *count = 1;
    return ESP_OK;
}

esp_err_t esp_wifi_scan_get_ap_records(uint16_t *count, wifi_ap_record_t *records)
{
    assert(scan_pending && *count >= 1);
    *count = 1;
    records[0].rssi = -51;
    records[0].ssid[0] = 'A';
    records[0].ssid[1] = 'P';
    records[0].bssid[0] = 0x02; /* locally administered AP address */
    records[0].bssid[5] = 0x44;
    records[0].primary = 11;
    records[0].authmode = RADIO_WIFI_AUTH_WPA2_PSK;
    scan_pending = false;
    return ESP_OK;
}

static bool collect(void *context, const radio_backend_event_t *event)
{
    (void)context;
    if (emitted_count < sizeof(emitted) / sizeof(emitted[0]))
        emitted[emitted_count++] = *event;
    return true;
}

static void assert_clean(void)
{
    assert(!s_sta_netif && !s_wifi_initialized && !s_wifi_started && !s_handler_registered);
    assert(!netif_live && !driver_registered && !defaults_live);
    assert(!wifi_live && !wifi_running && !scan_handler_live && !scan_pending);
}

int main(void)
{
    for (unsigned failure = 1; failure <= 11; ++failure) {
        step = 0;
        fail_at = failure;
        unsigned before = destroys;
        assert(!radio_wifi_scanner_start(collect, NULL));
        assert(step == failure && radio_wifi_scanner_failed());
        assert(destroys == before + (failure >= 4));
        assert_clean();
        radio_wifi_scanner_stop();
        assert_clean();
        emitted_count = 0;

        step = fail_at = 0;
        assert(radio_wifi_scanner_start(collect, NULL));
        assert(step == 11 && radio_wifi_scanner_started());
        assert(radio_wifi_scanner_start(collect, NULL));
        assert(scan_callback != NULL);
        scan_callback(NULL, WIFI_EVENT, WIFI_EVENT_SCAN_DONE, NULL);
        radio_wifi_scanner_tick(77);
        assert(emitted_count == 1 && emitted[0].kind == RADIO_BACKEND_EVENT_OBSERVATION);
        assert(emitted[0].observation.kind == RADIO_KIND_WIFI);
        assert(emitted[0].observation.seen_seq == 77);
        assert(emitted[0].observation.rssi == -51);
        assert(emitted[0].observation.data.wifi.ssid_len == 2);
        assert(emitted[0].observation.data.wifi.channel == 11);
        assert(emitted[0].observation.data.wifi.security == RADIO_SECURITY_WPA2_PSK);
        assert(scan_starts >= 2); /* scan is re-armed after extracting one bounded batch */
        emitted_count = 0;
        radio_wifi_scanner_stop();
        assert_clean();
    }

    puts("Radio Wi-Fi scanner startup rollback, normalized results, and repeat start/stop: PASS");
    return 0;
}

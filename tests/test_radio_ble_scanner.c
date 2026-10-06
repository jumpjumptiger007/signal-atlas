#define demo_radio_nvs_prepare demo_radio_nvs_prepare_stub
#define demo_radio_network_prepare demo_radio_network_prepare_stub
#include "demo_stubs/demo_runtime.c"
#undef demo_radio_nvs_prepare
#undef demo_radio_network_prepare
#include "../main/radio/ble_scanner.c"

#include <assert.h>
#include <stdio.h>

static int (*discovery_callback)(struct ble_gap_event *, void *);
static struct ble_gap_disc_params captured_params;
static int32_t captured_duration;
static unsigned discovery_starts;
static unsigned discovery_cancels;
static radio_backend_event_t emitted[16];
static unsigned emitted_count;

esp_err_t demo_radio_nvs_prepare(void) { return ESP_OK; }
esp_err_t demo_radio_network_prepare(void) { return ESP_OK; }

int ble_gap_disc(uint8_t own_address_type, int32_t duration,
                 const struct ble_gap_disc_params *params,
                 int (*callback)(struct ble_gap_event *, void *), void *arg)
{
    (void)arg;
    assert(own_address_type == 0);
    captured_duration = duration;
    captured_params = *params;
    discovery_callback = callback;
    discovery_starts++;
    return 0;
}

int ble_gap_disc_cancel(void)
{
    discovery_cancels++;
    return 0;
}

static bool collect(void *context, const radio_backend_event_t *event)
{
    (void)context;
    if (emitted_count < sizeof(emitted) / sizeof(emitted[0])) emitted[emitted_count++] = *event;
    return true;
}

static void submit_advertisement(uint8_t address_type, uint8_t last_address_byte)
{
    const uint8_t advertisement[] = { 3, 0x09, 'A', 'P' };
    struct ble_gap_event event = { .type = BLE_GAP_EVENT_DISC };
    event.disc.addr.type = address_type;
    event.disc.addr.val[0] = 0x44;
    event.disc.addr.val[5] = last_address_byte;
    event.disc.rssi = -63;
    event.disc.data = advertisement;
    event.disc.length_data = sizeof(advertisement);
    assert(discovery_callback != NULL);
    assert(discovery_callback(&event, NULL) == 0);
}

int main(void)
{
    /* Host-task allocation failure must unwind NimBLE without a false stop call. */
    test_create_fails = true;
    assert(!radio_ble_scanner_start(collect, NULL));
    assert(radio_ble_scanner_failed());
    assert(test_nimble_stops == 0 && test_nimble_deinits == 1);
    test_create_fails = false;
    discovery_callback = NULL;
    emitted_count = 0;

    assert(radio_ble_scanner_start(collect, NULL));
    assert(radio_ble_scanner_started() && !radio_ble_scanner_scanning());
    assert(ble_hs_cfg.sync_cb != NULL);
    ble_hs_cfg.sync_cb();
    assert(radio_ble_scanner_scanning());
    assert(discovery_starts == 1 && captured_duration == BLE_HS_FOREVER);
    assert(captured_params.passive == 1 && captured_params.filter_duplicates == 0);

    submit_advertisement(BLE_ADDR_PUBLIC, 0x01);
    submit_advertisement(1, 0x02);
    radio_ble_scanner_tick(123);
    assert(emitted_count == 2);
    assert(emitted[0].kind == RADIO_BACKEND_EVENT_OBSERVATION);
    assert(emitted[0].observation.kind == RADIO_KIND_BLE);
    assert(emitted[0].observation.identity.address_type == RADIO_ADDRESS_PUBLIC);
    assert(emitted[1].observation.identity.address_type == RADIO_ADDRESS_RANDOM);
    assert(emitted[0].observation.seen_seq == 123 && emitted[0].observation.rssi == -63);
    assert(emitted[0].observation.data.ble.name_len == 2);

    emitted_count = 0;
    for (unsigned i = 0; i < RADIO_BLE_REPORT_QUEUE_DEPTH + 2; ++i)
        submit_advertisement(BLE_ADDR_PUBLIC, (uint8_t)i);
    assert(radio_ble_scanner_dropped_reports() == 2);
    radio_ble_scanner_tick(124);
    assert(emitted_count == RADIO_BLE_REPORT_QUEUE_DEPTH);

    /* The host task acknowledges only after nimble_port_run has returned. */
    test_run_worker(s_host_task);
    assert(radio_ble_scanner_stop() == ESP_OK);
    assert(discovery_cancels == 1 && test_nimble_stops == 1);
    assert(!radio_ble_scanner_started() && radio_ble_scanner_queue_bytes() <= 1024);
    puts("Radio BLE passive-scan lifecycle, bounded report pipeline, and address identity: PASS");
    return 0;
}

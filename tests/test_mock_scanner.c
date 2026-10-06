#include "radio/mock_scanner.h"
#include "identify/protocol_decoder.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { radio_observation_t observations[16]; size_t count; size_t states; size_t errors; } capture_t;
static bool capture(void *context, const radio_backend_event_t *event)
{
    capture_t *out = context;
    if (event->kind == RADIO_BACKEND_EVENT_OBSERVATION && out->count < 16)
        out->observations[out->count++] = event->observation;
    else if (event->kind == RADIO_BACKEND_EVENT_STATE) ++out->states;
    else if (event->kind == RADIO_BACKEND_EVENT_ERROR) ++out->errors;
    return true;
}

int main(void)
{
    mock_scanner_t scanner;
    capture_t out = {0};
    mock_scanner_init(&scanner, MOCK_SCANNER_NORMAL);
    assert(mock_scanner_start(&scanner, capture, &out));
    assert(out.states == 1 && out.errors == 0);
    for (uint32_t i = 0; i < 12; ++i) mock_scanner_tick(&scanner, i + 1);
    assert(out.count == 12);
    assert(out.observations[0].data.ble.name_len == 12);
    assert(out.observations[1].data.ble.has_company_id && out.observations[1].data.ble.company_id == 0x004c);
    radio_decoded_frame_t frame;
    assert(radio_decode_ibeacon(out.observations[1].data.ble.company_id,
        out.observations[1].data.ble.manufacturer, out.observations[1].data.ble.manufacturer_len, &frame));
    assert(frame.kind == RADIO_FRAME_IBEACON);
    assert(out.observations[2].data.ble.service_data_len >= 22);
    assert(out.observations[2].data.ble.appearance_present && out.observations[2].data.ble.appearance == 0x03c0);
    assert(radio_decode_eddystone(out.observations[2].data.ble.service_data,
        out.observations[2].data.ble.service_data_len, &frame));
    assert(frame.kind == RADIO_FRAME_EDDYSTONE_UID);
    assert(out.observations[0].identity.address[5] == out.observations[7].identity.address[5]);
    assert(out.observations[0].rssi != out.observations[7].rssi);
    assert(radio_decode_eddystone(out.observations[4].data.ble.service_data,
        out.observations[4].data.ble.service_data_len, &frame));
    assert(frame.kind == RADIO_FRAME_EDDYSTONE_URL);
    assert(radio_decode_eddystone(out.observations[8].data.ble.service_data,
        out.observations[8].data.ble.service_data_len, &frame));
    assert(frame.kind == RADIO_FRAME_EDDYSTONE_TLM);
    mock_scanner_stop(&scanner);
    assert(!scanner.started);

    mock_scanner_init(&scanner, MOCK_SCANNER_OVERFLOW);
    memset(&out, 0, sizeof(out));
    assert(mock_scanner_start(&scanner, capture, &out));
    for (uint32_t i = 0; i < 194; ++i) mock_scanner_tick(&scanner, i + 1);
    assert(out.count == 16); /* callback is bounded; later scenario calls remain bounded too */
    assert(scanner.fixture_index == 97);
    mock_scanner_stop(&scanner);

    mock_scanner_init(&scanner, MOCK_SCANNER_EMPTY);
    memset(&out, 0, sizeof(out));
    assert(mock_scanner_start(&scanner, capture, &out));
    mock_scanner_tick(&scanner, 1);
    assert(out.count == 0);
    mock_scanner_stop(&scanner);

    mock_scanner_init(&scanner, MOCK_SCANNER_FAILURE);
    memset(&out, 0, sizeof(out));
    assert(!mock_scanner_start(&scanner, capture, &out));
    assert(out.errors == 1);
    puts("Mock scanner production-pipeline tests: PASS");
    return 0;
}

#include "mock_scanner.h"

#include <string.h>

/* Raw, bounded AD fixtures deliberately exercise the production parser. */
static const uint8_t AD_NAME[] = { 2, 0x01, 0x06, 13, 0x09, 'R','a','d','i','o',' ','B','e','a','c','o','n' };
static const uint8_t AD_COMPANY[] = {
    2,0x01,0x06, 26,0xFF,0x4C,0x00,0x02,0x15,
    0xE2,0xC5,0x6D,0xB5,0xDF,0xFB,0x48,0xD2,0xB0,0x60,0xD0,0xF5,0xA7,0x10,0x96,0xE0,
    0x00,0x01,0x00,0x02,0xC5
};
static const uint8_t AD_SERVICE[] = {
    3,0x03,0xAA,0xFE,
    23,0x16,0xAA,0xFE,0x00,0xEE,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,
    0x01,0x02,0x03,0x04,0x05,0x06,0x00,0x00
    ,3,0x19,0xC0,0x03
};
static const uint8_t AD_URL[] = {
    14,0x16,0xAA,0xFE,0x10,0xEC,0x02,'e','x','a','m','p','l','e',0x07
};
static const uint8_t AD_TLM[] = {
    17,0x16,0xAA,0xFE,0x20,0x00,0x0C,0x34,0x19,0x00,
    0x00,0x00,0x00,0x21,0x12,0x34,0x56,0x78
};
static const uint8_t AD_EMPTY[] = { 2,0x01,0x06 };

void mock_scanner_init(mock_scanner_t *scanner, mock_scanner_scenario_t scenario)
{
    if (scanner == NULL) return;
    memset(scanner, 0, sizeof(*scanner));
    scanner->scenario = (uint8_t)scenario;
}

bool mock_scanner_start(void *context, radio_backend_emit_fn emit, void *emit_context)
{
    mock_scanner_t *scanner = context;
    if (scanner == NULL || emit == NULL) return false;
    scanner->emit = emit;
    scanner->emit_context = emit_context;
    scanner->ticks = 0;
    scanner->fixture_index = 0;
    scanner->started = 1;
    radio_backend_event_t event = { .kind = RADIO_BACKEND_EVENT_STATE, .state = RADIO_BACKEND_SCANNING };
    (void)emit(emit_context, &event);
    if (scanner->scenario == MOCK_SCANNER_FAILURE) {
        event.kind = RADIO_BACKEND_EVENT_ERROR; event.state = RADIO_BACKEND_FAILED; event.error = -1;
        (void)emit(emit_context, &event);
        scanner->started = 0;
        return false;
    }
    return true;
}

void mock_scanner_stop(void *context)
{
    mock_scanner_t *scanner = context;
    if (scanner == NULL) return;
    scanner->started = 0;
    scanner->emit = NULL;
    scanner->emit_context = NULL;
}

static void emit_ad(mock_scanner_t *scanner, uint32_t seq, const uint8_t *address,
                    radio_address_type_t address_type, int8_t rssi,
                    const uint8_t *ad, size_t len)
{
    radio_backend_event_t event;
    memset(&event, 0, sizeof(event));
    event.kind = RADIO_BACKEND_EVENT_OBSERVATION;
    if (!radio_ble_observation_from_ad(&event.observation, address, address_type, rssi, seq, ad, len)) return;
    (void)scanner->emit(scanner->emit_context, &event);
}

void mock_scanner_tick(void *context, uint32_t now_seq)
{
    mock_scanner_t *scanner = context;
    if (scanner == NULL || !scanner->started || scanner->scenario == MOCK_SCANNER_EMPTY) return;
    ++scanner->ticks;
    if (scanner->scenario == MOCK_SCANNER_OVERFLOW) {
        if ((scanner->ticks & 1u) != 0) return;
        uint8_t address[6] = { 0xD2, 0x01, 0, 0, (uint8_t)(scanner->fixture_index >> 8), (uint8_t)scanner->fixture_index };
        ++scanner->fixture_index;
        emit_ad(scanner, now_seq, address, RADIO_ADDRESS_RANDOM, (int8_t)(-35 - scanner->fixture_index % 55), AD_EMPTY, sizeof(AD_EMPTY));
        return;
    }
    static const uint8_t addresses[][6] = {
        { 0xA0,0x11,0x22,0x33,0x44,0x55 },
        { 0xC2,0x10,0x20,0x30,0x40,0x50 },
        { 0xC2,0x10,0x20,0x30,0x40,0x51 },
        { 0xC2,0x10,0x20,0x30,0x40,0x52 },
    };
    const uint8_t i = (uint8_t)((scanner->ticks - 1u) % 10u);
    if (i == 0) emit_ad(scanner, now_seq, addresses[0], RADIO_ADDRESS_PUBLIC,
                                  (int8_t)(-47 - (scanner->ticks % 8)), AD_NAME, sizeof(AD_NAME));
    else if (i == 1 || i == 5) emit_ad(scanner, now_seq, addresses[1], RADIO_ADDRESS_RANDOM, (int8_t)(-61 + scanner->ticks % 4), AD_COMPANY, sizeof(AD_COMPANY));
    else if (i == 2 || i == 6) emit_ad(scanner, now_seq, addresses[2], RADIO_ADDRESS_RANDOM, -72, AD_SERVICE, sizeof(AD_SERVICE));
    else if (i == 3) emit_ad(scanner, now_seq, addresses[3], RADIO_ADDRESS_RANDOM, -83, AD_EMPTY, sizeof(AD_EMPTY));
    else if (i == 4) emit_ad(scanner, now_seq, addresses[2], RADIO_ADDRESS_RANDOM, -70, AD_URL, sizeof(AD_URL));
    else if (i == 8) emit_ad(scanner, now_seq, addresses[2], RADIO_ADDRESS_RANDOM, -71, AD_TLM, sizeof(AD_TLM));
    else emit_ad(scanner, now_seq, addresses[0], RADIO_ADDRESS_PUBLIC, -49, AD_NAME, sizeof(AD_NAME));
}

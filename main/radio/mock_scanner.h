#ifndef RADIO_EXPLORER_MOCK_SCANNER_H
#define RADIO_EXPLORER_MOCK_SCANNER_H

#include "radio.h"

typedef enum { MOCK_SCANNER_NORMAL = 0, MOCK_SCANNER_EMPTY, MOCK_SCANNER_FAILURE,
               MOCK_SCANNER_OVERFLOW } mock_scanner_scenario_t;
typedef struct {
    radio_backend_emit_fn emit;
    void *emit_context;
    uint32_t ticks;
    uint8_t started;
    uint8_t scenario;
    uint8_t fixture_index;
} mock_scanner_t;

void mock_scanner_init(mock_scanner_t *scanner, mock_scanner_scenario_t scenario);
bool mock_scanner_start(void *context, radio_backend_emit_fn emit, void *emit_context);
void mock_scanner_stop(void *context);
void mock_scanner_tick(void *context, uint32_t now_seq);

#endif

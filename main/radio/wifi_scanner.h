#ifndef RADIO_EXPLORER_WIFI_SCANNER_H
#define RADIO_EXPLORER_WIFI_SCANNER_H

#include "radio.h"

bool radio_wifi_scanner_start(radio_backend_emit_fn emit, void *emit_context);
void radio_wifi_scanner_tick(uint32_t now_seq);
void radio_wifi_scanner_stop(void);
bool radio_wifi_scanner_started(void);
bool radio_wifi_scanner_failed(void);

#endif

#ifndef RADIO_EXPLORER_BLE_SCANNER_H
#define RADIO_EXPLORER_BLE_SCANNER_H

#include "radio.h"
#include "esp_err.h"

enum { RADIO_BLE_REPORT_QUEUE_DEPTH = 8, RADIO_BLE_AD_SNAPSHOT_MAX = 31 };

bool radio_ble_scanner_start(radio_backend_emit_fn emit, void *emit_context);
void radio_ble_scanner_tick(uint32_t now_seq);
esp_err_t radio_ble_scanner_stop(void);
bool radio_ble_scanner_started(void);
bool radio_ble_scanner_scanning(void);
bool radio_ble_scanner_failed(void);
size_t radio_ble_scanner_queue_bytes(void);
uint32_t radio_ble_scanner_dropped_reports(void);
uint32_t radio_ble_scanner_stack_high_water_bytes(void);
int radio_ble_scanner_error(void);

#endif

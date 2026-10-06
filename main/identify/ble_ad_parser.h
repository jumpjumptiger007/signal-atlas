#ifndef RADIO_EXPLORER_BLE_AD_PARSER_H
#define RADIO_EXPLORER_BLE_AD_PARSER_H

#include "../model/observation.h"

typedef enum { BLE_AD_OK = 0, BLE_AD_MALFORMED, BLE_AD_INVALID_ARGUMENT } ble_ad_status_t;
ble_ad_status_t ble_ad_parse(const uint8_t *bytes, size_t length, radio_ble_data_t *out);

#endif

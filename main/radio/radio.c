#include "radio.h"

#include "../identify/ble_ad_parser.h"
#include <string.h>

radio_wifi_security_t radio_wifi_security_from_auth(uint8_t auth_mode)
{
    switch ((radio_wifi_auth_mode_t)auth_mode) {
    case RADIO_WIFI_AUTH_OPEN: return RADIO_SECURITY_OPEN;
    case RADIO_WIFI_AUTH_WEP: return RADIO_SECURITY_WEP;
    case RADIO_WIFI_AUTH_WPA_PSK: return RADIO_SECURITY_WPA_PSK;
    case RADIO_WIFI_AUTH_WPA2_PSK: return RADIO_SECURITY_WPA2_PSK;
    case RADIO_WIFI_AUTH_WPA_WPA2_PSK: return RADIO_SECURITY_WPA_WPA2_PSK;
    case RADIO_WIFI_AUTH_WPA3_PSK:
    case RADIO_WIFI_AUTH_WPA3_EXT_PSK:
    case RADIO_WIFI_AUTH_WPA3_EXT_PSK_MIXED: return RADIO_SECURITY_WPA3_PSK;
    case RADIO_WIFI_AUTH_WPA2_WPA3_PSK: return RADIO_SECURITY_WPA2_WPA3_PSK;
    case RADIO_WIFI_AUTH_ENTERPRISE:
    case RADIO_WIFI_AUTH_WPA3_ENT_192:
    case RADIO_WIFI_AUTH_WPA_ENTERPRISE:
    case RADIO_WIFI_AUTH_WPA2_WPA3_ENTERPRISE: return RADIO_SECURITY_ENTERPRISE;
    case RADIO_WIFI_AUTH_WAPI_PSK: return RADIO_SECURITY_WAPI_PSK;
    case RADIO_WIFI_AUTH_OWE: return RADIO_SECURITY_OWE;
    case RADIO_WIFI_AUTH_WPA3_ENTERPRISE: return RADIO_SECURITY_WPA3_ENTERPRISE;
    default: return RADIO_SECURITY_UNKNOWN;
    }
}

bool radio_wifi_observation_from_ap(radio_observation_t *out,
                                    const radio_wifi_ap_snapshot_t *snapshot,
                                    uint32_t seen_seq)
{
    if (out == NULL || snapshot == NULL || snapshot->ssid_length > RADIO_WIFI_SSID_MAX) return false;
    radio_observation_t value = {0};
    value.kind = RADIO_KIND_WIFI;
    value.identity.kind = RADIO_KIND_WIFI;
    memcpy(value.identity.address, snapshot->bssid, sizeof(value.identity.address));
    value.rssi = snapshot->rssi;
    value.seen_seq = seen_seq;
    value.data.wifi.ssid_len = snapshot->ssid_length;
    if (snapshot->ssid_length != 0)
        memcpy(value.data.wifi.ssid, snapshot->ssid, snapshot->ssid_length);
    value.data.wifi.hidden = snapshot->ssid_length == 0;
    value.data.wifi.channel = snapshot->channel;
    value.data.wifi.security = (uint8_t)radio_wifi_security_from_auth(snapshot->auth_mode);
    if (!radio_observation_normalize(&value)) return false;
    *out = value;
    return true;
}

bool radio_ble_observation_from_ad(radio_observation_t *out, const uint8_t address[6],
                                   radio_address_type_t address_type, int8_t rssi,
                                   uint32_t seen_seq, const uint8_t *ad, size_t ad_length)
{
    if (out == NULL || address == NULL || ad == NULL || ad_length == 0) return false;
    radio_observation_t value;
    memset(&value, 0, sizeof(value));
    value.kind = RADIO_KIND_BLE;
    value.identity.kind = RADIO_KIND_BLE;
    memcpy(value.identity.address, address, sizeof(value.identity.address));
    value.identity.address_type = (uint8_t)address_type;
    value.rssi = rssi;
    value.seen_seq = seen_seq;
    if (ble_ad_parse(ad, ad_length, &value.data.ble) != BLE_AD_OK) return false;
    if (!radio_observation_normalize(&value)) return false;
    *out = value;
    return true;
}

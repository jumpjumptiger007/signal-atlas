#include "observation.h"

#include <string.h>

bool radio_identity_equal(const radio_identity_t *a, const radio_identity_t *b)
{
    if (a == NULL || b == NULL || a->kind != b->kind ||
        memcmp(a->address, b->address, sizeof(a->address)) != 0) return false;
    return a->kind != RADIO_KIND_BLE || a->address_type == b->address_type;
}

bool radio_mac_is_locally_administered(const uint8_t address[6])
{
    return address != NULL && (address[0] & 0x02u) != 0;
}

bool radio_mac_is_private_or_local(const uint8_t address[6])
{
    return address == NULL || (address[0] & 0x03u) != 0;
}

const char *radio_wifi_security_name(radio_wifi_security_t security)
{
    switch (security) {
    case RADIO_SECURITY_OPEN: return "Open";
    case RADIO_SECURITY_WEP: return "WEP";
    case RADIO_SECURITY_WPA_PSK: return "WPA-PSK";
    case RADIO_SECURITY_WPA2_PSK: return "WPA2-PSK";
    case RADIO_SECURITY_WPA_WPA2_PSK: return "WPA/WPA2-PSK";
    case RADIO_SECURITY_WPA3_PSK: return "WPA3-PSK";
    case RADIO_SECURITY_WPA2_WPA3_PSK: return "WPA2/WPA3-PSK";
    case RADIO_SECURITY_ENTERPRISE: return "Enterprise";
    case RADIO_SECURITY_WAPI_PSK: return "WAPI-PSK";
    case RADIO_SECURITY_OWE: return "OWE";
    case RADIO_SECURITY_WPA3_ENTERPRISE: return "WPA3-Enterprise";
    default: return "Unknown";
    }
}

bool radio_observation_normalize(radio_observation_t *observation)
{
    if (observation == NULL || observation->identity.kind != observation->kind ||
        (observation->kind != RADIO_KIND_WIFI && observation->kind != RADIO_KIND_BLE)) return false;
    if (observation->kind == RADIO_KIND_BLE && observation->identity.address_type > RADIO_ADDRESS_RANDOM) return false;
    if (observation->kind == RADIO_KIND_WIFI) {
        radio_wifi_data_t *wifi = &observation->data.wifi;
        if (wifi->ssid_len > RADIO_WIFI_SSID_MAX || wifi->security > RADIO_SECURITY_UNKNOWN) return false;
        if (wifi->ssid_len == 0) wifi->hidden = true;
    } else {
        radio_ble_data_t *ble = &observation->data.ble;
        if (ble->name_len > RADIO_BLE_NAME_MAX || ble->uuid_count > RADIO_BLE_UUID_MAX ||
            ble->uuid_bytes_len > RADIO_BLE_UUID_BYTES_MAX ||
            ble->manufacturer_len > RADIO_BLE_MANUFACTURER_MAX ||
            ble->service_data_len > RADIO_BLE_SERVICE_DATA_MAX) return false;
    }
    return true;
}

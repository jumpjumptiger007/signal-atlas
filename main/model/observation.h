#ifndef RADIO_EXPLORER_OBSERVATION_H
#define RADIO_EXPLORER_OBSERVATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RADIO_NEARBY_CAPACITY 96u
#define RADIO_WIFI_SSID_MAX 32u
#define RADIO_BLE_NAME_MAX 31u
#define RADIO_BLE_UUID_MAX 6u
#define RADIO_BLE_UUID_BYTES_MAX 32u
#define RADIO_BLE_MANUFACTURER_MAX 24u
#define RADIO_BLE_SERVICE_DATA_MAX 24u

typedef enum { RADIO_KIND_WIFI = 1, RADIO_KIND_BLE = 2 } radio_kind_t;
typedef enum { RADIO_ADDRESS_PUBLIC = 0, RADIO_ADDRESS_RANDOM = 1 } radio_address_type_t;
typedef enum { RADIO_SECURITY_OPEN = 0, RADIO_SECURITY_WEP, RADIO_SECURITY_WPA_PSK,
               RADIO_SECURITY_WPA2_PSK, RADIO_SECURITY_WPA_WPA2_PSK,
               RADIO_SECURITY_WPA3_PSK, RADIO_SECURITY_WPA2_WPA3_PSK,
               RADIO_SECURITY_ENTERPRISE, RADIO_SECURITY_WAPI_PSK,
               RADIO_SECURITY_OWE, RADIO_SECURITY_WPA3_ENTERPRISE,
               RADIO_SECURITY_UNKNOWN } radio_wifi_security_t;

typedef struct {
    uint8_t kind;
    uint8_t address[6];
    uint8_t address_type;
} radio_identity_t;

typedef struct {
    uint8_t ssid_len;
    uint8_t ssid[RADIO_WIFI_SSID_MAX];
    uint8_t channel;
    uint8_t security;
    bool hidden;
} radio_wifi_data_t;

/* UUIDs are stored as length-prefixed little-endian AD bytes in a bounded pool. */
typedef struct {
    uint8_t name_len;
    uint8_t name[RADIO_BLE_NAME_MAX];
    int8_t tx_power;
    uint8_t has_tx_power;
    uint8_t has_company_id;
    uint16_t company_id;
    uint8_t appearance_present;
    uint16_t appearance;
    uint8_t uuid_count;
    uint8_t uuid_truncated;
    uint8_t uuid_bytes_len;
    uint8_t uuid_bytes[RADIO_BLE_UUID_BYTES_MAX];
    uint8_t manufacturer_len;
    uint8_t manufacturer[RADIO_BLE_MANUFACTURER_MAX];
    uint8_t service_data_len;
    uint8_t service_data[RADIO_BLE_SERVICE_DATA_MAX];
} radio_ble_data_t;

typedef struct {
    radio_kind_t kind;
    radio_identity_t identity;
    int8_t rssi;
    uint32_t seen_seq;
    union { radio_wifi_data_t wifi; radio_ble_data_t ble; } data;
} radio_observation_t;

bool radio_identity_equal(const radio_identity_t *a, const radio_identity_t *b);
bool radio_mac_is_locally_administered(const uint8_t address[6]);
bool radio_mac_is_private_or_local(const uint8_t address[6]);
const char *radio_wifi_security_name(radio_wifi_security_t security);
bool radio_observation_normalize(radio_observation_t *observation);

#endif

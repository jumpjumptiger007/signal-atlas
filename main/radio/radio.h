#ifndef RADIO_EXPLORER_RADIO_H
#define RADIO_EXPLORER_RADIO_H

#include "../model/observation.h"

typedef enum { RADIO_BACKEND_EVENT_OBSERVATION = 0, RADIO_BACKEND_EVENT_STATE,
               RADIO_BACKEND_EVENT_ERROR } radio_backend_event_kind_t;
typedef enum { RADIO_BACKEND_STOPPED = 0, RADIO_BACKEND_STARTING,
               RADIO_BACKEND_SCANNING, RADIO_BACKEND_IDLE,
               RADIO_BACKEND_FAILED, RADIO_BACKEND_UNAVAILABLE } radio_backend_state_t;
typedef struct {
    radio_backend_event_kind_t kind;
    radio_observation_t observation;
    radio_backend_state_t state;
    int error;
} radio_backend_event_t;
typedef bool (*radio_backend_emit_fn)(void *context, const radio_backend_event_t *event);

/* Backend contracts are bounded and platform-neutral. Hardware adapters arrive in Gates 3–4. */
typedef struct {
    bool (*start)(void *context, radio_backend_emit_fn emit, void *emit_context);
    void (*stop)(void *context);
    void (*tick)(void *context, uint32_t now_seq);
    void *context;
} radio_backend_t;

/* Numeric values follow the frozen ESP-IDF 5.5.3 wifi_auth_mode_t contract,
 * allowing the driver-to-domain mapping to remain host-testable. */
typedef enum {
    RADIO_WIFI_AUTH_OPEN = 0,
    RADIO_WIFI_AUTH_WEP = 1,
    RADIO_WIFI_AUTH_WPA_PSK = 2,
    RADIO_WIFI_AUTH_WPA2_PSK = 3,
    RADIO_WIFI_AUTH_WPA_WPA2_PSK = 4,
    RADIO_WIFI_AUTH_ENTERPRISE = 5,
    RADIO_WIFI_AUTH_WPA3_PSK = 6,
    RADIO_WIFI_AUTH_WPA2_WPA3_PSK = 7,
    RADIO_WIFI_AUTH_WAPI_PSK = 8,
    RADIO_WIFI_AUTH_OWE = 9,
    RADIO_WIFI_AUTH_WPA3_ENT_192 = 10,
    RADIO_WIFI_AUTH_WPA3_EXT_PSK = 11,
    RADIO_WIFI_AUTH_WPA3_EXT_PSK_MIXED = 12,
    RADIO_WIFI_AUTH_DPP = 13,
    RADIO_WIFI_AUTH_WPA3_ENTERPRISE = 14,
    RADIO_WIFI_AUTH_WPA2_WPA3_ENTERPRISE = 15,
    RADIO_WIFI_AUTH_WPA_ENTERPRISE = 16
} radio_wifi_auth_mode_t;

typedef struct {
    uint8_t ssid[RADIO_WIFI_SSID_MAX];
    uint8_t ssid_length;
    uint8_t bssid[6];
    int8_t rssi;
    uint8_t channel;
    uint8_t auth_mode;
} radio_wifi_ap_snapshot_t;

radio_wifi_security_t radio_wifi_security_from_auth(uint8_t auth_mode);
const char *radio_wifi_security_name(radio_wifi_security_t security);
bool radio_wifi_observation_from_ap(radio_observation_t *out,
                                    const radio_wifi_ap_snapshot_t *snapshot,
                                    uint32_t seen_seq);

bool radio_ble_observation_from_ad(radio_observation_t *out, const uint8_t address[6],
                                   radio_address_type_t address_type, int8_t rssi,
                                   uint32_t seen_seq, const uint8_t *ad, size_t ad_length);

#endif

#include "radio/radio.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_auth_mapping(void)
{
    assert(radio_wifi_security_from_auth(RADIO_WIFI_AUTH_OPEN) == RADIO_SECURITY_OPEN);
    assert(radio_wifi_security_from_auth(RADIO_WIFI_AUTH_WEP) == RADIO_SECURITY_WEP);
    assert(radio_wifi_security_from_auth(RADIO_WIFI_AUTH_WPA_PSK) == RADIO_SECURITY_WPA_PSK);
    assert(radio_wifi_security_from_auth(RADIO_WIFI_AUTH_WPA2_PSK) == RADIO_SECURITY_WPA2_PSK);
    assert(radio_wifi_security_from_auth(RADIO_WIFI_AUTH_WPA_WPA2_PSK) == RADIO_SECURITY_WPA_WPA2_PSK);
    assert(radio_wifi_security_from_auth(RADIO_WIFI_AUTH_WPA3_PSK) == RADIO_SECURITY_WPA3_PSK);
    assert(radio_wifi_security_from_auth(RADIO_WIFI_AUTH_WPA2_WPA3_PSK) == RADIO_SECURITY_WPA2_WPA3_PSK);
    assert(radio_wifi_security_from_auth(RADIO_WIFI_AUTH_ENTERPRISE) == RADIO_SECURITY_ENTERPRISE);
    assert(radio_wifi_security_from_auth(RADIO_WIFI_AUTH_WAPI_PSK) == RADIO_SECURITY_WAPI_PSK);
    assert(radio_wifi_security_from_auth(RADIO_WIFI_AUTH_OWE) == RADIO_SECURITY_OWE);
    assert(radio_wifi_security_from_auth(RADIO_WIFI_AUTH_WPA3_ENTERPRISE) == RADIO_SECURITY_WPA3_ENTERPRISE);
    assert(radio_wifi_security_from_auth(0xff) == RADIO_SECURITY_UNKNOWN);
    assert(strcmp(radio_wifi_security_name(RADIO_SECURITY_WPA2_WPA3_PSK), "WPA2/WPA3-PSK") == 0);
    assert(strcmp(radio_wifi_security_name(RADIO_SECURITY_UNKNOWN), "Unknown") == 0);
}

static void test_ap_observation_mapping(void)
{
    radio_wifi_ap_snapshot_t ap = {0};
    memcpy(ap.ssid, "radio-lab", 9); ap.ssid_length = 9;
    const uint8_t bssid[6] = {0x28,0x6f,0xb9,0x10,0x20,0x30};
    memcpy(ap.bssid, bssid, sizeof(bssid));
    ap.rssi = -47; ap.channel = 11; ap.auth_mode = RADIO_WIFI_AUTH_WPA2_PSK;
    radio_observation_t out;
    assert(radio_wifi_observation_from_ap(&out, &ap, 17));
    assert(out.kind == RADIO_KIND_WIFI && out.identity.kind == RADIO_KIND_WIFI);
    assert(memcmp(out.identity.address, bssid, sizeof(bssid)) == 0);
    assert(out.rssi == -47 && out.seen_seq == 17);
    assert(out.data.wifi.ssid_len == 9 && memcmp(out.data.wifi.ssid, "radio-lab", 9) == 0);
    assert(!out.data.wifi.hidden && out.data.wifi.channel == 11);
    assert(out.data.wifi.security == RADIO_SECURITY_WPA2_PSK);

    memset(&ap, 0, sizeof(ap));
    ap.bssid[0] = 0x02; ap.bssid[5] = 1; ap.rssi = -80; ap.channel = 1;
    assert(radio_wifi_observation_from_ap(&out, &ap, 18));
    assert(out.data.wifi.hidden && out.data.wifi.ssid_len == 0);
    assert(radio_mac_is_locally_administered(out.identity.address));
    assert(out.identity.address[5] == 1);
    ap.ssid_length = RADIO_WIFI_SSID_MAX + 1u;
    assert(!radio_wifi_observation_from_ap(&out, &ap, 19));
    assert(!radio_wifi_observation_from_ap(NULL, &ap, 19));
    assert(!radio_wifi_observation_from_ap(&out, NULL, 19));
}

int main(void)
{
    test_auth_mapping();
    test_ap_observation_mapping();
    printf("Wi-Fi mapper tests: PASS (backend event=%zu bytes × 32=%zu bytes)\n",
           sizeof(radio_backend_event_t), sizeof(radio_backend_event_t) * 32u);
    return 0;
}

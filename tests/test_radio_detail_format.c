#include "ui/detail_format.h"
#include "identify/registry.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void assert_order(const char *text, const char *first, const char *second)
{
    const char *a = strstr(text, first), *b = strstr(text, second);
    assert(a != NULL && b != NULL && a < b);
}

static void test_ibeacon_evidence_order(void)
{
    radio_observation_t o = {0};
    o.kind = RADIO_KIND_BLE; o.identity.kind = RADIO_KIND_BLE;
    o.identity.address_type = RADIO_ADDRESS_PUBLIC;
    const uint8_t address[6] = {0x28,0x6f,0xb9,1,2,3};
    memcpy(o.identity.address, address, sizeof(address)); o.rssi = -51;
    o.data.ble.has_company_id = 1; o.data.ble.company_id = 0x004c;
    const uint8_t frame[] = {0x02,0x15,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
        0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x01,0x23,0x45,0x67,0xfc};
    memcpy(o.data.ble.manufacturer, frame, sizeof(frame));
    o.data.ble.manufacturer_len = sizeof(frame);
    char text[1800];
    assert(radio_detail_format(&o, &o.identity, o.kind, o.rssi, 4, text, sizeof(text)) > 0);
    assert_order(text, "OBSERVED", "RESOLVED");
    assert_order(text, "RESOLVED", "SEEN");
    assert_order(text, "SEEN", "RAW");
    assert(strstr(text, "iBeacon") != NULL);
    assert(strstr(text, "Advertising data owner: Apple, Inc.") != NULL);
    assert(strstr(text, "Company ID: 0x004C (advertising data owner)") != NULL);
    assert(strstr(text, "POSSIBLE") == NULL);
}

static void test_eddystone_and_raw_only_last(void)
{
    radio_observation_t o = {0};
    o.kind = RADIO_KIND_BLE; o.identity.kind = RADIO_KIND_BLE;
    o.identity.address_type = RADIO_ADDRESS_RANDOM; o.identity.address[0] = 0xc2;
    const uint8_t uid[] = {0xaa,0xfe,0x00,0xfc,0x00,0x11,0x22,0x33,0x44,0x55,
        0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x00,0x00};
    memcpy(o.data.ble.service_data, uid, sizeof(uid)); o.data.ble.service_data_len = sizeof(uid);
    char text[1800];
    assert(radio_detail_format(&o, &o.identity, o.kind, -60, 2, text, sizeof(text)) > 0);
    assert_order(text, "OBSERVED", "RESOLVED");
    assert_order(text, "RESOLVED", "SEEN");
    assert_order(text, "SEEN", "RAW");
    assert(strstr(text, "Eddystone UID") != NULL);
    assert(strstr(text, "POSSIBLE") == NULL);
}

static void test_raw_appearance_is_observed(void)
{
    radio_observation_t o = {0};
    o.kind = RADIO_KIND_BLE; o.identity.kind = RADIO_KIND_BLE;
    o.data.ble.appearance_present = 1; o.data.ble.appearance = 0x0040;
    char text[1000];
    assert(radio_detail_format(&o, &o.identity, o.kind, -42, 1, text, sizeof(text)) > 0);
    assert(strstr(text, "Appearance value: 64") != NULL);
    assert(strstr(text, "Appearance name: Phone") != NULL);
    assert_order(text, "OBSERVED", "RESOLVED");
    assert_order(text, "RESOLVED", "SEEN");
    assert(strstr(text, "RAW") != NULL);
}

static void test_history_without_live_observation(void)
{
    radio_identity_t identity = {.kind = RADIO_KIND_WIFI};
    identity.address[0] = 0x28; identity.address[1] = 0x6f; identity.address[2] = 0xb9;
    char text[300];
    assert(radio_detail_format(NULL, &identity, RADIO_KIND_WIFI, -73, 9, text, sizeof(text)) > 0);
    assert_order(text, "OBSERVED", "SEEN");
    assert(strstr(text, "Last RSSI: -73 dBm") != NULL);
    assert(strstr(text, "9 observations") != NULL);
    assert(strstr(text, "RESOLVED") == NULL && strstr(text, "RAW") == NULL);
}

static void test_unresolved_raw_does_not_create_empty_section(void)
{
    radio_observation_t o = {0};
    o.kind = RADIO_KIND_BLE; o.identity.kind = RADIO_KIND_BLE;
    o.data.ble.manufacturer_len = 2;
    o.data.ble.manufacturer[0] = 0xde; o.data.ble.manufacturer[1] = 0xad;
    char text[500];
    assert(radio_detail_format(&o, &o.identity, o.kind, -50, 1, text, sizeof(text)) > 0);
    assert(strstr(text, "RESOLVED") == NULL);
    assert(strstr(text, "RAW") != NULL);
    assert_order(text, "SEEN", "RAW");
}

int main(void)
{
    assert(radio_registry_build_id() != NULL);
    test_ibeacon_evidence_order();
    test_eddystone_and_raw_only_last();
    test_raw_appearance_is_observed();
    test_history_without_live_observation();
    test_unresolved_raw_does_not_create_empty_section();
    puts("Signal Atlas detail formatter tests: PASS");
    return 0;
}

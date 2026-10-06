#include "../main/model/nearby_store.h"
#include "../main/identify/ble_ad_parser.h"
#include "../main/identify/protocol_decoder.h"
#include "../main/identify/registry.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static radio_observation_t wifi(uint8_t last, int8_t rssi, uint32_t seq)
{
    radio_observation_t o = {0};
    o.kind = RADIO_KIND_WIFI; o.identity.kind = RADIO_KIND_WIFI;
    o.identity.address[0] = 0x28; o.identity.address[1] = 0x6f; o.identity.address[2] = 0xb9; o.identity.address[5] = last;
    o.rssi = rssi; o.seen_seq = seq;
    o.data.wifi.ssid_len = 3; memcpy(o.data.wifi.ssid, "lab", 3); o.data.wifi.channel = 6;
    o.data.wifi.security = RADIO_SECURITY_WPA2_PSK;
    return o;
}

static void test_identity_and_normalization(void)
{
    uint8_t global[6] = {0x28, 0x6f, 0xb9, 0, 0, 1};
    uint8_t local[6] = {0x02, 0x6f, 0xb9, 0, 0, 1};
    assert(!radio_mac_is_locally_administered(global));
    assert(radio_mac_is_locally_administered(local));
    assert(!radio_mac_is_private_or_local(global));
    assert(radio_mac_is_private_or_local(local));

    radio_identity_t a = {.kind = RADIO_KIND_BLE, .address_type = RADIO_ADDRESS_PUBLIC};
    radio_identity_t b = a; b.address_type = RADIO_ADDRESS_RANDOM;
    memcpy(a.address, global, 6); memcpy(b.address, global, 6);
    assert(!radio_identity_equal(&a, &b));
    b.address_type = RADIO_ADDRESS_PUBLIC; assert(radio_identity_equal(&a, &b));
    b.address[5]++; assert(!radio_identity_equal(&a, &b));
    a.kind = b.kind = RADIO_KIND_WIFI; assert(radio_identity_equal(&a, &b) == false);

    radio_observation_t hidden = wifi(2, -40, 1);
    hidden.data.wifi.ssid_len = 0;
    assert(radio_observation_normalize(&hidden)); assert(hidden.data.wifi.hidden);
    hidden.data.wifi.ssid_len = RADIO_WIFI_SSID_MAX + 1u;
    assert(!radio_observation_normalize(&hidden));
}

static void test_ad_parser(void)
{
    const uint8_t fields[] = {
        2, 0x08, 's',
        5, 0x09, 'T', 'e', 's', 't',
        2, 0x0a, 0xd8,
        3, 0x19, 0x40, 0x00,
        3, 0xff, 0x4c, 0x00,
        3, 0x03, 0x0f, 0x18,
        0
    };
    radio_ble_data_t out;
    assert(ble_ad_parse(fields, sizeof(fields), &out) == BLE_AD_OK);
    assert(out.name_len == 4 && memcmp(out.name, "Test", 4) == 0);
    assert(out.has_tx_power && out.tx_power == -40);
    assert(out.appearance_present && out.appearance == 0x40);
    assert(out.has_company_id && out.company_id == 0x004c);
    assert(out.uuid_count == 1 && out.uuid_bytes_len == 3 && out.uuid_bytes[0] == 2);
    assert(out.uuid_bytes[1] == 0x0f && out.uuid_bytes[2] == 0x18);
    const uint8_t truncated[] = {4, 0xff, 0x4c};
    assert(ble_ad_parse(truncated, sizeof(truncated), &out) == BLE_AD_MALFORMED);
    assert(ble_ad_parse(NULL, 1, &out) == BLE_AD_INVALID_ARGUMENT);
    assert(ble_ad_parse(NULL, 0, &out) == BLE_AD_OK);
    const uint8_t short_name_only[] = {2, 0x08, 'x', 0};
    assert(ble_ad_parse(short_name_only, sizeof(short_name_only), &out) == BLE_AD_OK);
    assert(out.name_len == 1 && out.name[0] == 'x');
    const uint8_t unknown[] = {2, 0x30, 0xaa, 0};
    assert(ble_ad_parse(unknown, sizeof(unknown), &out) == BLE_AD_OK);
    uint8_t many_uuids[29];
    for (size_t i = 0; i < 7; ++i) {
        many_uuids[i * 4] = 3; many_uuids[i * 4 + 1] = 0x03;
        many_uuids[i * 4 + 2] = (uint8_t)i; many_uuids[i * 4 + 3] = 0x18;
    }
    many_uuids[28] = 0;
    assert(ble_ad_parse(many_uuids, sizeof(many_uuids), &out) == BLE_AD_OK);
    assert(out.uuid_count == RADIO_BLE_UUID_MAX && out.uuid_truncated);
    const uint8_t duplicate_company[] = {3,0xff,0x4c,0x00, 3,0xff,0x01,0x00, 0};
    assert(ble_ad_parse(duplicate_company, sizeof(duplicate_company), &out) == BLE_AD_OK);
    assert(out.company_id == 0x004c); /* First manufacturer field wins deterministically. */
    uint8_t long_name[35] = {33,0x09}; memset(long_name + 2, 'n', 33); long_name[34] = 0;
    assert(ble_ad_parse(long_name, sizeof(long_name), &out) == BLE_AD_OK);
    assert(out.name_len == RADIO_BLE_NAME_MAX);
}

static void test_protocol_decoders(void)
{
    const uint8_t beacon[] = {0x02,0x15,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x01,0x23,0x45,0x67,0xfc};
    radio_decoded_frame_t out;
    assert(radio_decode_ibeacon(0x004c, beacon, sizeof(beacon), &out));
    assert(out.kind == RADIO_FRAME_IBEACON && out.data.ibeacon.major == 0x0123 && out.data.ibeacon.minor == 0x4567);
    assert(out.data.ibeacon.tx_power == -4 && out.data.ibeacon.uuid[15] == 0xff);
    assert(!radio_decode_ibeacon(0x004c, beacon, sizeof(beacon) - 1, &out));
    assert(!radio_decode_ibeacon(0xffff, beacon, sizeof(beacon), &out));

    const uint8_t uid[] = {0x00,0xfc,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,
        0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x00,0x00};
    assert(radio_decode_eddystone(uid, sizeof(uid), &out));
    assert(out.kind == RADIO_FRAME_EDDYSTONE_UID && out.data.uid.tx_power == -4);
    const uint8_t expected_namespace[10] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99};
    const uint8_t expected_instance[6] = {0xaa,0xbb,0xcc,0xdd,0xee,0xff};
    assert(memcmp(out.data.uid.namespace_id, expected_namespace, sizeof(expected_namespace)) == 0);
    assert(memcmp(out.data.uid.instance_id, expected_instance, sizeof(expected_instance)) == 0);
    assert(radio_decode_eddystone(uid, sizeof(uid) - 2, &out)); /* Donor-supported truncated UID frame. */
    assert(!radio_decode_eddystone(uid, sizeof(uid) - 3, &out));
    const uint8_t url[] = {0xaa,0xfe,0x10,0xfc,0x03,'g','e','t','p','a','r','e','t','o',0x07};
    assert(radio_decode_eddystone(url, sizeof(url), &out));
    assert(out.kind == RADIO_FRAME_EDDYSTONE_URL && strcmp(out.data.url.value, "https://getpareto.com") == 0);
    assert(out.data.url.tx_power == -4);
    const uint8_t tlm[] = {0x20,0x00,0x0b,0xb8,0x15,0x00,0x00,0x00,0x00,0x45,0x00,0x00,0x02,0x58};
    assert(radio_decode_eddystone(tlm, sizeof(tlm), &out));
    assert(out.kind == RADIO_FRAME_EDDYSTONE_TLM && out.data.tlm.battery_mv == 3000 && out.data.tlm.adv_count == 69);
    assert(out.data.tlm.uptime_100ms == 600);
    uint8_t bad_url[sizeof(url)]; memcpy(bad_url, url, sizeof(url)); bad_url[4] = 0x7f;
    assert(!radio_decode_eddystone(bad_url, sizeof(bad_url), &out));
    const uint8_t unsupported[] = {0xfe,0xaa,0x30,0xfc,0,0,0,0,0,0,0,0};
    assert(!radio_decode_eddystone(unsupported, sizeof(unsupported), &out));
    assert(!radio_decode_eddystone(tlm, 1, &out));
}

static void test_parser_to_protocol_integration(void)
{
    radio_ble_data_t ble;
    radio_decoded_frame_t decoded;
    const uint8_t beacon_ad[] = {26,0xff,0x4c,0x00,
        0x02,0x15,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x01,0x23,0x45,0x67,0xfc,0};
    assert(ble_ad_parse(beacon_ad, sizeof(beacon_ad), &ble) == BLE_AD_OK);
    assert(ble.manufacturer_len == 23);
    assert(radio_decode_ibeacon(ble.company_id, ble.manufacturer, ble.manufacturer_len, &decoded));
    assert(decoded.data.ibeacon.major == 0x0123 && decoded.data.ibeacon.minor == 0x4567);

    const uint8_t uid_ad[] = {23,0x16,0xaa,0xfe,
        0x00,0xfc,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,
        0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x00,0x00,0};
    assert(ble_ad_parse(uid_ad, sizeof(uid_ad), &ble) == BLE_AD_OK);
    assert(ble.service_data_len == 22 && ble.uuid_count == 1);
    assert(ble.service_data[0] == 0xaa && ble.service_data[1] == 0xfe);
    assert(radio_decode_eddystone(ble.service_data, ble.service_data_len, &decoded));
    assert(decoded.data.uid.namespace_id[0] == 0x00 && decoded.data.uid.instance_id[0] == 0xaa);

    const uint8_t tlm_ad[] = {17,0x16,0xaa,0xfe,
        0x20,0x00,0x0b,0xb8,0x15,0x00,0x00,0x00,0x00,0x45,0x00,0x00,0x02,0x58,0};
    assert(ble_ad_parse(tlm_ad, sizeof(tlm_ad), &ble) == BLE_AD_OK);
    assert(ble.service_data_len == 16 && radio_decode_eddystone(ble.service_data, ble.service_data_len, &decoded));
    assert(decoded.data.tlm.uptime_100ms == 600);

    const uint8_t service_data_before_eddystone[] = {4,0x16,0x0f,0x18,0xaa,
        17,0x16,0xaa,0xfe,0x20,0x00,0x0b,0xb8,0x15,0x00,0x00,0x00,0x00,0x45,0x00,0x00,0x02,0x58,0};
    assert(ble_ad_parse(service_data_before_eddystone, sizeof(service_data_before_eddystone), &ble) == BLE_AD_OK);
    assert(ble.service_data[0] == 0xaa && ble.service_data[1] == 0xfe);
    assert(radio_decode_eddystone(ble.service_data, ble.service_data_len, &decoded));
    assert(decoded.kind == RADIO_FRAME_EDDYSTONE_TLM);
}

static void test_nearby_store(void)
{
    nearby_store_t store;
    nearby_store_init(&store, 10);
    radio_observation_t a = wifi(1, -50, 1), b = wifi(2, -60, 2);
    assert(nearby_store_update(&store, &a, 1, 5, 20));
    assert(nearby_store_update(&store, &b, 2, 5, 20));
    assert(store.count == 2 && store.records[0].observation.identity.address[5] == 2);
    assert(nearby_store_update(&store, &a, 3, 5, 20));
    assert(store.count == 2 && store.records[1].observations == 2);
    assert(store.records[1].observation.rssi == -50);
    nearby_store_select(&store, 0, 3);
    radio_identity_t selected = store.records[0].observation.identity;
    assert(nearby_store_update(&store, &a, 4, 5, 20));
    assert(store.order_frozen && radio_identity_equal(&selected, &store.records[0].observation.identity));
    assert(nearby_store_update(&store, &b, 15, 5, 20));
    assert(!store.order_frozen && store.selected_index >= 0);
    assert(radio_identity_equal(&selected, &store.records[store.selected_index].observation.identity));
    assert(nearby_store_expire(&store, 40, 20) == 1); /* selected item is protected */
    assert(store.count == 1 && store.records[store.selected_index].occupied);
    nearby_store_leave_list(&store);

    nearby_store_init(&store, 10);
    for (uint8_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i) {
        radio_observation_t item = wifi(i, (int8_t)(-30 - (i % 60)), i + 1);
        item.identity.address[3] = (uint8_t)(i + 1);
        assert(nearby_store_update(&store, &item, i + 1, 1000, 2000));
    }
    assert(store.count == RADIO_NEARBY_CAPACITY);
    nearby_store_select(&store, 0, 100);
    radio_identity_t protected_id = store.records[0].observation.identity;
    radio_observation_t newest = wifi(250, -90, 101); newest.identity.address[3] = 0xfe;
    assert(nearby_store_update(&store, &newest, 101, 1000, 2000));
    assert(store.count == RADIO_NEARBY_CAPACITY);
    bool still_present = false;
    for (size_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i)
        if (store.records[i].occupied && radio_identity_equal(&protected_id, &store.records[i].observation.identity)) still_present = true;
    assert(still_present);
    bool oldest_present = false;
    radio_identity_t oldest = wifi(0, -30, 1).identity; oldest.address[3] = 1;
    for (size_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i)
        if (store.records[i].occupied && radio_identity_equal(&oldest, &store.records[i].observation.identity)) oldest_present = true;
    assert(!oldest_present); /* Oldest eligible entry is evicted, selected entry is protected. */

    /* A fresh candidate survives overflow ahead of stale records. */
    nearby_store_init(&store, 10);
    radio_identity_t fresh_id = {0};
    radio_identity_t stale_id = {0};
    for (uint8_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i) {
        radio_observation_t item = wifi(i, -45, i + 1);
        item.identity.address[3] = (uint8_t)(i + 1);
        if (i == 95) fresh_id = item.identity;
        if (i == 0) stale_id = item.identity;
        assert(nearby_store_update(&store, &item, i + 1, 1000, 2000));
    }
    radio_observation_t fresh = wifi(95, -35, 200); fresh.identity.address[3] = 96;
    assert(nearby_store_update(&store, &fresh, 200, 1000, 2000));
    nearby_store_select(&store, 1, 200);
    radio_identity_t protected_stale = store.records[1].observation.identity;
    radio_observation_t replacement = wifi(250, -90, 201); replacement.identity.address[3] = 0xfe;
    assert(nearby_store_update(&store, &replacement, 201, 50, 1000));
    bool fresh_present = false, stale_present = false, protected_present = false;
    for (size_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i) if (store.records[i].occupied) {
        fresh_present |= radio_identity_equal(&fresh_id, &store.records[i].observation.identity);
        stale_present |= radio_identity_equal(&stale_id, &store.records[i].observation.identity);
        protected_present |= radio_identity_equal(&protected_stale, &store.records[i].observation.identity);
    }
    assert(fresh_present && !stale_present && protected_present);

    /* Timeout must resume sorting before inserting a new identity. */
    nearby_store_init(&store, 5);
    a = wifi(1, -70, 1); b = wifi(2, -60, 2);
    assert(nearby_store_update(&store, &a, 1, 10, 20));
    assert(nearby_store_update(&store, &b, 2, 10, 20));
    nearby_store_select(&store, 0, 3);
    selected = store.records[0].observation.identity;
    radio_observation_t newcomer = wifi(3, -20, 20);
    assert(nearby_store_update(&store, &newcomer, 20, 10, 40));
    assert(!store.order_frozen && store.selected_index >= 0);
    assert(radio_identity_equal(&selected, &store.records[store.selected_index].observation.identity));
    assert(radio_identity_equal(&newcomer.identity, &store.records[0].observation.identity));

    nearby_store_init(&store, 5);
    radio_observation_t wrapped = wifi(7, -40, UINT32_MAX - 1u);
    assert(nearby_store_update(&store, &wrapped, UINT32_MAX - 1u, 10, 20));
    assert(nearby_store_expire(&store, 2, 5) == 0);
    assert(nearby_store_expire(&store, 3, 5) == 1);
}

static void test_registries(void)
{
    uint8_t bits = 0;
    const uint8_t ma_l[] = {0x28,0x6f,0xb9,0x01,0x02,0x03};
    const uint8_t ma_m[] = {0xc8,0x5c,0xe2,0x7f,0x12,0x34};
    const uint8_t ma_s[] = {0x8c,0x1f,0x64,0xaf,0xa5,0x67};
    const uint8_t local[] = {0x02,0x6f,0xb9,0,0,1};
    assert(radio_ieee_lookup(ma_l, &bits) == NULL);
    assert(radio_ieee_lookup(ma_m, &bits) == NULL);
    assert(radio_ieee_lookup(ma_s, &bits) == NULL);
    assert(radio_ieee_lookup(local, &bits) == NULL);
    assert(strcmp(radio_company_lookup(0x004c), "Apple, Inc.") == 0);
    assert(radio_company_lookup(0xfffe) == NULL);
    uint8_t uuid[16]; radio_service_uuid16_expand(0x180f, uuid);
    assert(strcmp(radio_service_lookup(uuid), "Battery Service") == 0);
    assert(strcmp(radio_appearance_lookup(0x0040), "Phone") == 0);
    assert(radio_registry_ieee_record_count() == 0);
    assert(radio_registry_bluetooth_record_count() > 4000);
    assert(strlen(radio_registry_build_id()) == 12);
}

int main(void)
{
    test_identity_and_normalization(); test_ad_parser(); test_protocol_decoders(); test_parser_to_protocol_integration();
    test_nearby_store(); test_registries();
    printf("Nearby record=%zu bytes capacity=%u store=%zu bytes\n", sizeof(nearby_record_t),
           (unsigned)RADIO_NEARBY_CAPACITY, sizeof(nearby_store_t));
    puts("Signal Atlas domain tests: PASS");
    return 0;
}

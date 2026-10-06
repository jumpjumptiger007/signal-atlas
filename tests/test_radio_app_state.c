#include "app_state.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static radio_observation_t observation(uint8_t tail, int8_t rssi)
{
    radio_observation_t o;
    memset(&o, 0, sizeof(o));
    o.kind = RADIO_KIND_BLE;
    o.identity.kind = RADIO_KIND_BLE;
    o.identity.address_type = RADIO_ADDRESS_RANDOM;
    o.identity.address[0] = 0xC2;
    o.identity.address[5] = tail;
    o.rssi = rssi;
    return o;
}

static radio_observation_t wifi_observation(uint8_t tail, int8_t rssi)
{
    radio_observation_t o;
    memset(&o, 0, sizeof(o));
    o.kind = RADIO_KIND_WIFI;
    o.identity.kind = RADIO_KIND_WIFI;
    o.identity.address[0] = 0xA0;
    o.identity.address[5] = tail;
    o.rssi = rssi;
    o.data.wifi.ssid_len = 4;
    memcpy(o.data.wifi.ssid, "Test", 4);
    return o;
}

static void press_n(re_app_state_t *s, re_button_t key, unsigned n)
{
    while (n--) re_app_state_handle_button(s, key);
}

int main(void)
{
    re_app_state_t s;
    re_app_state_init(&s, 3);
    re_app_state_seed_history(&s);
    assert(s.screen == RE_SCREEN_NEARBY && s.history_count == 1);
    assert(re_app_state_radar_active(&s) == false);
    re_app_state_set_backend(&s, RE_BACKEND_SCANNING);
    assert(re_app_state_radar_active(&s));

    radio_observation_t a = observation(2, -60);
    re_app_state_observe(&s, &a, 10, 30);
    assert(s.nearby.count == 1 && s.history_count == 2);
    assert(re_app_state_is_new(&s, &a.identity));
    const uint32_t seq = s.sequence;
    a.rssi = -45;
    re_app_state_observe(&s, &a, 10, 30);
    assert(s.nearby.count == 1 && s.nearby.records[0].observations == 2);
    assert(s.nearby.records[0].observation.rssi == -45);
    assert(s.history_count == 2 && s.history[1].observations == 2);
    assert(s.sequence == seq + 1);
    radio_observation_t b = observation(3, -48);
    re_app_state_observe(&s, &b, 10, 30);
    assert(s.nearby.count == 2 && s.history_count == 3);
    re_app_state_handle_button(&s, RE_BUTTON_DOWN);
    assert(s.nearby.order_frozen && s.nearby.selected_index == s.nearby_selection);
    const radio_identity_t selected = s.nearby.records[s.nearby_selection].observation.identity;
    b.rssi = -35;
    re_app_state_observe(&s, &b, 10, 30);
    assert(s.nearby.order_frozen);
    assert(radio_identity_equal(&selected, &s.nearby.records[s.nearby_selection].observation.identity));

    re_app_state_handle_button(&s, RE_BUTTON_OK_CLICK);
    assert(s.screen == RE_SCREEN_DETAIL && s.detail_source == RE_DETAIL_NEARBY);
    assert(!re_app_state_radar_active(&s));
    re_app_state_handle_button(&s, RE_BUTTON_OK_LONG);
    assert(s.screen == RE_SCREEN_NEARBY && re_app_state_radar_active(&s));

    re_app_state_handle_button(&s, RE_BUTTON_OK_LONG);
    assert(s.modal == RE_MODAL_QUICK_MENU && s.modal_selection == 0);
    press_n(&s, RE_BUTTON_DOWN, 1);
    re_app_state_handle_button(&s, RE_BUTTON_OK_CLICK);
    assert(s.screen == RE_SCREEN_HISTORY && s.modal == RE_MODAL_NONE);
    re_app_state_handle_button(&s, RE_BUTTON_OK_CLICK);
    assert(s.screen == RE_SCREEN_DETAIL && s.detail_source == RE_DETAIL_HISTORY);
    re_app_state_handle_button(&s, RE_BUTTON_OK_LONG);
    assert(s.screen == RE_SCREEN_HISTORY);
    re_app_state_handle_button(&s, RE_BUTTON_OK_LONG);
    assert(s.screen == RE_SCREEN_NEARBY);

    re_app_state_handle_button(&s, RE_BUTTON_OK_LONG);
    press_n(&s, RE_BUTTON_DOWN, 3);
    assert(s.modal_selection == 3);
    re_app_state_handle_button(&s, RE_BUTTON_OK_CLICK);
    assert(s.modal == RE_MODAL_CLEAR_HISTORY && s.modal_selection == 1);
    re_app_state_handle_button(&s, RE_BUTTON_OK_CLICK);
    assert(s.history_count == 3 && s.nearby.count == 2);

    re_app_state_handle_button(&s, RE_BUTTON_OK_LONG);
    press_n(&s, RE_BUTTON_DOWN, 3);
    re_app_state_handle_button(&s, RE_BUTTON_OK_CLICK);
    assert(s.modal == RE_MODAL_CLEAR_HISTORY);
    re_app_state_handle_button(&s, RE_BUTTON_UP);
    assert(s.modal_selection == 0);
    re_app_state_handle_button(&s, RE_BUTTON_OK_CLICK);
    assert(s.history_count == 0 && s.nearby.count == 2);

    re_app_state_handle_button(&s, RE_BUTTON_OK_LONG);
    press_n(&s, RE_BUTTON_UP, 1);
    assert(s.modal_selection == 4);
    re_app_state_handle_button(&s, RE_BUTTON_OK_CLICK);
    assert(s.modal == RE_MODAL_ABOUT);
    re_app_state_handle_button(&s, RE_BUTTON_OK_LONG);
    assert(s.modal == RE_MODAL_NONE);

    re_app_state_handle_button(&s, RE_BUTTON_OK_LONG);
    re_app_state_handle_button(&s, RE_BUTTON_OK_CLICK);
    assert(s.modal == RE_MODAL_FILTER && s.modal_selection == RE_MODE_ALL);
    press_n(&s, RE_BUTTON_DOWN, 1);
    re_app_state_handle_button(&s, RE_BUTTON_OK_CLICK);
    assert(s.mode == RE_MODE_WIFI && s.backend == RE_BACKEND_UNAVAILABLE);
    assert(!re_app_state_has_visible_nearby(&s));
    assert(s.nearby.count == 2); /* Filter hides BLE; it does not fabricate/erase data. */
    assert(!re_app_state_radar_active(&s));

    re_app_state_set_mode(&s, RE_MODE_BLE);
    re_app_state_set_backend(&s, RE_BACKEND_SCANNING);
    assert(re_app_state_has_visible_nearby(&s) && re_app_state_radar_active(&s));
    re_app_state_toggle_pause(&s);
    assert(s.backend == RE_BACKEND_IDLE && !re_app_state_radar_active(&s));

    re_app_state_handle_button(&s, RE_BUTTON_OK_LONG);
    press_n(&s, RE_BUTTON_DOWN, 1); /* History */
    re_app_state_handle_button(&s, RE_BUTTON_OK_CLICK);
    assert(s.screen == RE_SCREEN_HISTORY && s.history_count == 0);
    assert(!re_app_state_radar_active(&s));

    re_app_state_t overflow;
    re_app_state_init(&overflow, 5);
    for (uint8_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i) {
        radio_observation_t unique = observation(i, -40);
        unique.identity.address[4] = (uint8_t)(i >> 8);
        re_app_state_observe(&overflow, &unique, 10, 30);
    }
    assert(overflow.nearby.count == RADIO_NEARBY_CAPACITY);
    radio_observation_t extra = observation(0xF0, -50);
    extra.identity.address[4] = 0x7F;
    re_app_state_observe(&overflow, &extra, 10, 30);
    assert(overflow.nearby.count == RADIO_NEARBY_CAPACITY && overflow.overflow_seen);
    re_app_state_t empty;
    re_app_state_init(&empty, 5);
    re_app_state_set_backend(&empty, RE_BACKEND_SCANNING);
    assert(!re_app_state_has_visible_nearby(&empty) && re_app_state_radar_active(&empty));
    re_app_state_set_backend(&empty, RE_BACKEND_FAILED);
    assert(!re_app_state_radar_active(&empty));

    re_app_state_t many;
    re_app_state_init(&many, 10);
    for (uint8_t i = 0; i < 10; ++i) {
        radio_observation_t unique = observation((uint8_t)(0x30 + i), -40);
        re_app_state_observe(&many, &unique, 10, 30);
    }
    assert(re_app_state_nearby_visible_count(&many) == 10);
    press_n(&many, RE_BUTTON_DOWN, 7);
    size_t selected_rank = re_app_state_nearby_visible_rank(&many, many.nearby_selection);
    assert(selected_rank == 7);
    int nearby_window[6];
    assert(re_app_state_nearby_window(&many, 6, nearby_window) == 6);
    bool nearby_selected_visible = false;
    for (size_t i = 0; i < 6; ++i) nearby_selected_visible |= nearby_window[i] == many.nearby_selection;
    assert(nearby_selected_visible);

    re_app_state_t mixed;
    re_app_state_init(&mixed, 10);
    radio_observation_t wifi = wifi_observation(1, -50);
    radio_observation_t ble = observation(1, -60);
    re_app_state_observe(&mixed, &wifi, 10, 30);
    re_app_state_observe(&mixed, &ble, 10, 30);
    assert(re_app_state_nearby_visible_count(&mixed) == 2);
    re_app_state_set_mode(&mixed, RE_MODE_WIFI);
    assert(re_app_state_nearby_visible_count(&mixed) == 1);
    assert(mixed.nearby.records[mixed.nearby_selection].observation.kind == RADIO_KIND_WIFI);
    mixed.nearby_selection = (mixed.nearby.records[0].observation.kind == RADIO_KIND_BLE) ? 0 : 1;
    re_app_state_handle_button(&mixed, RE_BUTTON_OK_CLICK);
    assert(mixed.screen == RE_SCREEN_NEARBY); /* Hidden BLE cannot open Detail under WI-FI filter. */
    re_app_state_set_mode(&mixed, RE_MODE_BLE);
    assert(re_app_state_nearby_visible_count(&mixed) == 1);
    assert(mixed.nearby.records[mixed.nearby_selection].observation.kind == RADIO_KIND_BLE);

    re_app_state_t history;
    re_app_state_init(&history, 10);
    for (uint8_t i = 0; i < 8; ++i) {
        radio_observation_t unique = observation((uint8_t)(0x60 + i), -40);
        re_app_state_observe(&history, &unique, 10, 30);
    }
    int history_order[RE_HISTORY_VOLATILE_CAPACITY];
    assert(re_app_state_history_order(&history, history_order, RE_HISTORY_VOLATILE_CAPACITY) == 8);
    assert(history_order[0] == 7 && history_order[7] == 0);
    history.screen = RE_SCREEN_HISTORY;
    history.history_selection = history_order[0];
    re_app_state_handle_button(&history, RE_BUTTON_DOWN);
    assert(history.history_selection == history_order[1]);
    history.history_selection = history_order[7];
    int history_window[6];
    assert(re_app_state_history_window(&history, 6, history_window) == 6);
    bool history_selected_visible = false;
    for (size_t i = 0; i < 6; ++i) history_selected_visible |= history_window[i] == history.history_selection;
    assert(history_selected_visible && history_window[5] == history_order[7]);

    re_app_state_t full_history;
    re_app_state_init(&full_history, 10);
    for (uint16_t i = 0; i < RADIO_HISTORY_CAPACITY; ++i) {
        radio_observation_t unique = observation((uint8_t)i, -55);
        unique.identity.address[1] = (uint8_t)(i >> 8);
        unique.identity.address[2] = (uint8_t)(i >> 16);
        re_app_state_observe(&full_history, &unique, 10, 30);
    }
    assert(full_history.history_count == RADIO_HISTORY_CAPACITY);
    static radio_history_record_t restored_records[RADIO_HISTORY_CAPACITY];
    memcpy(restored_records, full_history.history, sizeof(restored_records));
    re_app_state_t restored_history;
    re_app_state_init(&restored_history, 10);
    re_app_state_restore_history(&restored_history, restored_records,
        RADIO_HISTORY_CAPACITY, 17, 4);
    assert(restored_history.history_count == RADIO_HISTORY_CAPACITY);
    assert(restored_history.session_id == 18);
    assert(!re_app_state_is_new(&restored_history, &restored_records[100].identity));
    re_app_state_clear_history(&restored_history);
    assert(restored_history.history_count == 0 && restored_history.history_dirty &&
           restored_history.history_flush_now);

    printf("History record RAM=%zu bytes capacity=%u table=%zu bytes serialized=%u bytes\n",
           sizeof(radio_history_record_t), (unsigned)RADIO_HISTORY_CAPACITY,
           sizeof(radio_history_record_t) * RADIO_HISTORY_CAPACITY,
           (unsigned)RADIO_HISTORY_BLOB_BYTES);
    puts("Signal Atlas app-state tests: PASS");
    return 0;
}

#include "app_state.h"

#include <string.h>

static bool matches_mode(const nearby_record_t *record, re_mode_t mode)
{
    return record->occupied && (mode == RE_MODE_ALL ||
        (mode == RE_MODE_WIFI && record->observation.kind == RADIO_KIND_WIFI) ||
        (mode == RE_MODE_BLE && record->observation.kind == RADIO_KIND_BLE));
}

static int history_find(const re_app_state_t *state, const radio_identity_t *identity)
{
    for (size_t i = 0; i < RE_HISTORY_VOLATILE_CAPACITY; ++i)
        if (state->history[i].occupied && radio_identity_equal(&state->history[i].identity, identity))
            return (int)i;
    return -1;
}

static bool history_precedes(const radio_history_record_t *a, size_t a_index,
                             const radio_history_record_t *b, size_t b_index)
{
    const int32_t delta = (int32_t)(a->last_seen_seq - b->last_seen_seq);
    return delta > 0 || (delta == 0 && a_index < b_index);
}

static int history_index_at_rank(const re_app_state_t *state, size_t wanted_rank)
{
    for (size_t i = 0; i < RE_HISTORY_VOLATILE_CAPACITY; ++i) {
        if (!state->history[i].occupied) continue;
        size_t rank = 0;
        for (size_t j = 0; j < RE_HISTORY_VOLATILE_CAPACITY; ++j)
            if (state->history[j].occupied &&
                history_precedes(&state->history[j], j, &state->history[i], i)) ++rank;
        if (rank == wanted_rank) return (int)i;
    }
    return -1;
}

static bool nearby_has_identity(const nearby_store_t *store, const radio_identity_t *identity)
{
    for (size_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i)
        if (store->records[i].occupied && radio_identity_equal(&store->records[i].observation.identity, identity)) return true;
    return false;
}

void re_app_state_init(re_app_state_t *state, uint32_t freeze_timeout)
{
    if (state == NULL) return;
    memset(state, 0, sizeof(*state));
    nearby_store_init(&state->nearby, freeze_timeout);
    state->screen = RE_SCREEN_NEARBY;
    state->mode = RE_MODE_ALL;
    state->backend = RE_BACKEND_STOPPED;
    state->session_id = 1;
    state->nearby_selection = -1;
    state->history_selection = -1;
    state->dirty = true;
}

void re_app_state_set_backend(re_app_state_t *state, re_backend_state_t backend)
{
    if (state == NULL) return;
    state->backend = backend;
    state->dirty = true;
}

void re_app_state_set_mode(re_app_state_t *state, re_mode_t mode)
{
    if (state == NULL || mode > RE_MODE_BLE) return;
    state->mode = mode;
    if (re_app_state_nearby_visible_rank(state, state->nearby_selection) == SIZE_MAX) {
        nearby_store_leave_list(&state->nearby);
        state->nearby_selection = re_app_state_nearby_visible_at(state, 0);
        if (state->nearby_selection >= 0)
            nearby_store_select(&state->nearby, state->nearby_selection, state->sequence);
    }
    state->dirty = true;
}

void re_app_state_toggle_pause(re_app_state_t *state)
{
    if (state == NULL) return;
    state->scan_paused = !state->scan_paused;
    state->backend = state->scan_paused ? RE_BACKEND_IDLE : RE_BACKEND_SCANNING;
    state->dirty = true;
}

void re_app_state_observe(re_app_state_t *state, const radio_observation_t *observation,
                          uint32_t stale_after, uint32_t expire_after)
{
    if (state == NULL || observation == NULL) return;
    ++state->sequence;
    radio_observation_t value = *observation;
    value.seen_seq = state->sequence;
    const bool was_known = nearby_has_identity(&state->nearby, &value.identity);
    if (!was_known && state->nearby.count == RADIO_NEARBY_CAPACITY) state->overflow_seen = true;
    (void)nearby_store_update(&state->nearby, &value, state->sequence, stale_after, expire_after);
    int i = history_find(state, &value.identity);
    if (i < 0) {
        for (size_t j = 0; j < RE_HISTORY_VOLATILE_CAPACITY; ++j)
            if (!state->history[j].occupied) { i = (int)j; break; }
        if (i < 0) {
            i = -1;
            for (size_t j = 0; j < RE_HISTORY_VOLATILE_CAPACITY; ++j) {
                if ((int)j == state->history_selection) continue;
                if (i < 0 || (int32_t)(state->history[j].last_seen_seq -
                                      state->history[i].last_seen_seq) < 0) i = (int)j;
            }
            if (i < 0) i = state->history_selection >= 0 ? state->history_selection : 0;
        } else ++state->history_count;
        memset(&state->history[i], 0, sizeof(state->history[i]));
        state->history[i].occupied = 1;
        state->history[i].identity = value.identity;
        state->history[i].first_session = state->session_id;
        state->history[i].last_session = state->session_id;
        state->history[i].observations = 1;
        state->history[i].was_present_at_start = 0;
    } else if (state->history[i].observations != UINT32_MAX) {
        ++state->history[i].observations;
    }
    state->history[i].last_rssi = value.rssi;
    state->history[i].last_seen_seq = state->sequence;
    state->history[i].last_session = state->session_id;
    state->history_dirty = true;
    if (state->screen == RE_SCREEN_NEARBY) {
        if (state->nearby.order_frozen) state->nearby_selection = state->nearby.selected_index;
        else if (state->nearby_selection < 0 || re_app_state_nearby_visible_rank(state, state->nearby_selection) == SIZE_MAX)
            state->nearby_selection = re_app_state_nearby_visible_at(state, 0);
    }
    state->dirty = true;
}

void re_app_state_seed_history(re_app_state_t *state)
{
    if (state == NULL || state->history_seeded) return;
    /* A deterministic locally remembered sample: not a date or a radio claim. */
    radio_history_record_t *h = &state->history[0];
    memset(h, 0, sizeof(*h));
    h->occupied = 1;
    h->identity.kind = RADIO_KIND_BLE;
    h->identity.address_type = RADIO_ADDRESS_RANDOM;
    const uint8_t address[6] = { 0xC2, 0x10, 0x00, 0x00, 0x00, 0x01 };
    memcpy(h->identity.address, address, sizeof(address));
    h->last_rssi = -64;
    h->observations = 3;
    h->last_seen_seq = 0;
    h->first_session = state->session_id;
    h->last_session = state->session_id;
    h->was_present_at_start = 1;
    state->history_count = 1;
    state->history_seeded = true;
    state->dirty = true;
    state->history_dirty = true;
}

void re_app_state_restore_history(re_app_state_t *state,
                                  const radio_history_record_t *records,
                                  size_t count, uint32_t session_id,
                                  uint32_t generation)
{
    if (state == NULL || (records == NULL && count != 0)) return;
    if (count > RE_HISTORY_VOLATILE_CAPACITY) count = RE_HISTORY_VOLATILE_CAPACITY;
    if (records != state->history) {
        memset(state->history, 0, sizeof(state->history));
        if (count != 0) memcpy(state->history, records, count * sizeof(*records));
    }
    state->history_count = count;
    for (size_t i = 0; i < count; ++i) {
        state->history[i].occupied = 1;
        state->history[i].was_present_at_start = 1;
        state->history[i].last_seen_seq = 0;
    }
    state->session_id = session_id + 1u;
    if (state->session_id == 0) state->session_id = 1;
    state->history_generation = generation;
    state->history_seeded = count != 0;
    state->history_dirty = true;
    state->dirty = true;
}

void re_app_state_clear_history(re_app_state_t *state)
{
    if (state == NULL) return;
    memset(state->history, 0, sizeof(state->history));
    state->history_count = 0;
    state->history_seeded = false;
    state->history_selection = -1;
    state->history_generation++;
    state->history_dirty = true;
    state->history_flush_now = true;
    state->dirty = true;
}

static void open_modal(re_app_state_t *state, re_modal_t modal)
{
    state->modal = modal;
    state->modal_return_screen = state->screen;
    state->modal_selection = modal == RE_MODAL_CLEAR_HISTORY ? 1 : 0;
    state->dirty = true;
}

static void close_modal(re_app_state_t *state)
{
    state->screen = state->modal_return_screen;
    state->modal = RE_MODAL_NONE;
    state->modal_selection = 0;
    state->dirty = true;
}

static void confirm_modal(re_app_state_t *state)
{
    if (state->modal == RE_MODAL_QUICK_MENU) {
        switch (state->modal_selection) {
        case 0: state->modal = RE_MODAL_FILTER; state->modal_selection = (int)state->mode; break;
        case 1: {
            size_t count = re_app_state_history_order(state, NULL, 0);
            state->modal = RE_MODAL_NONE;
            state->screen = RE_SCREEN_HISTORY;
            state->history_selection = count ? history_index_at_rank(state, 0) : -1;
            break;
        }
        case 2: re_app_state_toggle_pause(state); state->modal = RE_MODAL_NONE; break;
        case 3: state->modal = RE_MODAL_CLEAR_HISTORY; state->modal_selection = 1; break;
        default: state->modal = RE_MODAL_ABOUT; state->modal_selection = 0; break;
        }
    } else if (state->modal == RE_MODAL_FILTER) {
        re_app_state_set_mode(state, (re_mode_t)state->modal_selection);
        state->modal = RE_MODAL_NONE;
        if (state->mode == RE_MODE_WIFI) state->backend = RE_BACKEND_UNAVAILABLE;
        else state->backend = state->scan_paused ? RE_BACKEND_IDLE : RE_BACKEND_SCANNING;
    } else if (state->modal == RE_MODAL_CLEAR_HISTORY) {
        if (state->modal_selection == 0) re_app_state_clear_history(state);
        state->modal = RE_MODAL_NONE;
        state->screen = state->modal_return_screen;
    } else {
        close_modal(state);
        return;
    }
    state->dirty = true;
}

static void move_selection(re_app_state_t *state, int delta)
{
    if (state->modal != RE_MODAL_NONE) {
        int max = state->modal == RE_MODAL_QUICK_MENU ? 4 :
                  state->modal == RE_MODAL_FILTER ? 2 :
                  state->modal == RE_MODAL_CLEAR_HISTORY ? 1 : 0;
        if (max > 0) {
            state->modal_selection = (state->modal_selection + (delta > 0 ? 1 : max)) % (max + 1);
            state->dirty = true;
        }
        return;
    }
    if (state->screen == RE_SCREEN_NEARBY) {
        size_t count = re_app_state_nearby_visible_count(state);
        if (count == 0) state->nearby_selection = -1;
        else {
            size_t rank = re_app_state_nearby_visible_rank(state, state->nearby_selection);
            if (rank == SIZE_MAX) rank = delta > 0 ? count - 1 : 0;
            rank = (rank + (delta > 0 ? 1 : count - 1)) % count;
            state->nearby_selection = re_app_state_nearby_visible_at(state, rank);
        }
        if (state->nearby_selection >= 0)
            nearby_store_select(&state->nearby, state->nearby_selection, state->sequence);
    } else if (state->screen == RE_SCREEN_HISTORY && state->history_count > 0) {
        size_t count = re_app_state_history_order(state, NULL, 0);
        size_t rank = re_app_state_history_rank(state, state->history_selection);
        if (rank == SIZE_MAX) rank = delta > 0 ? count - 1 : 0;
        rank = (rank + (delta > 0 ? 1 : count - 1)) % count;
        state->history_selection = history_index_at_rank(state, rank);
    }
    state->dirty = true;
}

void re_app_state_handle_button(re_app_state_t *state, re_button_t button)
{
    if (state == NULL) return;
    if (button == RE_BUTTON_OTHER) return;
    if (button == RE_BUTTON_OK_LONG) {
        if (state->modal != RE_MODAL_NONE) close_modal(state);
        else if (state->screen == RE_SCREEN_NEARBY) open_modal(state, RE_MODAL_QUICK_MENU);
        else if (state->screen == RE_SCREEN_DETAIL) {
            state->screen = state->detail_source == RE_DETAIL_HISTORY ? RE_SCREEN_HISTORY : RE_SCREEN_NEARBY;
            state->dirty = true;
        } else { state->screen = RE_SCREEN_NEARBY; state->history_selection = -1; state->dirty = true; }
        return;
    }
    if (button == RE_BUTTON_UP || button == RE_BUTTON_DOWN) {
        move_selection(state, button == RE_BUTTON_UP ? -1 : 1);
        return;
    }
    if (button != RE_BUTTON_OK_CLICK) return;
    if (state->modal != RE_MODAL_NONE) { confirm_modal(state); return; }
    if (state->screen == RE_SCREEN_NEARBY && state->nearby_selection >= 0 &&
        re_app_state_nearby_visible_rank(state, state->nearby_selection) != SIZE_MAX) {
        state->detail_source = RE_DETAIL_NEARBY; state->screen = RE_SCREEN_DETAIL;
    } else if (state->screen == RE_SCREEN_HISTORY) {
        if (state->history_selection < 0 && state->history_count > 0) {
            if (re_app_state_history_order(state, NULL, 0) != 0)
                state->history_selection = history_index_at_rank(state, 0);
        }
        if (state->history_selection >= 0) { state->detail_source = RE_DETAIL_HISTORY; state->screen = RE_SCREEN_DETAIL; }
    }
    state->dirty = true;
}

bool re_app_state_radar_active(const re_app_state_t *state)
{
    return state != NULL && state->screen == RE_SCREEN_NEARBY && state->backend == RE_BACKEND_SCANNING;
}

bool re_app_state_has_visible_nearby(const re_app_state_t *state)
{
    return re_app_state_nearby_visible_count(state) != 0;
}

bool re_app_state_is_new(const re_app_state_t *state, const radio_identity_t *identity)
{
    if (state == NULL || identity == NULL) return false;
    const int index = history_find(state, identity);
    return index < 0 || !state->history[index].was_present_at_start;
}

size_t re_app_state_nearby_visible_count(const re_app_state_t *state)
{
    if (state == NULL) return 0;
    size_t count = 0;
    for (size_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i)
        if (matches_mode(&state->nearby.records[i], state->mode)) ++count;
    return count;
}

int re_app_state_nearby_visible_at(const re_app_state_t *state, size_t rank)
{
    if (state == NULL) return -1;
    for (size_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i) {
        if (!matches_mode(&state->nearby.records[i], state->mode)) continue;
        if (rank-- == 0) return (int)i;
    }
    return -1;
}

size_t re_app_state_nearby_visible_rank(const re_app_state_t *state, int record_index)
{
    if (state == NULL || record_index < 0 || record_index >= (int)RADIO_NEARBY_CAPACITY ||
        !matches_mode(&state->nearby.records[record_index], state->mode)) return SIZE_MAX;
    size_t rank = 0;
    for (int i = 0; i < record_index; ++i)
        if (matches_mode(&state->nearby.records[i], state->mode)) ++rank;
    return rank;
}

size_t re_app_state_nearby_window(const re_app_state_t *state, size_t max_rows, int *indices)
{
    size_t count = re_app_state_nearby_visible_count(state);
    if (state == NULL || indices == NULL || max_rows == 0 || count == 0) return 0;
    size_t start = 0;
    size_t selected = re_app_state_nearby_visible_rank(state, state->nearby_selection);
    if (selected != SIZE_MAX && selected >= max_rows) start = selected - max_rows + 1;
    size_t out = 0;
    while (out < max_rows && start + out < count) {
        indices[out] = re_app_state_nearby_visible_at(state, start + out);
        ++out;
    }
    return out;
}

size_t re_app_state_history_order(const re_app_state_t *state, int *indices, size_t capacity)
{
    if (state == NULL) return 0;
    size_t count = 0;
    size_t stored = 0;
    for (size_t i = 0; i < RE_HISTORY_VOLATILE_CAPACITY; ++i) {
        if (!state->history[i].occupied) continue;
        if (indices != NULL && capacity != 0) {
            if (stored == capacity && !history_precedes(&state->history[i], i,
                    &state->history[indices[capacity - 1]],
                    (size_t)indices[capacity - 1])) {
                ++count;
                continue;
            }
            size_t pos = stored < capacity ? stored : capacity - 1;
            while (pos > 0 && history_precedes(&state->history[i], i,
                                                &state->history[indices[pos - 1]],
                                                (size_t)indices[pos - 1])) {
                if (pos < capacity) indices[pos] = indices[pos - 1];
                --pos;
            }
            if (pos < capacity) {
                indices[pos] = (int)i;
                if (stored < capacity) ++stored;
            }
        }
        ++count;
    }
    return count;
}

size_t re_app_state_history_rank(const re_app_state_t *state, int history_index)
{
    if (state == NULL || history_index < 0 || history_index >= (int)RE_HISTORY_VOLATILE_CAPACITY ||
        !state->history[history_index].occupied) return SIZE_MAX;
    size_t rank = 0;
    for (size_t i = 0; i < RE_HISTORY_VOLATILE_CAPACITY; ++i)
        if (state->history[i].occupied && history_precedes(&state->history[i], i,
                &state->history[history_index], (size_t)history_index)) ++rank;
    return rank;
}

size_t re_app_state_history_window(const re_app_state_t *state, size_t max_rows, int *indices)
{
    if (state == NULL || indices == NULL || max_rows == 0) return 0;
    size_t count = re_app_state_history_order(state, NULL, 0);
    size_t selected = re_app_state_history_rank(state, state->history_selection);
    size_t start = selected != SIZE_MAX && selected >= max_rows ? selected - max_rows + 1 : 0;
    size_t visible = count > start ? count - start : 0;
    if (visible > max_rows) visible = max_rows;
    for (size_t i = 0; i < visible; ++i)
        indices[i] = history_index_at_rank(state, start + i);
    return visible;
}

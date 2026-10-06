#include "nearby_store.h"

#include <string.h>

/* Sequence deltas intentionally use unsigned wrap; configured intervals stay < 2^31. */
static uint32_t age(uint32_t now, uint32_t then) { return now - then; }

void nearby_store_init(nearby_store_t *store, uint32_t freeze_timeout)
{
    if (store == NULL) return;
    memset(store, 0, sizeof(*store));
    store->selected_index = -1;
    store->freeze_timeout = freeze_timeout;
}

static int find_identity(const nearby_store_t *store, const radio_identity_t *identity)
{
    for (size_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i)
        if (store->records[i].occupied && radio_identity_equal(&store->records[i].observation.identity, identity))
            return (int)i;
    return -1;
}

static int eviction_candidate(const nearby_store_t *store, uint32_t now, uint32_t stale_after)
{
    int candidate = -1;
    for (size_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i) {
        const nearby_record_t *r = &store->records[i];
        if (!r->occupied || (int)i == store->selected_index) continue;
        if (candidate < 0) { candidate = (int)i; continue; }
        const nearby_record_t *c = &store->records[candidate];
        const bool rs = age(now, r->last_seen_seq) >= stale_after;
        const bool cs = age(now, c->last_seen_seq) >= stale_after;
        if ((rs && !cs) || (rs == cs && (age(now, r->last_seen_seq) > age(now, c->last_seen_seq) ||
            (r->last_seen_seq == c->last_seen_seq && r->observation.rssi < c->observation.rssi))))
            candidate = (int)i;
    }
    return candidate;
}

bool nearby_store_update(nearby_store_t *store, const radio_observation_t *input,
                         uint32_t now_seq, uint32_t stale_after, uint32_t expire_after)
{
    if (store == NULL || input == NULL) return false;
    radio_observation_t value = *input;
    if (!radio_observation_normalize(&value)) return false;
    value.seen_seq = now_seq;
    if (store->order_frozen && age(now_seq, store->freeze_seq) >= store->freeze_timeout)
        nearby_store_sort(store, now_seq);
    int index = find_identity(store, &value.identity);
    if (index >= 0) {
        nearby_record_t *r = &store->records[index];
        const uint32_t first = r->first_seen_seq;
        const uint32_t count = r->observations;
        r->observation = value;
        r->first_seen_seq = first;
        r->last_seen_seq = now_seq;
        r->observations = count == UINT32_MAX ? count : count + 1u;
        return true;
    }
    index = -1;
    for (size_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i)
        if (!store->records[i].occupied) { index = (int)i; break; }
    if (index < 0) {
        /* Expired records always precede stale/old/weak candidates. */
        uint32_t oldest_expired_age = 0;
        for (size_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i) {
            nearby_record_t *r = &store->records[i];
            if (!r->occupied || (int)i == store->selected_index) continue;
            uint32_t elapsed = age(now_seq, r->last_seen_seq);
            if (elapsed >= expire_after && elapsed > oldest_expired_age) { index = (int)i; oldest_expired_age = elapsed; }
        }
        if (index < 0) index = eviction_candidate(store, now_seq, stale_after);
        if (index < 0) return false;
    } else store->count++;
    nearby_record_t *r = &store->records[index];
    memset(r, 0, sizeof(*r));
    r->occupied = 1;
    r->observation = value;
    r->first_seen_seq = now_seq;
    r->last_seen_seq = now_seq;
    r->observations = 1;
    if (!store->order_frozen) nearby_store_sort(store, now_seq);
    return true;
}

void nearby_store_sort(nearby_store_t *store, uint32_t now_seq)
{
    if (store == NULL) return;
    if (store->order_frozen && age(now_seq, store->freeze_seq) < store->freeze_timeout) return;
    radio_identity_t selected = {0};
    bool had_selection = store->selected_index >= 0 && store->selected_index < (int)RADIO_NEARBY_CAPACITY &&
                         store->records[store->selected_index].occupied;
    if (had_selection) selected = store->records[store->selected_index].observation.identity;
    /* Compact holes first, then stable insertion-sort in place without a large stack buffer. */
    size_t write = 0;
    for (size_t read = 0; read < RADIO_NEARBY_CAPACITY; ++read) {
        if (!store->records[read].occupied) continue;
        if (write != read) { store->records[write] = store->records[read]; memset(&store->records[read], 0, sizeof(store->records[read])); }
        ++write;
    }
    for (size_t i = 1; i < write; ++i) {
        nearby_record_t current = store->records[i];
        size_t j = i;
        while (j > 0) {
            nearby_record_t *prev = &store->records[j - 1];
            bool before = age(now_seq, current.last_seen_seq) < age(now_seq, prev->last_seen_seq) ||
                (current.last_seen_seq == prev->last_seen_seq && current.observation.rssi > prev->observation.rssi);
            if (!before) break;
            store->records[j] = *prev;
            --j;
        }
        store->records[j] = current;
    }
    store->order_frozen = false;
    store->selected_index = -1;
    if (had_selection) for (size_t i = 0; i < write; ++i)
        if (radio_identity_equal(&store->records[i].observation.identity, &selected)) { store->selected_index = (int)i; break; }
}

void nearby_store_select(nearby_store_t *store, int index, uint32_t now_seq)
{
    if (store == NULL) return;
    store->selected_index = (index >= 0 && index < (int)RADIO_NEARBY_CAPACITY && store->records[index].occupied) ? index : -1;
    store->order_frozen = true;
    store->freeze_seq = now_seq;
}

void nearby_store_navigate(nearby_store_t *store, int delta, uint32_t now_seq)
{
    if (store == NULL || store->count == 0 || delta == 0) return;
    int next = store->selected_index;
    if (next < 0) {
        next = 0;
        while (next < (int)RADIO_NEARBY_CAPACITY && !store->records[next].occupied) ++next;
        if (next == (int)RADIO_NEARBY_CAPACITY) return;
    }
    else {
        do { next = (next + (delta > 0 ? 1 : (int)RADIO_NEARBY_CAPACITY - 1)) % (int)RADIO_NEARBY_CAPACITY; }
        while (!store->records[next].occupied);
    }
    nearby_store_select(store, next, now_seq);
}

void nearby_store_leave_list(nearby_store_t *store)
{
    if (store == NULL) return;
    store->selected_index = -1;
    store->order_frozen = false;
}

size_t nearby_store_expire(nearby_store_t *store, uint32_t now_seq, uint32_t expire_after)
{
    if (store == NULL) return 0;
    size_t removed = 0;
    for (size_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i) {
        nearby_record_t *r = &store->records[i];
        if (r->occupied && (int)i != store->selected_index && age(now_seq, r->last_seen_seq) >= expire_after) {
            memset(r, 0, sizeof(*r)); --store->count; ++removed;
        }
    }
    return removed;
}

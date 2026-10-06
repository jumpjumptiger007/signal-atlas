#ifndef RADIO_EXPLORER_NEARBY_STORE_H
#define RADIO_EXPLORER_NEARBY_STORE_H

#include "observation.h"

typedef struct {
    radio_observation_t observation;
    uint32_t first_seen_seq;
    uint32_t last_seen_seq;
    uint32_t observations;
    uint8_t occupied;
} nearby_record_t;

typedef struct {
    nearby_record_t records[RADIO_NEARBY_CAPACITY];
    size_t count;
    int selected_index;
    bool order_frozen;
    uint32_t freeze_seq;
    uint32_t freeze_timeout;
} nearby_store_t;

void nearby_store_init(nearby_store_t *store, uint32_t freeze_timeout);
bool nearby_store_update(nearby_store_t *store, const radio_observation_t *observation,
                         uint32_t now_seq, uint32_t stale_after, uint32_t expire_after);
void nearby_store_select(nearby_store_t *store, int index, uint32_t now_seq);
void nearby_store_navigate(nearby_store_t *store, int delta, uint32_t now_seq);
void nearby_store_leave_list(nearby_store_t *store);
void nearby_store_sort(nearby_store_t *store, uint32_t now_seq);
size_t nearby_store_expire(nearby_store_t *store, uint32_t now_seq, uint32_t expire_after);

#endif

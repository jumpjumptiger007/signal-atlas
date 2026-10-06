#ifndef RADIO_EXPLORER_DETAIL_FORMAT_H
#define RADIO_EXPLORER_DETAIL_FORMAT_H

#include "../model/observation.h"

#include <stddef.h>
#include <stdint.h>

/* Formats a bounded, evidence-labelled detail view. `observation` may be NULL
 * for an older history summary that is no longer in Nearby. */
size_t radio_detail_format(const radio_observation_t *observation,
                           const radio_identity_t *identity, uint8_t kind,
                           int8_t last_rssi, uint32_t observations,
                           char *dst, size_t capacity);

#endif

#ifndef RADIO_EXPLORER_HISTORY_CODEC_H
#define RADIO_EXPLORER_HISTORY_CODEC_H

#include "../model/observation.h"

#define RADIO_HISTORY_CAPACITY 512u
#define RADIO_HISTORY_SCHEMA_VERSION 1u
#define RADIO_HISTORY_RECORD_BYTES 22u
#define RADIO_HISTORY_HEADER_BYTES 24u
#define RADIO_HISTORY_BLOB_BYTES \
    (RADIO_HISTORY_HEADER_BYTES + RADIO_HISTORY_CAPACITY * RADIO_HISTORY_RECORD_BYTES)

typedef struct {
    radio_identity_t identity;
    uint32_t first_session;
    uint32_t last_session;
    uint32_t observations;
    uint32_t last_seen_seq;
    int8_t last_rssi;
    uint8_t classification_flags;
    uint8_t was_present_at_start;
    uint8_t occupied;
} radio_history_record_t;

typedef enum {
    RADIO_HISTORY_DECODE_OK = 0,
    RADIO_HISTORY_DECODE_EMPTY,
    RADIO_HISTORY_DECODE_INCOMPATIBLE,
    RADIO_HISTORY_DECODE_CORRUPT,
} radio_history_decode_result_t;

bool radio_history_codec_encode(uint32_t session_id, uint32_t generation,
                                const radio_history_record_t *records,
                                size_t count, uint8_t *out, size_t out_capacity,
                                size_t *out_length);
radio_history_decode_result_t radio_history_codec_decode(
    const uint8_t *input, size_t input_length, uint32_t *session_id,
    uint32_t *generation, radio_history_record_t *records,
    size_t record_capacity, size_t *record_count);
uint32_t radio_history_codec_crc32(const uint8_t *bytes, size_t length);

#endif

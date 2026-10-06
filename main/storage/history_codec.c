#include "history_codec.h"

#include <string.h>

enum { HEADER_CRC_OFFSET = 20 };
static const uint8_t MAGIC[4] = { 'R', 'E', 'H', '1' };

static uint32_t crc32_update(uint32_t crc, const uint8_t *bytes, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(crc & 1u));
    }
    return crc;
}

static uint32_t blob_crc(const uint8_t *bytes, size_t length)
{
    uint32_t crc = crc32_update(UINT32_MAX, bytes, HEADER_CRC_OFFSET);
    const uint8_t zeros[4] = {0};
    crc = crc32_update(crc, zeros, sizeof(zeros));
    crc = crc32_update(crc, bytes + HEADER_CRC_OFFSET + sizeof(zeros),
                       length - HEADER_CRC_OFFSET - sizeof(zeros));
    return ~crc;
}

static void put_u16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}

static uint16_t get_u16(const uint8_t *p)
{
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
}

static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

uint32_t radio_history_codec_crc32(const uint8_t *bytes, size_t length)
{
    if (bytes == NULL && length != 0) return 0;
    return ~crc32_update(UINT32_MAX, bytes, length);
}

bool radio_history_codec_encode(uint32_t session_id, uint32_t generation,
                                const radio_history_record_t *records,
                                size_t count, uint8_t *out, size_t out_capacity,
                                size_t *out_length)
{
    if (out_length != NULL) *out_length = 0;
    if (out == NULL || (records == NULL && count != 0) || count > RADIO_HISTORY_CAPACITY ||
        out_capacity < RADIO_HISTORY_HEADER_BYTES + count * RADIO_HISTORY_RECORD_BYTES)
        return false;
    memset(out, 0, RADIO_HISTORY_HEADER_BYTES + count * RADIO_HISTORY_RECORD_BYTES);
    memcpy(out, MAGIC, sizeof(MAGIC));
    put_u16(out + 4, RADIO_HISTORY_SCHEMA_VERSION);
    put_u16(out + 6, RADIO_HISTORY_RECORD_BYTES);
    put_u32(out + 8, session_id);
    put_u16(out + 12, (uint16_t)count);
    put_u32(out + 16, generation);
    uint16_t order[RADIO_HISTORY_CAPACITY];
    size_t ordered = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!records[i].occupied || (records[i].identity.kind != RADIO_KIND_WIFI &&
                                     records[i].identity.kind != RADIO_KIND_BLE)) return false;
        size_t pos = ordered;
        while (pos > 0) {
            const radio_history_record_t *previous = &records[order[pos - 1]];
            const int32_t delta = (int32_t)(records[i].last_seen_seq - previous->last_seen_seq);
            if (delta < 0 || (delta == 0 && i > order[pos - 1])) break;
            order[pos] = order[pos - 1];
            --pos;
        }
        order[pos] = (uint16_t)i;
        ++ordered;
    }
    uint8_t *p = out + RADIO_HISTORY_HEADER_BYTES;
    for (size_t i = 0; i < count; ++i, p += RADIO_HISTORY_RECORD_BYTES) {
        const radio_history_record_t *record = &records[order[i]];
        p[0] = (uint8_t)record->identity.kind;
        p[1] = record->identity.address_type;
        memcpy(p + 2, record->identity.address, 6);
        put_u32(p + 8, record->first_session);
        put_u32(p + 12, record->last_session);
        put_u32(p + 16, record->observations);
        p[20] = (uint8_t)record->last_rssi;
        p[21] = record->classification_flags;
    }
    const size_t length = RADIO_HISTORY_HEADER_BYTES + count * RADIO_HISTORY_RECORD_BYTES;
    put_u32(out + HEADER_CRC_OFFSET, blob_crc(out, length));
    if (out_length != NULL) *out_length = length;
    return true;
}

radio_history_decode_result_t radio_history_codec_decode(
    const uint8_t *input, size_t input_length, uint32_t *session_id,
    uint32_t *generation, radio_history_record_t *records,
    size_t record_capacity, size_t *record_count)
{
    if (record_count != NULL) *record_count = 0;
    if (input == NULL || input_length < RADIO_HISTORY_HEADER_BYTES) return RADIO_HISTORY_DECODE_CORRUPT;
    if (memcmp(input, MAGIC, sizeof(MAGIC)) != 0) return RADIO_HISTORY_DECODE_CORRUPT;
    const uint16_t version = get_u16(input + 4);
    const uint16_t record_bytes = get_u16(input + 6);
    if (version != RADIO_HISTORY_SCHEMA_VERSION || record_bytes != RADIO_HISTORY_RECORD_BYTES)
        return RADIO_HISTORY_DECODE_INCOMPATIBLE;
    const size_t count = get_u16(input + 12);
    const size_t expected = RADIO_HISTORY_HEADER_BYTES + count * RADIO_HISTORY_RECORD_BYTES;
    if (count > RADIO_HISTORY_CAPACITY || count > record_capacity ||
        (records == NULL && count != 0) || expected != input_length)
        return RADIO_HISTORY_DECODE_CORRUPT;
    if (get_u32(input + HEADER_CRC_OFFSET) != blob_crc(input, input_length))
        return RADIO_HISTORY_DECODE_CORRUPT;
    if (session_id != NULL) *session_id = get_u32(input + 8);
    if (generation != NULL) *generation = get_u32(input + 16);
    const uint8_t *p = input + RADIO_HISTORY_HEADER_BYTES;
    for (size_t i = 0; i < count; ++i, p += RADIO_HISTORY_RECORD_BYTES) {
        radio_history_record_t *record = &records[i];
        memset(record, 0, sizeof(*record));
        if (p[0] != RADIO_KIND_WIFI && p[0] != RADIO_KIND_BLE) return RADIO_HISTORY_DECODE_CORRUPT;
        record->identity.kind = (radio_kind_t)p[0];
        record->identity.address_type = p[1];
        memcpy(record->identity.address, p + 2, 6);
        record->first_session = get_u32(p + 8);
        record->last_session = get_u32(p + 12);
        record->observations = get_u32(p + 16);
        record->last_rssi = (int8_t)p[20];
        record->classification_flags = p[21];
        record->occupied = 1;
        record->was_present_at_start = 1;
        for (size_t previous = 0; previous < i; ++previous)
            if (radio_identity_equal(&records[previous].identity, &record->identity))
                return RADIO_HISTORY_DECODE_CORRUPT;
    }
    if (record_count != NULL) *record_count = count;
    return count == 0 ? RADIO_HISTORY_DECODE_EMPTY : RADIO_HISTORY_DECODE_OK;
}

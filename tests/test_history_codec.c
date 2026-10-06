#include "storage/history_codec.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    static radio_history_record_t source[RADIO_HISTORY_CAPACITY];
    static radio_history_record_t restored[RADIO_HISTORY_CAPACITY];
    static uint8_t blob[RADIO_HISTORY_BLOB_BYTES];
    size_t length = 0, count = 0;
    uint32_t session = 0, generation = 0;
    for (size_t i = 0; i < RADIO_HISTORY_CAPACITY; ++i) {
        source[i].occupied = 1;
        source[i].identity.kind = i % 2 == 0 ? RADIO_KIND_WIFI : RADIO_KIND_BLE;
        source[i].identity.address_type = (uint8_t)(i % 2);
        source[i].identity.address[0] = (uint8_t)i;
        source[i].identity.address[5] = (uint8_t)(i >> 1);
        source[i].first_session = 2;
        source[i].last_session = 9;
        source[i].observations = (uint32_t)(i + 1);
        source[i].last_seen_seq = (uint32_t)i;
        source[i].last_rssi = (int8_t)-40;
        source[i].classification_flags = (uint8_t)(i & 3u);
    }
    assert(radio_history_codec_encode(10, 42, source, RADIO_HISTORY_CAPACITY,
                                      blob, sizeof(blob), &length));
    assert(length == RADIO_HISTORY_BLOB_BYTES);
    assert(radio_history_codec_decode(blob, length, &session, &generation,
                                      restored, RADIO_HISTORY_CAPACITY, &count) == RADIO_HISTORY_DECODE_OK);
    assert(count == RADIO_HISTORY_CAPACITY && session == 10 && generation == 42);
    for (size_t i = 0; i < RADIO_HISTORY_CAPACITY; ++i) {
        const size_t original = RADIO_HISTORY_CAPACITY - i - 1u;
        assert(radio_identity_equal(&source[original].identity, &restored[i].identity));
        assert(source[original].first_session == restored[i].first_session);
        assert(source[original].last_session == restored[i].last_session);
        assert(source[original].observations == restored[i].observations);
        assert(source[original].last_rssi == restored[i].last_rssi);
        assert(source[original].classification_flags == restored[i].classification_flags);
        assert(restored[i].was_present_at_start);
    }
    assert(radio_history_codec_decode(blob, length - 1, &session, &generation,
                                      restored, RADIO_HISTORY_CAPACITY, &count) == RADIO_HISTORY_DECODE_CORRUPT);
    blob[20] ^= 1;
    assert(radio_history_codec_decode(blob, length, &session, &generation,
                                      restored, RADIO_HISTORY_CAPACITY, &count) == RADIO_HISTORY_DECODE_CORRUPT);
    blob[20] ^= 1;
    blob[4]++;
    assert(radio_history_codec_decode(blob, length, &session, &generation,
                                      restored, RADIO_HISTORY_CAPACITY, &count) == RADIO_HISTORY_DECODE_INCOMPATIBLE);
    puts("History codec 512-record round-trip, schema, bounds, and corruption tests: PASS");
    return 0;
}

#ifndef RADIO_EXPLORER_PROTOCOL_DECODER_H
#define RADIO_EXPLORER_PROTOCOL_DECODER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { RADIO_FRAME_NONE = 0, RADIO_FRAME_IBEACON, RADIO_FRAME_EDDYSTONE_UID,
               RADIO_FRAME_EDDYSTONE_URL, RADIO_FRAME_EDDYSTONE_TLM } radio_frame_kind_t;

typedef struct {
    radio_frame_kind_t kind;
    union {
        struct { uint8_t uuid[16]; uint16_t major, minor; int8_t tx_power; } ibeacon;
        struct { uint8_t namespace_id[10], instance_id[6]; int8_t tx_power; } uid;
        struct { char value[96]; uint8_t length; int8_t tx_power; } url;
        struct { uint8_t version; uint16_t battery_mv; int16_t temperature_c_q8; uint32_t adv_count, uptime_100ms; } tlm;
    } data;
} radio_decoded_frame_t;

bool radio_decode_ibeacon(uint16_t company_id, const uint8_t *data, size_t length, radio_decoded_frame_t *out);
bool radio_decode_eddystone(const uint8_t *service_data, size_t length, radio_decoded_frame_t *out);

#endif

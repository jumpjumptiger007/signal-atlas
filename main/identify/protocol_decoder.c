#include "protocol_decoder.h"

#include <string.h>

static uint16_t be16(const uint8_t *p) { return ((uint16_t)p[0] << 8) | p[1]; }
static uint32_t be32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }

bool radio_decode_ibeacon(uint16_t company_id, const uint8_t *data, size_t length, radio_decoded_frame_t *out)
{
    if (company_id != 0x004c || data == NULL || out == NULL || length != 23 || data[0] != 0x02 || data[1] != 0x15)
        return false;
    memset(out, 0, sizeof(*out));
    out->kind = RADIO_FRAME_IBEACON;
    memcpy(out->data.ibeacon.uuid, data + 2, 16);
    out->data.ibeacon.major = be16(data + 18);
    out->data.ibeacon.minor = be16(data + 20);
    out->data.ibeacon.tx_power = (int8_t)data[22];
    return true;
}

static bool append_char(char *dst, size_t cap, uint8_t *len, char c)
{
    if ((size_t)*len + 1 >= cap) return false;
    dst[(*len)++] = c; dst[*len] = '\0'; return true;
}

static bool decode_url(const uint8_t *data, size_t length, radio_decoded_frame_t *out)
{
    static const char *const schemes[] = {"http://www.", "https://www.", "http://", "https://"};
    static const char *const expansions[] = {".com/", ".org/", ".edu/", ".net/", ".info/", ".biz/", ".gov/", ".com", ".org", ".edu", ".net", ".info", ".biz", ".gov"};
    if (length < 3 || data[1] > 3) return false;
    memset(out, 0, sizeof(*out)); out->kind = RADIO_FRAME_EDDYSTONE_URL;
    out->data.url.tx_power = (int8_t)data[0];
    const char *scheme = schemes[data[1]];
    for (const char *p = scheme; *p; ++p) if (!append_char(out->data.url.value, sizeof(out->data.url.value), &out->data.url.length, *p)) return false;
    for (size_t i = 2; i < length; ++i) {
        uint8_t c = data[i];
        if (c < 14) {
            const char *p = expansions[c];
            while (*p) if (!append_char(out->data.url.value, sizeof(out->data.url.value), &out->data.url.length, *p++)) return false;
        } else if (c >= 32 && c <= 126) {
            if (!append_char(out->data.url.value, sizeof(out->data.url.value), &out->data.url.length, (char)c)) return false;
        } else return false;
    }
    return true;
}

bool radio_decode_eddystone(const uint8_t *data, size_t length, radio_decoded_frame_t *out)
{
    if (data == NULL || out == NULL || length == 0) return false;
    size_t offset = 0;
    /* Accept raw AD Service Data (UUID16 AA FE on-air) and donor frame bytes. */
    if (length >= 3 && data[0] == 0xaa && data[1] == 0xfe) offset = 2;
    if (length <= offset) return false;
    const uint8_t frame = data[offset]; const uint8_t *p = data + offset + 1; const size_t n = length - offset - 1;
    memset(out, 0, sizeof(*out));
    if (frame == 0x00 && (n == 17 || n == 19)) {
        out->kind = RADIO_FRAME_EDDYSTONE_UID;
        out->data.uid.tx_power = (int8_t)p[0];
        memcpy(out->data.uid.namespace_id, p + 1, 10); memcpy(out->data.uid.instance_id, p + 11, 6);
        return true;
    }
    if (frame == 0x10) return decode_url(p, n, out);
    if (frame == 0x20 && n == 13 && p[0] == 0x00) {
        out->kind = RADIO_FRAME_EDDYSTONE_TLM;
        out->data.tlm.version = p[0];
        out->data.tlm.battery_mv = be16(p + 1);
        out->data.tlm.temperature_c_q8 = (int16_t)be16(p + 3);
        out->data.tlm.adv_count = be32(p + 5);
        out->data.tlm.uptime_100ms = be32(p + 9);
        return true;
    }
    return false;
}

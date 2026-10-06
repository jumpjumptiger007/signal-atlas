#include "ble_ad_parser.h"

#include <string.h>

enum { AD_FLAGS = 0x01, AD_UUID16_INCOMPLETE = 0x02, AD_UUID16_COMPLETE = 0x03,
       AD_UUID32_INCOMPLETE = 0x04, AD_UUID32_COMPLETE = 0x05,
       AD_UUID128_INCOMPLETE = 0x06, AD_UUID128_COMPLETE = 0x07,
       AD_SHORT_NAME = 0x08, AD_COMPLETE_NAME = 0x09, AD_TX_POWER = 0x0a,
       AD_APPEARANCE = 0x19, AD_SERVICE_DATA16 = 0x16, AD_SERVICE_DATA32 = 0x20,
       AD_SERVICE_DATA128 = 0x21, AD_MANUFACTURER = 0xff };

static uint16_t le16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }

static void copy_bounded(uint8_t *dst, uint8_t *dst_len, size_t cap, const uint8_t *src, size_t len)
{
    size_t n = len < cap ? len : cap;
    if (n != 0) memcpy(dst, src, n);
    *dst_len = (uint8_t)n;
}

static bool append_uuid(radio_ble_data_t *out, const uint8_t *bytes, size_t width)
{
    if (out->uuid_count >= RADIO_BLE_UUID_MAX || width > UINT8_MAX ||
        out->uuid_bytes_len + 1u + width > RADIO_BLE_UUID_BYTES_MAX) return false;
    out->uuid_bytes[out->uuid_bytes_len++] = (uint8_t)width;
    memcpy(out->uuid_bytes + out->uuid_bytes_len, bytes, width);
    out->uuid_bytes_len = (uint8_t)(out->uuid_bytes_len + width);
    ++out->uuid_count;
    return true;
}

static bool parse_uuid_list(radio_ble_data_t *out, const uint8_t *p, size_t n, size_t width)
{
    if (n == 0 || n % width != 0) return false;
    for (size_t i = 0; i < n; i += width) if (!append_uuid(out, p + i, width)) out->uuid_truncated = 1;
    return true;
}

static bool service_data_is_eddystone(const radio_ble_data_t *out)
{
    /* AD UUID16 values are on-air little-endian: FEAA is encoded AA FE. */
    return out->service_data_len >= 2 && out->service_data[0] == 0xaa && out->service_data[1] == 0xfe;
}

static void store_service_data(radio_ble_data_t *out, const uint8_t *p, size_t n)
{
    const bool is_eddystone = n >= 2 && p[0] == 0xaa && p[1] == 0xfe;
    if (out->service_data_len == 0 || (is_eddystone && !service_data_is_eddystone(out)))
        copy_bounded(out->service_data, &out->service_data_len, RADIO_BLE_SERVICE_DATA_MAX, p, n);
}

ble_ad_status_t ble_ad_parse(const uint8_t *bytes, size_t length, radio_ble_data_t *out)
{
    if ((bytes == NULL && length != 0) || out == NULL) return BLE_AD_INVALID_ARGUMENT;
    memset(out, 0, sizeof(*out));
    size_t pos = 0;
    while (pos < length) {
        uint8_t field_len = bytes[pos++];
        if (field_len == 0) break;
        if ((size_t)field_len > length - pos) return BLE_AD_MALFORMED;
        uint8_t type = bytes[pos];
        const uint8_t *p = bytes + pos + 1u;
        size_t n = field_len - 1u;
        switch (type) {
        case AD_SHORT_NAME:
            if (out->name_len == 0) copy_bounded(out->name, &out->name_len, RADIO_BLE_NAME_MAX, p, n);
            break;
        case AD_COMPLETE_NAME:
            copy_bounded(out->name, &out->name_len, RADIO_BLE_NAME_MAX, p, n);
            break;
        case AD_TX_POWER:
            if (n != 1) return BLE_AD_MALFORMED;
            if (!out->has_tx_power) { out->tx_power = (int8_t)p[0]; out->has_tx_power = 1; }
            break;
        case AD_APPEARANCE:
            if (n != 2) return BLE_AD_MALFORMED;
            if (!out->appearance_present) { out->appearance = le16(p); out->appearance_present = 1; }
            break;
        case AD_MANUFACTURER:
            if (n < 2) return BLE_AD_MALFORMED;
            if (!out->has_company_id) {
                out->company_id = le16(p); out->has_company_id = 1;
                copy_bounded(out->manufacturer, &out->manufacturer_len, RADIO_BLE_MANUFACTURER_MAX, p + 2, n - 2);
            }
            break;
        case AD_UUID16_INCOMPLETE: case AD_UUID16_COMPLETE:
            if (!parse_uuid_list(out, p, n, 2)) return BLE_AD_MALFORMED;
            break;
        case AD_UUID32_INCOMPLETE: case AD_UUID32_COMPLETE:
            if (!parse_uuid_list(out, p, n, 4)) return BLE_AD_MALFORMED;
            break;
        case AD_UUID128_INCOMPLETE: case AD_UUID128_COMPLETE:
            if (!parse_uuid_list(out, p, n, 16)) return BLE_AD_MALFORMED;
            break;
        case AD_SERVICE_DATA16:
            if (n < 2) return BLE_AD_MALFORMED;
            if (!append_uuid(out, p, 2)) out->uuid_truncated = 1;
            store_service_data(out, p, n);
            break;
        case AD_SERVICE_DATA32:
            if (n < 4) return BLE_AD_MALFORMED;
            if (!append_uuid(out, p, 4)) out->uuid_truncated = 1;
            store_service_data(out, p, n);
            break;
        case AD_SERVICE_DATA128:
            if (n < 16) return BLE_AD_MALFORMED;
            if (!append_uuid(out, p, 16)) out->uuid_truncated = 1;
            store_service_data(out, p, n);
            break;
        case AD_FLAGS:
            if (n != 1) return BLE_AD_MALFORMED;
            break;
        default: break; /* Unknown AD types are safely ignored. */
        }
        pos += field_len;
    }
    return BLE_AD_OK;
}

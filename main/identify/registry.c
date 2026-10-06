#include "registry.h"

#include "../model/observation.h"
#include "registry_data.h"

static uint32_t read_offset(const uint8_t value[3])
{
    return (uint32_t)value[0] | ((uint32_t)value[1] << 8) | ((uint32_t)value[2] << 16);
}

static uint32_t name_offset(const registry_name_record_t *records, size_t count, uint16_t key)
{
    size_t lo = 0, hi = count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (records[mid].key < key) lo = mid + 1;
        else hi = mid;
    }
    return lo < count && records[lo].key == key ? read_offset(records[lo].name_offset) : UINT32_MAX;
}

static const char *lookup_name(uint32_t offset)
{
    return offset == UINT32_MAX || offset >= radio_registry_strings_size ? NULL : radio_registry_strings + offset;
}

static const char *lookup_ieee_name(uint32_t offset)
{
    return offset == UINT32_MAX || offset >= radio_ieee_strings_size ? NULL : radio_ieee_strings + offset;
}

void radio_service_uuid16_expand(uint16_t uuid, uint8_t out[16])
{
    static const uint8_t base[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                                     0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb};
    if (out == NULL) return;
    for (size_t i = 0; i < sizeof(base); ++i) out[i] = base[i];
    out[2] = (uint8_t)(uuid >> 8); out[3] = (uint8_t)uuid;
}

const char *radio_service_lookup(const uint8_t uuid[16])
{
    if (uuid == NULL) return NULL;
    size_t lo = 0, hi = radio_service_records_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2; int cmp = 0;
        for (size_t i = 0; i < 16; ++i) if (radio_service_records[mid].uuid[i] != uuid[i]) {
            cmp = radio_service_records[mid].uuid[i] < uuid[i] ? -1 : 1; break;
        }
        if (cmp < 0) lo = mid + 1; else hi = mid;
    }
    if (lo == radio_service_records_count) return NULL;
    for (size_t i = 0; i < 16; ++i) if (radio_service_records[lo].uuid[i] != uuid[i]) return NULL;
    return lookup_name(read_offset(radio_service_records[lo].name_offset));
}

static bool find_ieee(const registry_ieee_record_t *records, size_t count,
                      const uint8_t key[6], uint32_t *offset)
{
    size_t lo = 0, hi = count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int cmp = 0;
        for (size_t i = 0; i < 6; ++i) if (records[mid].prefix[i] != key[i]) { cmp = records[mid].prefix[i] < key[i] ? -1 : 1; break; }
        if (cmp < 0) lo = mid + 1;
        else hi = mid;
    }
    if (lo < count) {
        for (size_t i = 0; i < 6; ++i) if (records[lo].prefix[i] != key[i]) return false;
        *offset = read_offset(records[lo].name_offset); return true;
    }
    return false;
}

const char *radio_ieee_lookup(const uint8_t mac[6], uint8_t *matched_bits)
{
    if (mac == NULL || radio_mac_is_private_or_local(mac)) return NULL;
    uint8_t key[6]; uint32_t offset;
    for (size_t i = 0; i < 6; ++i) key[i] = mac[i];
    key[4] &= 0xf0u; key[5] = 0;
    if (find_ieee(radio_ieee_ma_s, radio_ieee_ma_s_count, key, &offset)) { if (matched_bits) *matched_bits = 36; return lookup_ieee_name(offset); }
    key[3] = (uint8_t)(mac[3] & 0xf0u); key[4] = 0;
    if (find_ieee(radio_ieee_ma_m, radio_ieee_ma_m_count, key, &offset)) { if (matched_bits) *matched_bits = 28; return lookup_ieee_name(offset); }
    key[3] = key[4] = 0; key[5] = 0;
    if (find_ieee(radio_ieee_ma_l, radio_ieee_ma_l_count, key, &offset)) { if (matched_bits) *matched_bits = 24; return lookup_ieee_name(offset); }
    return NULL;
}

const char *radio_company_lookup(uint16_t id) { return lookup_name(name_offset(radio_company_records, radio_company_records_count, id)); }
const char *radio_appearance_lookup(uint16_t id) { return lookup_name(name_offset(radio_appearance_records, radio_appearance_records_count, id)); }
const char *radio_registry_build_id(void) { return RADIO_REGISTRY_BUILD_ID; }
size_t radio_registry_ieee_record_count(void) { return radio_ieee_ma_l_count + radio_ieee_ma_m_count + radio_ieee_ma_s_count; }
size_t radio_registry_bluetooth_record_count(void) { return radio_company_records_count + radio_service_records_count + radio_appearance_records_count; }

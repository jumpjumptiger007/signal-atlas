#include "detail_format.h"

#include "../identify/protocol_decoder.h"
#include "../identify/registry.h"

#include <stdarg.h>
#include <stdio.h>

typedef struct { char *dst; size_t cap; size_t used; } writer_t;

static void append(writer_t *w, const char *fmt, ...)
{
    if (w->cap == 0 || w->used >= w->cap - 1) return;
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(w->dst + w->used, w->cap - w->used, fmt, args);
    va_end(args);
    if (n > 0) {
        size_t remaining = w->cap - w->used;
        w->used += (size_t)n < remaining ? (size_t)n : remaining - 1;
    }
}

static void address_text(const radio_identity_t *identity, char out[18])
{
    (void)snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
        identity->address[0], identity->address[1], identity->address[2],
        identity->address[3], identity->address[4], identity->address[5]);
}

static void bytes_text(const uint8_t *bytes, size_t length, char *out, size_t cap)
{
    if (cap == 0) return;
    size_t n = length < cap - 1 ? length : cap - 1;
    for (size_t i = 0; i < n; ++i)
        out[i] = bytes[i] >= 0x20 && bytes[i] <= 0x7e ? (char)bytes[i] : '?';
    out[n] = '\0';
}

static void append_hex(writer_t *w, const uint8_t *bytes, size_t length)
{
    size_t n = length < 12 ? length : 12;
    for (size_t i = 0; i < n; ++i) append(w, "%02X%s", bytes[i], i + 1 == n ? "" : " ");
    if (length > n) append(w, " …");
}

static void resolved_ble(writer_t *w, const radio_ble_data_t *ble)
{
    bool has_resolved = false;
    if (ble->appearance_present && radio_appearance_lookup(ble->appearance) != NULL) has_resolved = true;
    if (ble->has_company_id || ble->uuid_count)
        has_resolved = true;
    radio_decoded_frame_t frame;
    bool ibeacon = ble->has_company_id && radio_decode_ibeacon(ble->company_id,
        ble->manufacturer, ble->manufacturer_len, &frame);
    radio_decoded_frame_t eddystone_frame;
    bool eddystone = ble->service_data_len && radio_decode_eddystone(
        ble->service_data, ble->service_data_len, &eddystone_frame);
    if (!has_resolved && !ibeacon && !eddystone) return;

    append(w, "\nRESOLVED\n");
    if (ble->appearance_present) {
        const char *appearance = radio_appearance_lookup(ble->appearance);
        if (appearance != NULL) append(w, "Appearance name: %s\n", appearance);
    }
    if (ble->has_company_id) {
        const char *company = radio_company_lookup(ble->company_id);
        append(w, "Advertising data owner: %s\n", company ? company : "unknown Company ID");
    }
    if (ble->uuid_count != 0) {
        append(w, "Services:\n");
        size_t pos = 0;
        while (pos < ble->uuid_bytes_len) {
            uint8_t width = ble->uuid_bytes[pos++];
            if (width == 0 || pos + width > ble->uuid_bytes_len) break;
            if (width == 2) {
                uint16_t value = (uint16_t)(ble->uuid_bytes[pos] | ((uint16_t)ble->uuid_bytes[pos + 1] << 8));
                uint8_t full[16]; radio_service_uuid16_expand(value, full);
                const char *name = radio_service_lookup(full);
                append(w, "  %04X %s\n", value, name ? name : "unknown service");
            } else append(w, "  %u-byte UUID\n", width);
            pos += width;
        }
    }
    if (ibeacon) {
        append(w, "iBeacon\nUUID: ");
        for (size_t i = 0; i < sizeof(frame.data.ibeacon.uuid); ++i) append(w, "%02X", frame.data.ibeacon.uuid[i]);
        append(w, "\nMajor: %u · Minor: %u · TX: %d dBm\n", frame.data.ibeacon.major,
               frame.data.ibeacon.minor, frame.data.ibeacon.tx_power);
    }
    if (eddystone) {
        if (eddystone_frame.kind == RADIO_FRAME_EDDYSTONE_UID) {
            append(w, "Eddystone UID · TX %d dBm\nNamespace: ", eddystone_frame.data.uid.tx_power);
            for (size_t i = 0; i < 10; ++i) append(w, "%02X", eddystone_frame.data.uid.namespace_id[i]);
            append(w, "\nInstance: ");
            for (size_t i = 0; i < 6; ++i) append(w, "%02X", eddystone_frame.data.uid.instance_id[i]);
            append(w, "\n");
        } else if (eddystone_frame.kind == RADIO_FRAME_EDDYSTONE_URL) {
            append(w, "Eddystone URL · TX %d dBm\n%s\n", eddystone_frame.data.url.tx_power, eddystone_frame.data.url.value);
        } else if (eddystone_frame.kind == RADIO_FRAME_EDDYSTONE_TLM) {
            append(w, "Eddystone TLM\nBattery: %u mV\nAdvertisements: %lu\nUptime: %lu × 100 ms\n",
                eddystone_frame.data.tlm.battery_mv, (unsigned long)eddystone_frame.data.tlm.adv_count,
                (unsigned long)eddystone_frame.data.tlm.uptime_100ms);
        }
    }
}

size_t radio_detail_format(const radio_observation_t *observation,
                           const radio_identity_t *identity, uint8_t kind,
                           int8_t last_rssi, uint32_t observations,
                           char *dst, size_t capacity)
{
    if (dst == NULL || capacity == 0) return 0;
    writer_t w = { .dst = dst, .cap = capacity, .used = 0 };
    dst[0] = '\0';
    if (observation != NULL) { identity = &observation->identity; kind = (uint8_t)observation->kind; last_rssi = observation->rssi; }
    if (identity == NULL) { append(&w, "OBSERVED\nNo selected observation.\n"); return w.used; }
    char address[18]; address_text(identity, address);
    append(&w, "OBSERVED\nIdentity: %s\nProtocol: %s\n", address,
        kind == RADIO_KIND_WIFI ? "Wi-Fi BSSID" : "BLE address");
    if (observation == NULL) {
        append(&w, "Last RSSI: %d dBm\n\nSEEN\nThis session · %lu observations\n",
            last_rssi, (unsigned long)observations);
        return w.used;
    }

    if (observation->kind == RADIO_KIND_WIFI) {
        const radio_wifi_data_t *wifi = &observation->data.wifi;
        char ssid[RADIO_WIFI_SSID_MAX + 1];
        if (wifi->hidden || wifi->ssid_len == 0) (void)snprintf(ssid, sizeof(ssid), "Hidden SSID");
        else bytes_text(wifi->ssid, wifi->ssid_len, ssid, sizeof(ssid));
        append(&w, "Network: %s\nBSSID: %s\nRSSI: %d dBm\nChannel: %u\nSecurity: %s\n",
            ssid, address, observation->rssi, wifi->channel,
            radio_wifi_security_name((radio_wifi_security_t)wifi->security));
        if (!radio_mac_is_private_or_local(observation->identity.address)) {
            uint8_t bits = 0; const char *vendor = radio_ieee_lookup(observation->identity.address, &bits);
            if (vendor != NULL) append(&w, "\nRESOLVED\nIEEE prefix (%u-bit): %s\n", bits, vendor);
        }
    } else {
        const radio_ble_data_t *ble = &observation->data.ble;
        char name[RADIO_BLE_NAME_MAX + 1];
        if (ble->name_len) bytes_text(ble->name, ble->name_len, name, sizeof(name));
        else (void)snprintf(name, sizeof(name), "(not advertised)");
        append(&w, "Name: %s\nAddress: %s\nAddress type: %s\nRSSI: %d dBm\n",
            name, address, observation->identity.address_type == RADIO_ADDRESS_RANDOM ? "random" : "public", observation->rssi);
        if (ble->has_company_id) append(&w, "Company ID: 0x%04X (advertising data owner)\n", ble->company_id);
        if (ble->appearance_present) append(&w, "Appearance value: %u\n", ble->appearance);
        resolved_ble(&w, ble);
        append(&w, "\nSEEN\nThis session · %lu observations\n", (unsigned long)observations);
        append(&w, "\nRAW\nCompany data: %u bytes\nService data: %u bytes\n",
            ble->manufacturer_len, ble->service_data_len);
        if (ble->manufacturer_len) { append(&w, "  "); append_hex(&w, ble->manufacturer, ble->manufacturer_len); append(&w, "\n"); }
        if (ble->service_data_len) { append(&w, "  "); append_hex(&w, ble->service_data, ble->service_data_len); append(&w, "\n"); }
        return w.used;
    }
    append(&w, "\nSEEN\nThis session · %lu observations\n", (unsigned long)observations);
    return w.used;
}

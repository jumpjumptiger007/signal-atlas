#ifndef RADIO_EXPLORER_REGISTRY_H
#define RADIO_EXPLORER_REGISTRY_H

#include <stddef.h>
#include <stdint.h>

const char *radio_ieee_lookup(const uint8_t mac[6], uint8_t *matched_bits);
const char *radio_company_lookup(uint16_t company_id);
const char *radio_service_lookup(const uint8_t uuid[16]);
void radio_service_uuid16_expand(uint16_t uuid, uint8_t out[16]);
const char *radio_appearance_lookup(uint16_t appearance);
const char *radio_registry_build_id(void);
size_t radio_registry_ieee_record_count(void);
size_t radio_registry_bluetooth_record_count(void);

#endif

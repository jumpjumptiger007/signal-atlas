#ifndef RADIO_EXPLORER_HISTORY_STORE_H
#define RADIO_EXPLORER_HISTORY_STORE_H

#include "history_codec.h"
#include "esp_err.h"

esp_err_t radio_history_store_load(radio_history_record_t *records,
                                   size_t capacity, size_t *count,
                                   uint32_t *session_id, uint32_t *generation);
esp_err_t radio_history_store_save(const radio_history_record_t *records,
                                   size_t count, uint32_t session_id,
                                   uint32_t generation);
esp_err_t radio_history_store_prepare(const radio_history_record_t *records,
                                      size_t count, uint32_t session_id,
                                      uint32_t generation);
esp_err_t radio_history_store_commit(void);
esp_err_t radio_history_store_clear(void);

#endif

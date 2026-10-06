#include "history_store.h"

#include "nvs.h"

#include <string.h>

static const char *NAMESPACE = "radio_exp";
static const char *KEY = "history";
static uint8_t s_blob[RADIO_HISTORY_BLOB_BYTES];
static size_t s_blob_length;

esp_err_t radio_history_store_load(radio_history_record_t *records,
                                   size_t capacity, size_t *count,
                                   uint32_t *session_id, uint32_t *generation)
{
    if (count != NULL) *count = 0;
    nvs_handle_t handle;
    esp_err_t error = nvs_open(NAMESPACE, NVS_READONLY, &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) return ESP_ERR_NOT_FOUND;
    if (error != ESP_OK) return error;
    size_t length = sizeof(s_blob);
    error = nvs_get_blob(handle, KEY, s_blob, &length);
    nvs_close(handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) return ESP_ERR_NOT_FOUND;
    if (error != ESP_OK) return error;
    const radio_history_decode_result_t result = radio_history_codec_decode(
        s_blob, length, session_id, generation, records, capacity, count);
    if (result == RADIO_HISTORY_DECODE_OK || result == RADIO_HISTORY_DECODE_EMPTY) return ESP_OK;
    return result == RADIO_HISTORY_DECODE_INCOMPATIBLE ? ESP_ERR_INVALID_VERSION : ESP_ERR_INVALID_CRC;
}

esp_err_t radio_history_store_prepare(const radio_history_record_t *records,
                                      size_t count, uint32_t session_id,
                                      uint32_t generation)
{
    size_t length = 0;
    if (!radio_history_codec_encode(session_id, generation, records, count,
                                    s_blob, sizeof(s_blob), &length)) return ESP_ERR_INVALID_ARG;
    s_blob_length = length;
    return ESP_OK;
}

esp_err_t radio_history_store_commit(void)
{
    if (s_blob_length < RADIO_HISTORY_HEADER_BYTES || s_blob_length > sizeof(s_blob))
        return ESP_ERR_INVALID_STATE;
    nvs_handle_t handle;
    esp_err_t error = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (error != ESP_OK) return error;
    error = nvs_set_blob(handle, KEY, s_blob, s_blob_length);
    if (error == ESP_OK) error = nvs_commit(handle);
    nvs_close(handle);
    return error;
}

esp_err_t radio_history_store_save(const radio_history_record_t *records,
                                   size_t count, uint32_t session_id,
                                   uint32_t generation)
{
    esp_err_t error = radio_history_store_prepare(records, count, session_id, generation);
    return error == ESP_OK ? radio_history_store_commit() : error;
}

esp_err_t radio_history_store_clear(void)
{
    nvs_handle_t handle;
    esp_err_t error = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (error != ESP_OK) return error;
    error = nvs_erase_key(handle, KEY);
    if (error == ESP_ERR_NVS_NOT_FOUND) error = ESP_OK;
    if (error == ESP_OK) error = nvs_commit(handle);
    nvs_close(handle);
    return error;
}

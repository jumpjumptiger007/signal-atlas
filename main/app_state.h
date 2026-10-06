#ifndef RADIO_EXPLORER_APP_STATE_H
#define RADIO_EXPLORER_APP_STATE_H

#include "model/nearby_store.h"
#include "storage/history_codec.h"

typedef enum { RE_SCREEN_NEARBY = 0, RE_SCREEN_DETAIL, RE_SCREEN_HISTORY } re_screen_t;
typedef enum { RE_DETAIL_NEARBY = 0, RE_DETAIL_HISTORY } re_detail_source_t;
typedef enum { RE_MODAL_NONE = 0, RE_MODAL_QUICK_MENU, RE_MODAL_FILTER,
               RE_MODAL_CLEAR_HISTORY, RE_MODAL_ABOUT } re_modal_t;
typedef enum { RE_MODE_ALL = 0, RE_MODE_WIFI, RE_MODE_BLE } re_mode_t;
typedef enum { RE_BACKEND_STOPPED = 0, RE_BACKEND_STARTING, RE_BACKEND_SCANNING,
               RE_BACKEND_IDLE, RE_BACKEND_FAILED, RE_BACKEND_UNAVAILABLE } re_backend_state_t;
typedef enum { RE_BUTTON_UP = 0, RE_BUTTON_DOWN, RE_BUTTON_OK_CLICK, RE_BUTTON_OK_LONG,
               RE_BUTTON_OTHER } re_button_t;

#define RE_HISTORY_VOLATILE_CAPACITY RADIO_HISTORY_CAPACITY

typedef struct {
    nearby_store_t nearby;
    radio_history_record_t history[RE_HISTORY_VOLATILE_CAPACITY];
    size_t history_count;
    uint32_t session_id;
    uint32_t history_generation;
    re_screen_t screen;
    re_detail_source_t detail_source;
    re_modal_t modal;
    re_screen_t modal_return_screen;
    re_mode_t mode;
    re_backend_state_t backend;
    int nearby_selection;
    int history_selection;
    int modal_selection;
    uint32_t sequence;
    bool history_seeded;
    bool overflow_seen;
    bool empty_scenario;
    bool scan_paused;
    bool dirty;
    bool history_dirty;
    bool history_flush_now;
} re_app_state_t;

void re_app_state_init(re_app_state_t *state, uint32_t freeze_timeout);
void re_app_state_set_backend(re_app_state_t *state, re_backend_state_t backend);
void re_app_state_set_mode(re_app_state_t *state, re_mode_t mode);
void re_app_state_toggle_pause(re_app_state_t *state);
void re_app_state_observe(re_app_state_t *state, const radio_observation_t *observation,
                          uint32_t stale_after, uint32_t expire_after);
void re_app_state_seed_history(re_app_state_t *state);
void re_app_state_restore_history(re_app_state_t *state,
                                  const radio_history_record_t *records,
                                  size_t count, uint32_t session_id,
                                  uint32_t generation);
void re_app_state_clear_history(re_app_state_t *state);
void re_app_state_handle_button(re_app_state_t *state, re_button_t button);
bool re_app_state_radar_active(const re_app_state_t *state);
bool re_app_state_has_visible_nearby(const re_app_state_t *state);
bool re_app_state_is_new(const re_app_state_t *state, const radio_identity_t *identity);
size_t re_app_state_nearby_visible_count(const re_app_state_t *state);
int re_app_state_nearby_visible_at(const re_app_state_t *state, size_t rank);
size_t re_app_state_nearby_visible_rank(const re_app_state_t *state, int record_index);
size_t re_app_state_nearby_window(const re_app_state_t *state, size_t max_rows, int *indices);
size_t re_app_state_history_order(const re_app_state_t *state, int *indices, size_t capacity);
size_t re_app_state_history_rank(const re_app_state_t *state, int history_index);
size_t re_app_state_history_window(const re_app_state_t *state, size_t max_rows, int *indices);

#endif

#include "ui_app.h"

#include "ui_radar.h"
#include "ui_theme.h"
#include "detail_format.h"
#include "../identify/registry.h"
#include "../model/observation.h"

#include <stdio.h>
#include <string.h>

enum { ROWS = 6 };
static lv_obj_t *s_screen;
static lv_obj_t *s_header;
static lv_obj_t *s_title;
static lv_obj_t *s_status;
static lv_obj_t *s_battery;
static lv_obj_t *s_rows[ROWS];
static lv_obj_t *s_labels[ROWS];
static lv_obj_t *s_footer;
static lv_obj_t *s_detail;
static lv_obj_t *s_modal;
static lv_obj_t *s_modal_options[5];
static re_screen_t s_built_screen = (re_screen_t)-1;
static re_modal_t s_built_modal = (re_modal_t)-1;
static int s_battery_soc = -1;

static lv_color_t color(uint32_t rgb) { return lv_color_hex(rgb); }

static lv_obj_t *label_create(lv_obj_t *parent, const char *text, uint32_t rgb, const lv_font_t *font)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color(rgb), 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    return label;
}

static void style_screen(lv_obj_t *screen)
{
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, color(RE_COLOR_BACKGROUND), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

static const char *mode_name(re_mode_t mode)
{
    static const char *const names[] = { "ALL", "WI-FI", "BLE" };
    return mode <= RE_MODE_BLE ? names[mode] : "ALL";
}

static const char *backend_name(const re_app_state_t *state)
{
    if (state->backend == RE_BACKEND_FAILED) return "BACKEND ERROR";
    if (state->backend == RE_BACKEND_UNAVAILABLE) return "RADIO BACKEND NOT IN THIS GATE";
    if (state->backend == RE_BACKEND_IDLE) return "PAUSED";
    if (state->backend == RE_BACKEND_STARTING) return "STARTING";
    if (state->backend == RE_BACKEND_SCANNING) return "SCANNING";
    return "STOPPED";
}

static void format_identity(const radio_identity_t *id, char *dst, size_t cap)
{
    if (id == NULL || cap == 0) return;
    (void)snprintf(dst, cap, "%02X:%02X:%02X:%02X:%02X:%02X",
                   id->address[0], id->address[1], id->address[2],
                   id->address[3], id->address[4], id->address[5]);
}

static void safe_bytes(const uint8_t *src, size_t len, char *dst, size_t cap)
{
    if (dst == NULL || cap == 0) return;
    size_t n = len < cap - 1 ? len : cap - 1;
    for (size_t i = 0; i < n; ++i) {
        uint8_t c = src[i];
        dst[i] = (c >= 0x20 && c <= 0x7e) ? (char)c : '?';
    }
    dst[n] = '\0';
}

static void nearby_name(const re_app_state_t *state, const nearby_record_t *record,
                        char *dst, size_t cap)
{
    if (record->observation.kind == RADIO_KIND_WIFI) {
        const radio_wifi_data_t *wifi = &record->observation.data.wifi;
        if (wifi->hidden || wifi->ssid_len == 0) (void)snprintf(dst, cap, "Hidden network");
        else safe_bytes(wifi->ssid, wifi->ssid_len, dst, cap);
    } else if (record->observation.data.ble.name_len != 0) {
        safe_bytes(record->observation.data.ble.name, record->observation.data.ble.name_len, dst, cap);
    } else {
        (void)snprintf(dst, cap, "BLE %02X:%02X:%02X",
                       record->observation.identity.address[3],
                       record->observation.identity.address[4],
                       record->observation.identity.address[5]);
    }
    (void)state;
}

static void build_header(re_app_state_t *state)
{
    s_header = lv_obj_create(s_screen);
    lv_obj_remove_style_all(s_header);
    lv_obj_set_size(s_header, 240, 42);
    lv_obj_set_style_bg_color(s_header, color(RE_COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(s_header, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_header, 0, 0);
    lv_obj_clear_flag(s_header, LV_OBJ_FLAG_SCROLLABLE);
    const char *heading = state->screen == RE_SCREEN_NEARBY ? "SIGNAL ATLAS" :
                          state->screen == RE_SCREEN_DETAIL ? "DETAIL" : "HISTORY";
    s_title = label_create(s_header, heading, RE_COLOR_ACCENT, &lv_font_montserrat_16);
    lv_obj_set_pos(s_title, 10, 7);
    if (state->screen == RE_SCREEN_NEARBY) {
        re_ui_radar_create(s_header);
        lv_obj_set_pos(lv_obj_get_child(s_header, -1), 205, 9);
    }
    char battery_text[12] = "";
    if (s_battery_soc >= 0 && s_battery_soc <= 100)
        (void)snprintf(battery_text, sizeof(battery_text), "%d%%", s_battery_soc);
    s_battery = label_create(s_header, battery_text, RE_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(s_battery, state->screen == RE_SCREEN_NEARBY ? 165 : 188, 11);
    if (state->screen == RE_SCREEN_NEARBY) {
        char text[40];
        (void)snprintf(text, sizeof(text), "%s  ·  %s", mode_name(state->mode), backend_name(state));
        s_status = label_create(s_screen, text, RE_COLOR_MUTED, &lv_font_montserrat_12);
        lv_obj_set_pos(s_status, 12, 46);
    }
}

static void build_list_rows(void)
{
    for (int i = 0; i < ROWS; ++i) {
        s_rows[i] = lv_obj_create(s_screen);
        lv_obj_remove_style_all(s_rows[i]);
        lv_obj_set_size(s_rows[i], 220, 32);
        lv_obj_set_style_radius(s_rows[i], 4, 0);
        lv_obj_set_style_pad_hor(s_rows[i], 7, 0);
        lv_obj_set_style_bg_opa(s_rows[i], LV_OPA_COVER, 0);
        lv_obj_clear_flag(s_rows[i], LV_OBJ_FLAG_SCROLLABLE);
        s_labels[i] = label_create(s_rows[i], "", RE_COLOR_TEXT, &lv_font_montserrat_14);
        lv_obj_set_width(s_labels[i], 205);
        lv_obj_center(s_labels[i]);
        lv_obj_set_pos(s_rows[i], 10, 68 + i * 36);
    }
    s_footer = label_create(s_screen, "UP/DOWN  MOVE     OK  OPEN     HOLD  MENU", RE_COLOR_MUTED, &lv_font_montserrat_10);
    lv_obj_set_pos(s_footer, 10, 296);
}

static const nearby_record_t *selected_nearby(const re_app_state_t *state)
{
    int i = state->nearby_selection >= 0 ? state->nearby_selection : state->nearby.selected_index;
    if (i < 0 || i >= (int)RADIO_NEARBY_CAPACITY || !state->nearby.records[i].occupied) return NULL;
    return &state->nearby.records[i];
}

static void build_detail_text(const re_app_state_t *state, char *text, size_t cap)
{
    const nearby_record_t *record = selected_nearby(state);
    if (state->detail_source == RE_DETAIL_HISTORY) {
        const radio_history_record_t *h = state->history_selection >= 0 &&
            state->history_selection < (int)RE_HISTORY_VOLATILE_CAPACITY ? &state->history[state->history_selection] : NULL;
        if (h == NULL || !h->occupied) {
            (void)radio_detail_format(NULL, NULL, 0, 0, 0, text, cap);
            return;
        }
        record = NULL;
        for (size_t i = 0; i < RADIO_NEARBY_CAPACITY; ++i)
            if (state->nearby.records[i].occupied && radio_identity_equal(&state->nearby.records[i].observation.identity, &h->identity)) {
                record = &state->nearby.records[i]; break;
            }
        if (record == NULL) {
            (void)radio_detail_format(NULL, &h->identity, h->identity.kind,
                                      h->last_rssi, h->observations, text, cap);
            return;
        }
        (void)radio_detail_format(&record->observation, &h->identity, h->identity.kind,
                                  h->last_rssi, h->observations, text, cap);
        return;
    }
    if (record == NULL) (void)radio_detail_format(NULL, NULL, 0, 0, 0, text, cap);
    else (void)radio_detail_format(&record->observation, &record->observation.identity,
                                  record->observation.kind, record->observation.rssi,
                                  record->observations, text, cap);
}

static void create_primary(re_app_state_t *state)
{
    lv_obj_t *old_screen = s_screen;
    re_ui_radar_set_active(false);
    re_ui_radar_reset();
    s_modal = NULL;
    s_screen = lv_obj_create(NULL);
    style_screen(s_screen);
    s_header = s_title = s_status = s_battery = s_footer = s_detail = NULL;
    memset(s_rows, 0, sizeof(s_rows)); memset(s_labels, 0, sizeof(s_labels));
    build_header(state);
    if (state->screen == RE_SCREEN_NEARBY || state->screen == RE_SCREEN_HISTORY) {
        build_list_rows();
    } else {
        s_detail = lv_label_create(s_screen);
        lv_obj_set_style_text_color(s_detail, color(RE_COLOR_TEXT), 0);
        lv_obj_set_style_text_font(s_detail, &lv_font_montserrat_14, 0);
        lv_label_set_long_mode(s_detail, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(s_detail, 214);
        lv_obj_set_height(s_detail, 244);
        lv_obj_set_pos(s_detail, 12, 52);
        lv_obj_add_flag(s_detail, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scroll_dir(s_detail, LV_DIR_VER);
        s_footer = label_create(s_screen, "UP/DOWN  SCROLL             HOLD  BACK", RE_COLOR_MUTED, &lv_font_montserrat_10);
        lv_obj_set_pos(s_footer, 10, 296);
    }
    lv_screen_load(s_screen);
    if (old_screen != NULL) lv_obj_delete(old_screen);
    s_built_screen = state->screen;
    s_built_modal = (re_modal_t)-1;
}

static void refresh_list(re_app_state_t *state)
{
    if (state->screen == RE_SCREEN_NEARBY) {
        int visible_indices[ROWS];
        int visible = (int)re_app_state_nearby_window(state, ROWS, visible_indices);
        for (int row = 0; row < visible; ++row) {
            int index = visible_indices[row];
            const nearby_record_t *record = &state->nearby.records[index];
            char name[40], identity[20], line[80];
            nearby_name(state, record, name, sizeof(name));
            format_identity(&record->observation.identity, identity, sizeof(identity));
            (void)identity;
            (void)snprintf(line, sizeof(line), "%c  %-18.18s %s%d",
                record->observation.kind == RADIO_KIND_WIFI ? 'W' : 'B', name,
                re_app_state_is_new(state, &record->observation.identity) ? "NEW " : "", record->observation.rssi);
            lv_label_set_text(s_labels[row], line);
            lv_obj_set_style_bg_color(s_rows[row], index == state->nearby_selection ? color(RE_COLOR_PANEL_ALT) : color(RE_COLOR_PANEL), 0);
            lv_obj_remove_flag(s_rows[row], LV_OBJ_FLAG_HIDDEN);
        }
        for (int i = visible; i < ROWS; ++i) lv_obj_add_flag(s_rows[i], LV_OBJ_FLAG_HIDDEN);
        if (visible == 0) {
            const char *empty = state->backend == RE_BACKEND_FAILED ? "SCANNER ERROR · HOLD MENU" :
                state->backend == RE_BACKEND_UNAVAILABLE ? "WI-FI SCANNER AVAILABLE NEXT GATE" :
                state->mode == RE_MODE_WIFI ? "NO WI-FI RESULTS · BACKEND NOT IN THIS GATE" :
                "NO OBSERVATIONS YET";
            lv_label_set_text(s_labels[0], empty);
            lv_obj_remove_flag(s_rows[0], LV_OBJ_FLAG_HIDDEN);
        }
        if (state->overflow_seen) lv_label_set_text(s_footer, "CAPACITY FULL · OLDEST SUITABLE ENTRY ROLLS OFF");
        char status[48];
        (void)snprintf(status, sizeof(status), "%s  ·  %s  ·  %02u", mode_name(state->mode), backend_name(state), (unsigned)state->nearby.count);
        lv_label_set_text(s_status, status);
        re_ui_radar_set_active(re_app_state_radar_active(state));
    } else {
        int window[ROWS];
        size_t visible = re_app_state_history_window(state, ROWS, window);
        for (size_t row = 0; row < visible; ++row) {
            int index = window[row];
            const radio_history_record_t *h = &state->history[index];
            char address[20], line[80]; format_identity(&h->identity, address, sizeof(address));
            (void)snprintf(line, sizeof(line), "%c  %-18s %d dBm",
                h->identity.kind == RADIO_KIND_WIFI ? 'W' : 'B', address, h->last_rssi);
            lv_label_set_text(s_labels[row], line);
            lv_obj_set_style_bg_color(s_rows[row], index == state->history_selection ? color(RE_COLOR_PANEL_ALT) : color(RE_COLOR_PANEL), 0);
            lv_obj_remove_flag(s_rows[row], LV_OBJ_FLAG_HIDDEN);
        }
        for (size_t i = visible; i < ROWS; ++i) lv_obj_add_flag(s_rows[i], LV_OBJ_FLAG_HIDDEN);
        if (visible == 0) { lv_label_set_text(s_labels[0], "NO LOCAL HISTORY"); lv_obj_remove_flag(s_rows[0], LV_OBJ_FLAG_HIDDEN); }
    }
}

static void modal_refresh(re_app_state_t *state)
{
    static const char *const quick[] = { "Filter", "History", "Pause / Resume", "Clear History", "About" };
    static const char *const modes[] = { "ALL", "WI-FI", "BLE" };
    static const char *const confirm[] = { "Clear history", "Cancel" };
    if (state->modal != s_built_modal) {
        if (s_modal != NULL) lv_obj_delete(s_modal);
        s_modal = NULL;
        memset(s_modal_options, 0, sizeof(s_modal_options));
        if (state->modal == RE_MODAL_NONE) { s_built_modal = RE_MODAL_NONE; return; }
        s_modal = lv_obj_create(s_screen);
        lv_obj_remove_style_all(s_modal);
        lv_obj_set_size(s_modal, 216, 220);
        lv_obj_set_pos(s_modal, 12, 52);
        lv_obj_set_style_radius(s_modal, 8, 0);
        lv_obj_set_style_border_width(s_modal, 1, 0);
        lv_obj_set_style_border_color(s_modal, color(RE_COLOR_ACCENT), 0);
        lv_obj_set_style_bg_color(s_modal, color(RE_COLOR_PANEL), 0);
        lv_obj_set_style_bg_opa(s_modal, LV_OPA_COVER, 0);
        lv_obj_clear_flag(s_modal, LV_OBJ_FLAG_SCROLLABLE);
        const char *title = state->modal == RE_MODAL_QUICK_MENU ? "QUICK MENU" :
                            state->modal == RE_MODAL_FILTER ? "FILTER / MODE" :
                            state->modal == RE_MODAL_CLEAR_HISTORY ? "CLEAR HISTORY?" : "ABOUT";
        lv_obj_t *modal_title = label_create(s_modal, title, RE_COLOR_ACCENT, &lv_font_montserrat_16);
        lv_obj_set_pos(modal_title, 12, 10);
        if (state->modal == RE_MODAL_ABOUT) {
            char about[128];
            (void)snprintf(about, sizeof(about), "Signal Atlas v0.1\nOffline · read-only\nRegistry %s", radio_registry_build_id());
            lv_obj_t *modal_body = label_create(s_modal, about, RE_COLOR_TEXT, &lv_font_montserrat_14);
            lv_obj_set_pos(modal_body, 12, 50);
        } else {
            int n = state->modal == RE_MODAL_QUICK_MENU ? 5 : state->modal == RE_MODAL_FILTER ? 3 : 2;
            for (int i = 0; i < n; ++i) {
                const char *text = state->modal == RE_MODAL_QUICK_MENU ? quick[i] :
                                   state->modal == RE_MODAL_FILTER ? modes[i] : confirm[i];
                s_modal_options[i] = label_create(s_modal, text, RE_COLOR_TEXT, &lv_font_montserrat_14);
                lv_obj_set_pos(s_modal_options[i], 14, 44 + i * 29);
                lv_obj_set_width(s_modal_options[i], 184);
            }
        }
        s_built_modal = state->modal;
    }
    if (s_modal_options[0] != NULL) {
        for (size_t i = 0; i < 5; ++i) if (s_modal_options[i] != NULL)
            lv_obj_set_style_text_color(s_modal_options[i], (int)i == state->modal_selection ? color(RE_COLOR_ACCENT) : color(RE_COLOR_TEXT), 0);
    }
}

void re_ui_init(re_app_state_t *state)
{
    (void)state;
    s_screen = NULL; s_modal = NULL; s_built_screen = (re_screen_t)-1; s_built_modal = (re_modal_t)-1;
    (void)lv_timer_create(re_ui_radar_tick, 143, NULL);
}

void re_ui_render(re_app_state_t *state, int battery_soc)
{
    if (state == NULL) return;
    if (s_battery_soc != battery_soc) {
        s_battery_soc = battery_soc;
        if (s_battery != NULL && battery_soc >= 0 && battery_soc <= 100)
            lv_label_set_text_fmt(s_battery, "%d%%", battery_soc);
    }
    if (s_screen == NULL || s_built_screen != state->screen) create_primary(state);
    if (state->screen == RE_SCREEN_NEARBY || state->screen == RE_SCREEN_HISTORY) refresh_list(state);
    else if (s_detail != NULL) {
        char text[1800]; build_detail_text(state, text, sizeof(text)); lv_label_set_text(s_detail, text);
    }
    modal_refresh(state);
}

void re_ui_scroll_detail(int direction)
{
    if (s_detail == NULL || direction == 0) return;
    lv_obj_scroll_by(s_detail, 0, direction < 0 ? -32 : 32, LV_ANIM_OFF);
}

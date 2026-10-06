#include "ui_radar.h"

#include "ui_theme.h"

static lv_obj_t *s_scope;
static lv_obj_t *s_sweep;
static lv_obj_t *s_blips[2];
static bool s_active;
static uint8_t s_phase;

void re_ui_radar_reset(void)
{
    s_scope = NULL;
    s_sweep = NULL;
    s_blips[0] = NULL;
    s_blips[1] = NULL;
    s_active = false;
}

void re_ui_radar_create(lv_obj_t *parent)
{
    re_ui_radar_reset();
    s_scope = lv_obj_create(parent);
    lv_obj_set_size(s_scope, 24, 24);
    lv_obj_remove_style_all(s_scope);
    lv_obj_set_style_radius(s_scope, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_scope, 1, 0);
    lv_obj_set_style_border_color(s_scope, lv_color_hex(RE_COLOR_ACCENT), 0);
    lv_obj_set_style_bg_opa(s_scope, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(s_scope, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *cross = lv_obj_create(s_scope);
    lv_obj_remove_style_all(cross);
    lv_obj_set_size(cross, 1, 16);
    lv_obj_set_style_bg_color(cross, lv_color_hex(RE_COLOR_PANEL_ALT), 0);
    lv_obj_set_style_bg_opa(cross, LV_OPA_COVER, 0);
    lv_obj_center(cross);
    lv_obj_t *cross_h = lv_obj_create(s_scope);
    lv_obj_remove_style_all(cross_h);
    lv_obj_set_size(cross_h, 16, 1);
    lv_obj_set_style_bg_color(cross_h, lv_color_hex(RE_COLOR_PANEL_ALT), 0);
    lv_obj_set_style_bg_opa(cross_h, LV_OPA_COVER, 0);
    lv_obj_center(cross_h);
    static lv_point_precise_t points[2];
    s_sweep = lv_line_create(s_scope);
    points[0] = (lv_point_precise_t){ 12, 12 };
    points[1] = (lv_point_precise_t){ 12, 1 };
    lv_line_set_points(s_sweep, points, 2);
    lv_obj_set_style_line_color(s_sweep, lv_color_hex(RE_COLOR_ACCENT), 0);
    lv_obj_set_style_line_width(s_sweep, 1, 0);
    for (size_t i = 0; i < 2; ++i) {
        s_blips[i] = lv_obj_create(s_scope);
        lv_obj_remove_style_all(s_blips[i]);
        lv_obj_set_size(s_blips[i], 3, 3);
        lv_obj_set_style_radius(s_blips[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(s_blips[i], lv_color_hex(RE_COLOR_ACCENT), 0);
        lv_obj_set_style_bg_opa(s_blips[i], LV_OPA_COVER, 0);
        lv_obj_set_pos(s_blips[i], i == 0 ? 16 : 5, i == 0 ? 6 : 15);
    }
    s_active = false;
    lv_obj_add_flag(s_scope, LV_OBJ_FLAG_HIDDEN);
}

void re_ui_radar_set_active(bool active)
{
    s_active = active;
    if (s_scope == NULL) return;
    if (active) lv_obj_remove_flag(s_scope, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(s_scope, LV_OBJ_FLAG_HIDDEN);
}

void re_ui_radar_tick(lv_timer_t *timer)
{
    (void)timer;
    if (!s_active || s_sweep == NULL) return;
    static const lv_point_precise_t ends[8] = {
        { 12, 1 }, { 20, 4 }, { 23, 12 }, { 20, 20 },
        { 12, 23 }, { 4, 20 }, { 1, 12 }, { 4, 4 },
    };
    static lv_point_precise_t line_points[2];
    line_points[0] = (lv_point_precise_t){ 12, 12 };
    line_points[1] = ends[s_phase & 7u];
    lv_line_set_points(s_sweep, line_points, 2);
    ++s_phase;
    if (s_blips[0] != NULL) {
        if ((s_phase & 3u) == 0) lv_obj_add_flag(s_blips[0], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_remove_flag(s_blips[0], LV_OBJ_FLAG_HIDDEN);
        if ((s_phase & 7u) == 3) lv_obj_remove_flag(s_blips[1], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_blips[1], LV_OBJ_FLAG_HIDDEN);
    }
}

#ifndef RADIO_EXPLORER_UI_RADAR_H
#define RADIO_EXPLORER_UI_RADAR_H

#include "lvgl.h"
#include "../app_state.h"

void re_ui_radar_create(lv_obj_t *parent);
void re_ui_radar_reset(void);
void re_ui_radar_set_active(bool active);
void re_ui_radar_tick(lv_timer_t *timer);

#endif

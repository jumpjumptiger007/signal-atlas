#ifndef RADIO_EXPLORER_UI_APP_H
#define RADIO_EXPLORER_UI_APP_H

#include "../app_state.h"

void re_ui_init(re_app_state_t *state);
void re_ui_render(re_app_state_t *state, int battery_soc);
void re_ui_scroll_detail(int direction);

#endif

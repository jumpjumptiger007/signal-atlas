#ifndef RADIO_EXPLORER_APP_H
#define RADIO_EXPLORER_APP_H

#include "bsp_button.h"
#include "esp_err.h"

esp_err_t radio_explorer_app_init(void);
void radio_explorer_app_button(bsp_btn_t button, bsp_btn_ev_t event);

#endif

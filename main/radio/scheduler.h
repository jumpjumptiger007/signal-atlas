#ifndef RADIO_EXPLORER_SCHEDULER_H
#define RADIO_EXPLORER_SCHEDULER_H

#include "../app_state.h"

typedef enum {
    RADIO_SCHEDULE_STOPPED = 0,
    RADIO_SCHEDULE_WIFI,
    RADIO_SCHEDULE_BLE,
} radio_schedule_phase_t;

enum {
    RADIO_SCHEDULE_BLE_WINDOW_MS = 5000,
    RADIO_SCHEDULE_WIFI_WINDOW_MS = 2500,
};

typedef struct {
    re_mode_t previous_mode;
    radio_schedule_phase_t phase;
    uint32_t phase_started_ms;
    bool initialized;
} radio_scheduler_t;

void radio_scheduler_init(radio_scheduler_t *scheduler);
radio_schedule_phase_t radio_scheduler_step(radio_scheduler_t *scheduler,
                                             re_mode_t mode, bool paused,
                                             uint32_t now_ms);

#endif

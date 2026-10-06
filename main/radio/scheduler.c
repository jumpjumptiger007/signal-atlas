#include "scheduler.h"

#include <string.h>

void radio_scheduler_init(radio_scheduler_t *scheduler)
{
    if (scheduler != NULL) memset(scheduler, 0, sizeof(*scheduler));
}

radio_schedule_phase_t radio_scheduler_step(radio_scheduler_t *scheduler,
                                             re_mode_t mode, bool paused,
                                             uint32_t now_ms)
{
    if (scheduler == NULL || paused) return RADIO_SCHEDULE_STOPPED;
    if (mode == RE_MODE_WIFI) {
        scheduler->initialized = true;
        scheduler->previous_mode = mode;
        scheduler->phase = RADIO_SCHEDULE_WIFI;
        scheduler->phase_started_ms = now_ms;
        return RADIO_SCHEDULE_WIFI;
    }
    if (mode == RE_MODE_BLE) {
        scheduler->initialized = true;
        scheduler->previous_mode = mode;
        scheduler->phase = RADIO_SCHEDULE_BLE;
        scheduler->phase_started_ms = now_ms;
        return RADIO_SCHEDULE_BLE;
    }

    if (!scheduler->initialized || scheduler->previous_mode != RE_MODE_ALL) {
        scheduler->initialized = true;
        scheduler->previous_mode = RE_MODE_ALL;
        scheduler->phase = RADIO_SCHEDULE_BLE;
        scheduler->phase_started_ms = now_ms;
    } else {
        const uint32_t elapsed = now_ms - scheduler->phase_started_ms;
        const uint32_t duration = scheduler->phase == RADIO_SCHEDULE_BLE ?
            RADIO_SCHEDULE_BLE_WINDOW_MS : RADIO_SCHEDULE_WIFI_WINDOW_MS;
        if (elapsed >= duration) {
            scheduler->phase = scheduler->phase == RADIO_SCHEDULE_BLE ?
                RADIO_SCHEDULE_WIFI : RADIO_SCHEDULE_BLE;
            scheduler->phase_started_ms = now_ms;
        }
    }
    return scheduler->phase;
}

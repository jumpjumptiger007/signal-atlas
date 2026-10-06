#include "radio/scheduler.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    radio_scheduler_t scheduler;
    radio_scheduler_init(&scheduler);
    assert(radio_scheduler_step(&scheduler, RE_MODE_ALL, false, 100) == RADIO_SCHEDULE_BLE);
    assert(radio_scheduler_step(&scheduler, RE_MODE_ALL, false, 5099) == RADIO_SCHEDULE_BLE);
    assert(radio_scheduler_step(&scheduler, RE_MODE_ALL, false, 5100) == RADIO_SCHEDULE_WIFI);
    assert(radio_scheduler_step(&scheduler, RE_MODE_ALL, false, 7599) == RADIO_SCHEDULE_WIFI);
    assert(radio_scheduler_step(&scheduler, RE_MODE_ALL, false, 7600) == RADIO_SCHEDULE_BLE);
    assert(radio_scheduler_step(&scheduler, RE_MODE_WIFI, false, 7800) == RADIO_SCHEDULE_WIFI);
    assert(radio_scheduler_step(&scheduler, RE_MODE_BLE, false, 7900) == RADIO_SCHEDULE_BLE);
    assert(radio_scheduler_step(&scheduler, RE_MODE_ALL, true, 8000) == RADIO_SCHEDULE_STOPPED);
    assert(radio_scheduler_step(&scheduler, RE_MODE_ALL, false, 8100) == RADIO_SCHEDULE_BLE);

    radio_scheduler_init(&scheduler);
    scheduler.initialized = true;
    scheduler.previous_mode = RE_MODE_ALL;
    scheduler.phase = RADIO_SCHEDULE_BLE;
    scheduler.phase_started_ms = UINT32_MAX - 1000u;
    assert(radio_scheduler_step(&scheduler, RE_MODE_ALL, false, 3998u) == RADIO_SCHEDULE_BLE);
    assert(radio_scheduler_step(&scheduler, RE_MODE_ALL, false, 3999u) == RADIO_SCHEDULE_WIFI);
    puts("Radio scheduler deterministic mode, pause, alternation, and wrap tests: PASS");
    return 0;
}

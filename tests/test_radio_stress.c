#include "app_state.h"
#include "radio/scheduler.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    re_app_state_t state;
    radio_scheduler_t scheduler;
    radio_history_record_t decoded[RADIO_HISTORY_CAPACITY];
    uint8_t serialized[RADIO_HISTORY_BLOB_BYTES];
    re_app_state_init(&state, 30);
    radio_scheduler_init(&scheduler);
    size_t serialized_length = 0;

    for (uint32_t cycle = 0; cycle < 50; ++cycle) {
        const re_mode_t mode = cycle % 3 == 0 ? RE_MODE_ALL :
                               cycle % 3 == 1 ? RE_MODE_WIFI : RE_MODE_BLE;
        const bool paused = cycle % 7 == 0;
        const radio_schedule_phase_t phase = radio_scheduler_step(&scheduler, mode, paused,
                                                                   cycle * 2500u);
        assert(paused ? phase == RADIO_SCHEDULE_STOPPED : phase != RADIO_SCHEDULE_STOPPED);
        state.mode = mode;
        state.scan_paused = paused;
        for (uint8_t n = 0; n < 12; ++n) {
            const uint16_t id = (uint16_t)((cycle * 12u + n) % 400u);
            radio_observation_t observation = {0};
            observation.kind = (id & 1u) ? RADIO_KIND_BLE : RADIO_KIND_WIFI;
            observation.identity.kind = (uint8_t)observation.kind;
            observation.identity.address_type = RADIO_ADDRESS_RANDOM;
            observation.identity.address[0] = (uint8_t)(0x80u | (id >> 8));
            observation.identity.address[1] = (uint8_t)id;
            observation.identity.address[5] = (uint8_t)(id >> 1);
            observation.rssi = (int8_t)(-35 - (n % 40));
            re_app_state_observe(&state, &observation, 80, 240);
        }
        assert(state.nearby.count <= RADIO_NEARBY_CAPACITY);
        assert(state.history_count <= RADIO_HISTORY_CAPACITY);
        assert(radio_history_codec_encode(state.session_id, state.history_generation,
            state.history, state.history_count, serialized, sizeof(serialized),
            &serialized_length));
        size_t restored_count = 0;
        uint32_t session_id = 0, generation = 0;
        assert(radio_history_codec_decode(serialized, serialized_length,
            &session_id, &generation, decoded, RADIO_HISTORY_CAPACITY,
            &restored_count) == RADIO_HISTORY_DECODE_OK);
        assert(restored_count == state.history_count && session_id == state.session_id);
    }
    assert(state.history_count == 400);
    puts("Signal Atlas 50-cycle mode/pause/observation/history serialization stress: PASS");
    return 0;
}

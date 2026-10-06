set(radio_explorer_sources
    "main.c"
    "app.c"
    "demo_radio.c"
    "app_state.c"
    "storage/history_codec.c"
    "storage/history_store.c"
    "radio/radio.c"
    "radio/scheduler.c"
    "radio/wifi_scanner.c"
    "ui/ui_app.c"
    "ui/detail_format.c"
    "ui/ui_radar.c"
    "model/observation.c"
    "model/nearby_store.c"
    "identify/ble_ad_parser.c"
    "identify/protocol_decoder.c"
    "identify/registry.c"
    "identify/registry_data.c"
)
option(SIGNAL_ATLAS_USE_LOCAL_IEEE_REGISTRY "Use developer-local IEEE MA-L/MA-M/MA-S data" OFF)
if(SIGNAL_ATLAS_USE_LOCAL_IEEE_REGISTRY)
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/identify/registry_data_ieee_local.c")
        message(FATAL_ERROR "Local IEEE registry requested but registry_data_ieee_local.c is missing")
    endif()
    list(APPEND radio_explorer_sources "identify/registry_data_ieee_local.c")
else()
    list(APPEND radio_explorer_sources "identify/registry_data_ieee_empty.c")
endif()
set(radio_explorer_requires bsp esp_timer esp_event esp_netif esp_wifi nvs_flash)

if(CONFIG_RADIO_EXPLORER_SIMULATOR)
    list(APPEND radio_explorer_sources "radio/mock_scanner.c")
else()
    list(APPEND radio_explorer_sources "radio/ble_scanner.c")
    list(APPEND radio_explorer_requires bt)
endif()

idf_component_register(
    SRCS ${radio_explorer_sources}
    INCLUDE_DIRS "."
    REQUIRES ${radio_explorer_requires}
)

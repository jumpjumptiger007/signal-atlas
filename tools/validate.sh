#!/usr/bin/env bash
set -euo pipefail

mode="${1:---all}"
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
    echo "Usage: $0 [--all|--static|--firmware]" >&2
}

run_static_checks() {
    local actionlint_bin
    local test_dir
    local section_gc_flag

    python3 tools/check_repo.py

    actionlint_bin="${ACTIONLINT_BIN:-}"
    if [[ -z "${actionlint_bin}" ]]; then
        actionlint_bin="$(command -v actionlint || true)"
    fi
    if [[ -z "${actionlint_bin}" || ! -x "${actionlint_bin}" ]]; then
        actionlint_bin="$(./tools/install-actionlint.sh)"
    fi
    "${actionlint_bin}" -color .github/workflows/*.yml

    test_dir="$(mktemp -d /tmp/ai-passport-host-tests.XXXXXX)"
    if [[ "$(uname -s)" == "Darwin" ]]; then
        section_gc_flag="-Wl,-dead_strip"
    else
        section_gc_flag="-Wl,--gc-sections"
    fi
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_ui_pixel_math.c main/ui_pixel_math.c \
        -o "${test_dir}/test_ui_pixel_math"
    "${test_dir}/test_ui_pixel_math"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_demo_navigation.c main/demo_navigation.c \
        -o "${test_dir}/test_demo_navigation"
    "${test_dir}/test_demo_navigation"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Icomponents/bsp/src \
        tests/test_bsp_display_rounding.c components/bsp/src/bsp_display_rounding.c \
        -o "${test_dir}/test_bsp_display_rounding"
    "${test_dir}/test_bsp_display_rounding"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Icomponents/bsp/src \
        tests/test_bsp_es8311_sleep_check.c components/bsp/src/bsp_es8311_sleep_check.c \
        -o "${test_dir}/test_bsp_es8311_sleep_check"
    "${test_dir}/test_bsp_es8311_sleep_check"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
        -Itests/bsp_stubs -Icomponents/bsp/include \
        tests/test_bsp_button.c -o "${test_dir}/test_bsp_button"
    "${test_dir}/test_bsp_button"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
        -Itests/bsp_stubs -Icomponents/bsp/include \
        tests/test_bsp_lvgl_init.c components/bsp/src/bsp_display_rounding.c \
        -o "${test_dir}/test_bsp_lvgl_init"
    "${test_dir}/test_bsp_lvgl_init"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
        -Itests/audio_stubs -Icomponents/bsp/include -Icomponents/bsp/src \
        tests/test_bsp_audio_recovery.c components/bsp/src/bsp_es8311_sleep_check.c \
        -o "${test_dir}/test_bsp_audio_recovery"
    "${test_dir}/test_bsp_audio_recovery"
    for demo in audio low_power ble wifi; do
        "${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
            -ffunction-sections -fdata-sections -Itests/demo_stubs -Imain \
            "tests/test_demo_${demo}_runtime.c" "${section_gc_flag}" \
            -o "${test_dir}/test_demo_${demo}_runtime"
        "${test_dir}/test_demo_${demo}_runtime"
    done
    PYTHONDONTWRITEBYTECODE=1 python3 tests/test_deep_sleep_contract.py
    PYTHONDONTWRITEBYTECODE=1 python3 tests/test_check_repo.py
    PYTHONDONTWRITEBYTECODE=1 python3 tests/test_verify_firmware.py
    PYTHONDONTWRITEBYTECODE=1 python3 tests/test_archive_firmware.py
    PYTHONDONTWRITEBYTECODE=1 python3 tests/test_install_passport_skills.py
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain/model -Imain/identify \
        tests/test_radio_model.c main/model/observation.c main/model/nearby_store.c \
        main/identify/ble_ad_parser.c main/identify/protocol_decoder.c \
        main/identify/registry.c main/identify/registry_data.c main/identify/registry_data_ieee_empty.c \
        -o "${test_dir}/test_radio_model"
    "${test_dir}/test_radio_model"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain -Imain/model -Imain/identify \
        tests/test_radio_app_state.c main/app_state.c main/model/observation.c main/model/nearby_store.c \
        -o "${test_dir}/test_radio_app_state"
    "${test_dir}/test_radio_app_state"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_radio_scheduler.c main/radio/scheduler.c -o "${test_dir}/test_radio_scheduler"
    "${test_dir}/test_radio_scheduler"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_history_codec.c main/storage/history_codec.c main/model/observation.c \
        -o "${test_dir}/test_history_codec"
    "${test_dir}/test_history_codec"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain -Imain/model \
        tests/test_radio_stress.c main/app_state.c main/radio/scheduler.c \
        main/model/observation.c main/model/nearby_store.c main/storage/history_codec.c \
        -o "${test_dir}/test_radio_stress"
    "${test_dir}/test_radio_stress"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain -Imain/model -Imain/identify \
        tests/test_radio_detail_format.c main/ui/detail_format.c main/model/observation.c \
        main/identify/protocol_decoder.c main/identify/registry.c main/identify/registry_data.c main/identify/registry_data_ieee_empty.c \
        -o "${test_dir}/test_radio_detail_format"
    "${test_dir}/test_radio_detail_format"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain -Imain/model -Imain/identify \
        tests/test_mock_scanner.c main/radio/radio.c main/radio/mock_scanner.c \
        main/model/observation.c main/identify/ble_ad_parser.c main/identify/protocol_decoder.c \
        -o "${test_dir}/test_mock_scanner"
    "${test_dir}/test_mock_scanner"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain -Imain/model \
        tests/test_radio_wifi_mapping.c main/radio/radio.c main/model/observation.c \
        main/identify/ble_ad_parser.c -o "${test_dir}/test_radio_wifi_mapping"
    "${test_dir}/test_radio_wifi_mapping"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
        -Itests/demo_stubs -Imain -Imain/model -Imain/identify \
        tests/test_radio_wifi_scanner.c main/radio/radio.c main/model/observation.c \
        main/identify/ble_ad_parser.c -o "${test_dir}/test_radio_wifi_scanner"
    "${test_dir}/test_radio_wifi_scanner"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
        -Itests/demo_stubs -Imain -Imain/model -Imain/identify \
        tests/test_radio_ble_scanner.c main/radio/radio.c main/model/observation.c \
        main/identify/ble_ad_parser.c -o "${test_dir}/test_radio_ble_scanner"
    "${test_dir}/test_radio_ble_scanner"
    PYTHONDONTWRITEBYTECODE=1 python3 tests/test_radio_app_contract.py
    PYTHONDONTWRITEBYTECODE=1 python3 tests/test_signal_atlas_identity.py
    PYTHONDONTWRITEBYTECODE=1 python3 tests/test_generate_registries.py
    rm -rf "${test_dir}"
    echo "Host tests: PASS"
}

run_firmware_checks() (
    local validation_build_dir

    if ! command -v idf.py >/dev/null 2>&1; then
        echo "ERROR: idf.py is not available; activate ESP-IDF 5.5.3 first." >&2
        return 1
    fi

    validation_build_dir="$(mktemp -d /tmp/ai-passport-firmware.XXXXXX)"
    trap 'case "${validation_build_dir}" in /tmp/ai-passport-firmware.*) rm -rf -- "${validation_build_dir}" ;; esac' EXIT

    SDKCONFIG_DEFAULTS="${repo_root}/sdkconfig.defaults" \
        idf.py -B "${validation_build_dir}" \
        -D "SDKCONFIG=${validation_build_dir}/sdkconfig" build
    idf.py -B "${validation_build_dir}" merge-bin \
        -o "${validation_build_dir}/FoloToy-AI-Passport-full.bin"
    python3 tools/verify_firmware.py "${validation_build_dir}"
    PYTHONDONTWRITEBYTECODE=1 python3 tools/archive_firmware.py create \
        "${validation_build_dir}" --archive-root "${repo_root}/build/firmware"
    mkdir -p "${repo_root}/build"
    install -m 0644 \
        "${validation_build_dir}/FoloToy-AI-Passport-full.bin" \
        "${repo_root}/build/FoloToy-AI-Passport-full.bin"
    echo "Firmware build: PASS"
)

cd "${repo_root}"
case "${mode}" in
    --all)
        run_static_checks
        run_firmware_checks
        ;;
    --static)
        run_static_checks
        ;;
    --firmware)
        run_firmware_checks
        ;;
    *)
        usage
        exit 2
        ;;
esac

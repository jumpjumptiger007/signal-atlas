#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
scenario="${1:-normal}"
if ! command -v idf.py >/dev/null 2>&1; then
    echo "ERROR: idf.py is unavailable; activate ESP-IDF 5.5.3 first." >&2
    exit 1
fi
idf_version="$(idf.py --version)"
if [[ "${idf_version}" != *"ESP-IDF v5.5.3"* ]]; then
    echo "ERROR: expected ESP-IDF v5.5.3, got: ${idf_version}" >&2
    exit 1
fi

case "${scenario}" in
    normal)
        scenario_config="CONFIG_RADIO_EXPLORER_MOCK_SCENARIO_NORMAL=y"
        scenario_defaults="${repo_root}/sdkconfig.simulator.defaults"
        artifact_name="FoloToy-AI-Passport-full.bin"
        ;;
    empty)
        scenario_config="CONFIG_RADIO_EXPLORER_MOCK_SCENARIO_EMPTY=y"
        scenario_defaults="${repo_root}/sdkconfig.simulator.empty.defaults"
        artifact_name="FoloToy-AI-Passport-empty-full.bin"
        ;;
    failure)
        scenario_config="CONFIG_RADIO_EXPLORER_MOCK_SCENARIO_FAILURE=y"
        scenario_defaults="${repo_root}/sdkconfig.simulator.failure.defaults"
        artifact_name="FoloToy-AI-Passport-failure-full.bin"
        ;;
    overflow)
        scenario_config="CONFIG_RADIO_EXPLORER_MOCK_SCENARIO_OVERFLOW=y"
        scenario_defaults="${repo_root}/sdkconfig.simulator.${scenario}.defaults"
        artifact_name="FoloToy-AI-Passport-overflow-full.bin"
        ;;
    *)
        echo "Usage: $0 [normal|empty|failure|overflow]" >&2
        exit 2
        ;;
esac
if [[ ! -f "${scenario_defaults}" ]]; then
    echo "ERROR: Simulator scenario defaults not found: ${scenario_defaults}" >&2
    exit 1
fi

build_dir="$(mktemp -d /tmp/radio-explorer-simulator.XXXXXX)"
trap 'case "${build_dir}" in /tmp/radio-explorer-simulator.*) rm -rf -- "${build_dir}" ;; esac' EXIT
SDKCONFIG_DEFAULTS="${repo_root}/sdkconfig.defaults;${repo_root}/sdkconfig.simulator.defaults;${scenario_defaults}" \
    idf.py -B "${build_dir}" -D "SDKCONFIG=${build_dir}/sdkconfig" build

grep -qx 'CONFIG_RADIO_EXPLORER_SIMULATOR=y' "${build_dir}/sdkconfig"
grep -qx "${scenario_config}" "${build_dir}/sdkconfig"
grep -qx 'CONFIG_ESP_CONSOLE_UART_DEFAULT=y' "${build_dir}/sdkconfig"
if grep -qx 'CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y' "${build_dir}/sdkconfig"; then
    echo "ERROR: Simulator profile must route runtime logs to UART0." >&2
    exit 1
fi
if grep -qx 'CONFIG_BT_ENABLED=y' "${build_dir}/sdkconfig"; then
    echo "ERROR: Simulator profile unexpectedly enables Bluetooth." >&2
    exit 1
fi

idf.py -B "${build_dir}" merge-bin -o "${build_dir}/${artifact_name}"
if [[ "${artifact_name}" != "FoloToy-AI-Passport-full.bin" ]]; then
    install -m 0644 "${build_dir}/${artifact_name}" \
        "${build_dir}/FoloToy-AI-Passport-full.bin"
fi
python3 "${repo_root}/tools/verify_firmware.py" "${build_dir}"
artifact_dir="${repo_root}/build/radio-explorer-simulator"
mkdir -p "${artifact_dir}"
chmod 0644 "${build_dir}/${artifact_name}"
mv -f "${build_dir}/${artifact_name}" "${artifact_dir}/${artifact_name}"
echo "Simulator Full Flash (${scenario}): ${artifact_dir}/${artifact_name}"
wc -c "${artifact_dir}/${artifact_name}"
shasum -a 256 "${artifact_dir}/${artifact_name}"

# Signal Atlas Donor Audit

Status: **Frozen pre-implementation research**

Date of research: **2026-10-05**

This document is the canonical pre-implementation donor decision made before Codex Autopilot execution. Codex is not responsible for re-running product research or choosing alternative donors unless an implementation-time incompatibility is discovered.

If a pinned source is unavailable or materially incompatible, Codex must report the deviation and use the narrowest safe substitute. It must not silently replace the architecture.

## Decision summary

Use the following hierarchy:

1. FoloToy AI Passport current baseline for board/BSP/radio lifecycle/build behavior.
2. ESP-IDF v5.5.3 for actual passive NimBLE scan mechanics.
3. reelyActive decoders for small deterministic iBeacon/Eddystone algorithm ports and test vectors.
4. Nordic Bluetooth Numbers Database as the Bluetooth Assigned Numbers build-time data source.
5. IEEE Registration Authority public MA-L/MA-M/MA-S listings as the MAC assignment data source, with release-time redistribution review noted below.
6. ESP32-BLECollector as reference-only for product/data-flow concepts; do not transplant its runtime architecture.

---

## 1. FoloToy/ai-passport

Repository:
`https://github.com/FoloToy/ai-passport`

Pinned baseline commit:
`33d3d1d93a1125b356b47b6d83a7a60121be801e`

Baseline commit date:
2026-10-04

License:
MIT

### Required reuse

#### `main/demo_radio.c`

Use directly as the lifecycle pattern for:

- NVS preparation
- `esp_netif` preparation
- default event loop ownership
- failure behavior
- the rule that wireless startup must not erase NVS to recover

Decision:
**adapt directly**

#### `main/demo_wifi.c`

Use as the primary Wi-Fi scanner lifecycle donor.

Reuse/adapt:

- explicit STA netif creation
- Wi-Fi initialization
- event registration for `WIFI_EVENT_SCAN_DONE`
- `WIFI_STORAGE_RAM`
- `WIFI_MODE_STA`
- `esp_wifi_scan_start()`
- checked startup rollback
- stop/deinit/handler unregister sequence

Do not reuse:

- its five-row demo presentation
- `ui_pixel` visual shell
- demo-local text formatting as the production data model

Decision:
**adapt directly**

#### `main/demo_ble.c`

Use as the primary AI Passport NimBLE lifecycle donor.

Reuse/adapt:

- NVS preparation
- `nimble_port_init()`
- owned host task
- explicit stop acknowledgement
- stop timeout handling
- `nimble_port_stop()`
- `nimble_port_deinit()`
- cleanup retry/ownership thinking
- reset/sync callback pattern

Do not reuse:

- advertiser/peripheral behavior
- device-name/GATT demo behavior
- demo UI

Decision:
**adapt lifecycle directly**

#### `components/bsp/**`

Use the existing BSP in place. Do not copy or fork board facts into Signal Atlas.

Key board source of truth:
`components/bsp/include/bsp_pins.h`

Confirmed baseline hardware facts include:

- ST7789P3 240×320
- SPI2 display
- 80 MHz display clock
- ADC button ladder on GPIO0
- 500 ms long-press baseline
- backlight on GPIO21

Decision:
**use in place**

#### `sdkconfig.defaults`

Current baseline has NimBLE peripheral/broadcaster enabled and central/observer disabled.

Signal Atlas production configuration therefore needs an intentional minimal observer/scanner configuration change.

Decision:
**modify deliberately for Signal Atlas; do not inherit the current role set unchanged**

#### `partitions.csv`

Current upstream layout:

```text
nvs      0x9000  0x6000
phy_init 0xf000  0x1000
factory  0x10000 0x7f0000
```

Signal Atlas may introduce a deliberate persistent history allocation, but the resulting 8 MB layout must be validated by the existing firmware gate.

---

## 2. Espressif ESP-IDF v5.5.3

Repository:
`https://github.com/espressif/esp-idf`

Pinned tag:
`v5.5.3`

Resolved release commit:
`2c211b236707889e8400c4dc5644dd5c4ee071e0`

Annotated tag object:
`b31fcc7a314a44ad992b58f589f7d1d8a4fadff6`

License:
Apache-2.0

Primary donor file:
`examples/bluetooth/nimble/blecent/main/main.c`

### Passive scan code to adapt

The v5.5.3 `blecent_scan()` donor already demonstrates:

- `ble_hs_id_infer_auto()`
- `struct ble_gap_disc_params`
- duplicate filtering
- `passive = 1`
- default interval/window handling
- `ble_gap_disc(...)`

Its `BLE_GAP_EVENT_DISC` path demonstrates:

- receiving advertisement reports
- `ble_hs_adv_parse_fields(...)`
- GAP callback structure

### What must be removed

Signal Atlas must not carry forward the central/client behavior from `blecent`:

- `ble_gap_connect()`
- peer management
- GATT discovery
- read/write/subscribe flows
- security/bonding flows

Decision:
**directly adapt only passive discovery + report parsing patterns; strip all connection/GATT behavior**

### Important implementation note

ESP-IDF `blecent` enables controller duplicate filtering in its example. Signal Atlas needs repeat observations for RSSI/seen-count updates, so Codex must evaluate the scanner parameters against the product model rather than blindly retaining `filter_duplicates = 1`.

Frozen design preference:
**allow repeated reports at the application level when required for live RSSI/history behavior, then perform bounded dedupe/update in `Nearby Store`.**

---

## 3. tobozo/ESP32-BLECollector

Repository:
`https://github.com/tobozo/ESP32-BLECollector`

Pinned commit:
`18929248287322d62b738d3ff0837313b71fd400`

License:
MIT

Relevant files:

- `ESP32-BLECollector/BLE.h`
- `ESP32-BLECollector/BLECache.h`
- `ESP32-BLECollector/DB.h`
- `SD/ble-oui.db`
- `SD/mac-oui-light.db`
- `SD/mac-oui.db`

Observed architectural assumptions:

- Arduino-style runtime
- large monolithic headers
- DB/SQLite-style persistence path
- SD/FAT assets
- legacy ESP32/WROVER-oriented use cases
- large UI/assets layer
- database files much larger than needed for this product

Decision:
**reference-only**

Use its product/data-flow ideas:

- scan → record → enrich → persist
- bounded device cache concepts
- first/last seen concepts
- offline identification UX

Do not directly transplant:

- BLE runtime implementation
- DB runtime
- SD storage
- UI
- build system
- architecture

Rationale:
The ESP-IDF + FoloToy donors are substantially closer to the target and reduce adaptation risk.

---

## 4. reelyactive/advlib-ble

Repository:
`https://github.com/reelyactive/advlib-ble`

Pinned commit:
`b23768dab5a3e59f941f2ab44a10a7c51c49abbb`

License:
MIT

Relevant core files:

- `lib/advdata.js`
- `lib/advdatatypes.js`
- `lib/assignednumbers.js`

Decision:
**reference-only for generic AD parsing semantics**

The production firmware should rely on NimBLE's standard advertisement parsing where appropriate and use a small bounded C parser only for fields/protocol framing not conveniently exposed by the stack.

---

## 5. reelyactive iBeacon donor

Repository:
`https://github.com/reelyactive/advlib-ble-manufacturers`

Pinned commit:
`952b448e12c2a60ea2cf9a4e22b4dd10e5540112`

License:
MIT

Relevant files:

- `lib/apple.js`
- `test/unit/apple.js`

The donor clearly defines the deterministic iBeacon byte layout used by its decoder:

- frame type `0x02`
- frame length byte `0x15`
- 16-byte UUID
- 2-byte major
- 2-byte minor
- signed TX power byte

It also supplies a compact valid test vector.

Decision:
**port the minimal iBeacon decoder logic to bounded C and port the donor test vector**

Do not port unrelated Apple proprietary frame classification from the same file for v0.1.

Preserve required MIT notice when substantial logic is adapted.

---

## 6. reelyactive Eddystone donor

Repository:
`https://github.com/reelyactive/advlib-ble-services`

Pinned commit:
`1351ff0a63aa8e170cb0c9807423739641d9de6f`

License:
MIT

Relevant files:

- `lib/eddystone.js`
- `test/unit/eddystone.test.js`

The donor provides deterministic byte layouts and test vectors for:

- Eddystone UID (`0x00`)
- Eddystone URL (`0x10`)
- Eddystone TLM (`0x20`)

It also contains EID/Find Hub behavior that is out of v0.1 scope.

Decision:
**port only UID / URL / unencrypted TLM logic and the relevant test vectors**

Do not add EID, Find Hub, or encrypted TLM feature surface in v0.1 unless needed solely to reject/label them safely.

Preserve required MIT notice when substantial logic is adapted.

---

## 7. Nordic Bluetooth Numbers Database

Repository:
`https://github.com/nordicsemi/bluetooth-numbers-database`

Pinned commit:
`d379cc06c9c3681bc1c1cd4d61c246a0b0f80b0e`

License:
BSD-3-Clause

Use these v1 inputs:

- `v1/company_ids.json` — 216,175 bytes at the pinned commit
- `v1/service_uuids.json` — 18,707 bytes
- `v1/gap_appearance.json` — 21,264 bytes

Do not include characteristic UUIDs in v0.1 unless later needed by a frozen UI field; passive advertising Service UUID names are sufficient for the current product.

Decision:
**build-time data source**

The firmware build must consume generated compact tables, not runtime JSON.

The generator must record:

- source repository
- pinned commit
- source file hashes if practical
- generated date/revision

Redistribution must retain the BSD-3-Clause notice in documentation/materials as required.

---

## 8. IEEE Registration Authority public listings

Authoritative source:
IEEE Registration Authority public listings.

Needed datasets:

- MA-L
- MA-M
- MA-S

The IEEE public site explicitly provides downloadable CSV public listings and describes:

- MA-L as 24-bit assignment
- MA-M as 28-bit assignment
- MA-S as 36-bit assignment

Therefore Signal Atlas must implement correct longest-prefix matching rather than treating every assignment as a 24-bit OUI.

Decision:
**authoritative build-time data source**

### Release/legal note

During this research, the public listing pages clearly exposed downloadable CSV data but did not present a simple MIT/BSD-style redistribution license for transformed embedded datasets.

Therefore:

- keep IEEE source attribution
- keep registry refresh tooling separate from normal firmware compilation
- document the source URL/revision date
- do not claim that redistribution terms have been legally cleared
- before a public/commercial firmware release that embeds a transformed IEEE table, perform a release/legal review of IEEE dataset redistribution terms

This is a release risk, not a reason for Codex to redesign the implementation.

### Gate B1 public-tree packaging boundary

For the intended distributable source tree, the IEEE CSV snapshots and transformed IEEE records are excluded until redistribution permission is verified. The parser/generator logic and MA-S → MA-M → MA-L runtime lookup remain intact. The default provider is empty; developers may independently obtain the official CSVs and create an ignored local-only provider using the procedure in `README.md`. Ordinary builds never download IEEE inputs. Nordic Bluetooth Assigned Numbers source and derived tables remain distributed under the pinned BSD-3-Clause terms. This is a data-packaging boundary only and does not revise the frozen lookup architecture.

---

## 9. AI Passport Simulator

Repository:
`https://github.com/VOID001/FoloToy-Passport-Simulator`

Pinned research commit:
`91b0914e5400df4d2bbb102774970c509ef39151`

The Simulator README explicitly states:

- ESP32-C3 firmware can run from a local Full Flash image
- local Full Flash must start at `0x0`
- image must be <= 8 MiB
- display and buttons are supported
- Wi-Fi/networking is supported
- BLE Controller is not simulated
- firmware entering BLE ROM code can pause with unsupported behavior

Decision:
**validation environment, not a BLE donor**

Signal Atlas Simulator profile must not initialize the real BLE Controller. It must use `MockBleScanner` feeding the same production pipeline.

---

## 10. Frozen implementation map

Codex should execute this mapping rather than perform a new architecture study.

| Signal Atlas subsystem | Frozen implementation source |
| --- | --- |
| Board/display/buttons | use FoloToy BSP in place |
| NVS/network prep | adapt `main/demo_radio.c` |
| Wi-Fi lifecycle/scan | adapt `main/demo_wifi.c` |
| NimBLE init/stop lifecycle | adapt `main/demo_ble.c` |
| Passive BLE GAP scan | adapt ESP-IDF v5.5.3 `blecent` scan/report path |
| BLE normalization | Signal Atlas glue code |
| Generic bounded AD normalization | NimBLE parser + small Signal Atlas glue |
| iBeacon | port minimal reelyActive `apple.js` logic + test vector |
| Eddystone UID/URL/TLM | port minimal reelyActive `eddystone.js` logic + test vectors |
| BT Company/Service/Appearance tables | generated from pinned Nordic DB |
| IEEE vendor table | local-only optional tables generated from independently obtained IEEE MA-L/MA-M/MA-S listings; public/default table is empty |
| Nearby store / identity / history | Signal Atlas product logic |
| UI | original Signal Atlas Instrument Green implementation |
| Simulator BLE | Signal Atlas MockBleScanner |
| Simulator environment | VOID001 simulator |

## Gate 1 implementation provenance

The frozen donor choices above were not changed. Gate 1 implemented the following bounded adaptations:

| Donor | Exact files / input | Local destination | Use and changes |
| --- | --- | --- | --- |
| reelyActive/advlib-ble-manufacturers `952b448e12c2a60ea2cf9a4e22b4dd10e5540112` (MIT) | `lib/apple.js`, `test/unit/apple.js` | `main/identify/protocol_decoder.c`, `tests/test_radio_model.c` | Adapted only the deterministic iBeacon frame layout and donor valid vector; omitted all other Apple frame classifications. Notice in `THIRD_PARTY_NOTICES.md`. |
| reelyActive/advlib-ble-services `1351ff0a63aa8e170cb0c9807423739641d9de6f` (MIT) | `lib/eddystone.js`, `test/unit/eddystone.test.js` | `main/identify/protocol_decoder.c`, `tests/test_radio_model.c` | Adapted only Eddystone UID/URL/unsecured TLM layouts and vectors, with bounded output; omitted EID, encrypted TLM, and Find Hub. Notice in `THIRD_PARTY_NOTICES.md`. |
| Nordic Bluetooth Numbers Database `d379cc06c9c3681bc1c1cd4d61c246a0b0f80b0e` (BSD-3-Clause) | `v1/company_ids.json`, `v1/service_uuids.json`, `v1/gap_appearance.json` | `tools/registry_sources/nordic/v1/`, generated `main/identify/registry_data.c` | Build-time compact table generation; no runtime JSON dependency. BSD notice in `THIRD_PARTY_NOTICES.md`. |
| IEEE Registration Authority public listings | MA-L `oui.csv`, MA-M `mam.csv`, MA-S `oui36.csv` | Excluded from repository; developer-selected external local inputs can generate ignored `main/identify/registry_data_ieee_local.c` | The local generator preserves longest-prefix behavior and excludes conflicting assignments. No IEEE source snapshot or transformed record is distributed by default. |

Nordic source SHA-256 values, row counts, build identifier, and generated logical Flash size are recorded in `main/identify/registry_metadata.json`. Any local IEEE metadata is ignored and remains on the developer's machine.

## Gate 3–4 implementation provenance

The frozen donor decisions above remain unchanged.

| Donor | Exact files / revision | Local destination | Reuse/adaptation and notice |
| --- | --- | --- | --- |
| FoloToy AI Passport (`https://github.com/FoloToy/ai-passport`, `33d3d1d93a1125b356b47b6d83a7a60121be801e`) | `main/demo_radio.c`, `main/demo_wifi.c` | Existing `main/demo_radio.c` is compiled directly; `main/radio/wifi_scanner.c`, `main/radio/radio.c`, and `main/model/observation.c` | Uses donor NVS/network preparation, STA lifecycle, and ESP-IDF Wi-Fi APIs. Bounded AP snapshots normalize into the shared observation event. No association, credentials, monitor mode, or client sniffing. No donor source was copied; repository license/notices remain applicable. |
| FoloToy AI Passport (`https://github.com/FoloToy/ai-passport`, `33d3d1d93a1125b356b47b6d83a7a60121be801e`) | `main/demo_ble.c` lifecycle | `main/radio/ble_scanner.c` | Adapts NimBLE init/deinit, owned host-task, sync/reset and stop acknowledgement pattern. Product scanner keeps GAP callback work to bounded copy/enqueue; parsing is deferred. No connection/GATT/security/bonding API is invoked. |
| ESP-IDF (`https://github.com/espressif/esp-idf`, tag `v5.5.3`, `2c211b236707889e8400c4dc5644dd5c4ee071e0`) | `examples/bluetooth/nimble/blecent/main/main.c`, passive discovery setup around lines 442–472 and GAP discovery reports around lines 724 onward | `main/radio/ble_scanner.c` | Adapts passive GAP discovery parameters and report handling only. Duplicate filtering is set to 0 to preserve repeated RSSI/sighting updates; connection and peer logic are excluded. ESP-IDF's Apache-2.0 notice is retained with the upstream checkout; adapted concepts/code remain under the repository's existing notice obligations. |

The production profile disables `CONFIG_BT_NIMBLE_SECURITY_ENABLE`: the product does not pair/connect, and ESP-IDF 5.5.3's observer-only + static-to-dynamic NimBLE configuration otherwise leaves an unresolved `ble_sm_deinit` reference at link time. This is a minimal build-configuration adjustment, not an enablement of BLE connection features. Gate 4 host lifecycle tests and the v5.5.3 production compile verify the selected path. Real BLE/RF behavior remains unverified pending physical-device acceptance.

---

## 11. What Codex is still allowed to decide

Codex may make local implementation decisions that do not alter the plan, including:

- exact C identifiers
- exact file splitting when equivalent
- small queue depths within budget
- task stack sizes after measurement
- exact generated-table packing
- exact sort formula
- exact scan timing constants within the centralized scheduler
- NVS record encoding details
- LVGL object composition consistent with the frozen UI

Codex must not independently reopen:

- donor selection
- product scope
- evidence semantics
- UI style
- identity semantics
- BLE connection policy
- cloud/network policy
- Simulator BLE strategy
- Gate definitions

unless execution uncovers a concrete blocker. In that case it must report the blocker and use the smallest deviation necessary.

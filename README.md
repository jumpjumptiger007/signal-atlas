# Signal Atlas

Signal Atlas is an offline, read-only field instrument for observing and interpreting nearby Wi-Fi and Bluetooth LE broadcasts on the FoloToy AI Passport.

It scans nearby 2.4 GHz Wi-Fi access points and BLE advertisements, then translates raw radio metadata into human-readable information using local registries and deterministic protocol decoders. No cloud service or network connection is required.

> Project status: pre-hardware development. UI, application logic, parsers, storage, registry generation, build profiles, and simulator coverage can be developed now. Real BLE/RF behavior remains unverified until a physical AI Passport is available.

## Source baseline

The implementation baseline is pinned to FoloToy AI Passport commit:

```text
33d3d1d93a1125b356b47b6d83a7a60121be801e
```

The execution workspace may initially contain the Signal Atlas planning documents plus benign OS metadata. Gate 0 is responsible for safely bootstrapping the pinned upstream source into the workspace before implementation when necessary. Benign OS metadata such as `.DS_Store` must not cause a routine approval pause.

## Target

- FoloToy AI Passport
- ESP32-C3
- 8 MB Flash
- no PSRAM
- 240 × 320 ST7789P3 display
- UP / DOWN / OK buttons
- Wi-Fi 2.4 GHz
- Bluetooth LE
- ESP-IDF 5.5.3 baseline

## What it does

### Nearby

A unified nearby list for:

- Wi-Fi access points / BSSIDs
- BLE advertisers

Modes:

- `ALL`
- `WI-FI`
- `BLE`

`ALL` uses bounded alternating scan windows rather than treating Wi-Fi and BLE as two permanently independent radios.

### Detail

Signal Atlas separates information by evidence level:

- **OBSERVED** — directly present in scan results or advertisement data
- **RESOLVED** — deterministic lookup from a public registry or protocol definition
- **POSSIBLE** — explicitly labeled heuristic inference

If no useful inference exists, the `POSSIBLE` section is omitted.

### Offline identification

Planned v0.1 identification includes:

- IEEE MA-S / MA-M / MA-L longest-prefix MAC lookup (optional local dataset; public/default builds ship zero IEEE entries)
- Bluetooth Company IDs
- Bluetooth Service UUIDs
- Bluetooth Appearance values
- iBeacon
- Eddystone UID
- Eddystone URL
- Eddystone TLM

Public source datasets are converted at build time into compact firmware-friendly lookup tables. The device does not parse the source JSON or CSV files at runtime.

### Registry data packaging

The default/public build includes the pinned Nordic Bluetooth Assigned Numbers data under BSD-3-Clause. IEEE MA-L/MA-M/MA-S lookup code and longest-prefix behavior are retained, but the repository and default/public firmware contain **zero IEEE-derived records** because redistribution clearance for the transformed listings has not been established. The official [IEEE Registration Authority listings](https://standards.ieee.org/products-programs/regauth/) provide the MA-L, MA-M, and MA-S downloads; obtain those files independently and do not commit them.

For a developer-local lookup build, place the official CSV files outside the repository and run:

```sh
python3 tools/generate_registries.py --mode ieee-local \\
  --ma-l /path/to/oui.csv --ma-m /path/to/mam.csv --ma-s /path/to/oui36.csv \\
  --ieee-date YYYY-MM-DD --output main/identify
idf.py -DSIGNAL_ATLAS_USE_LOCAL_IEEE_REGISTRY=ON build
```

The generated `registry_data_ieee_local.c` and local metadata are ignored by Git and are not suitable for redistribution. The ordinary public build does not download or consume IEEE data.

### Local history

History stores compact sighting summaries rather than packet logs.

A radio identity can be marked as:

- new to local history
- seen before
- currently observed

Wi-Fi identity is based on BSSID.

BLE identity is based on address plus address type. Signal Atlas does not attempt to correlate rotating BLE private addresses into a guessed physical-device identity.

## Offline and read-only by design

Signal Atlas does not:

- connect to discovered Wi-Fi networks
- connect to BLE devices or perform GATT interrogation
- upload scan data
- require a cloud service
- download registries at runtime
- use Wi-Fi monitor mode
- enumerate ordinary Wi-Fi client devices
- track people across randomized BLE addresses
- estimate physical distance from RSSI
- perform Evil Twin or attack detection
- assign security scores
- keep full packet history

## UI

The visual direction is **Instrument Green**: a compact field-instrument / oscilloscope-inspired interface designed specifically for the AI Passport display.

Primary views:

1. Nearby
2. Detail
3. History

Secondary actions use a lightweight modal system.

The Nearby header includes a small **Mini Radar** scan indicator. It is decorative only, runs only while scanning, and must not own radio state or trigger full-screen redraws.

Controls:

- `UP` — previous / scroll up
- `DOWN` — next / scroll down
- `OK` — open / select / confirm
- `Long OK` — back / close
- `Long OK` on Nearby — Quick Menu

The application uses its own Signal Atlas UI. The upstream AI Passport test/demo visual shell is not reused or recolored as the finished interface.

## Architecture

The intended flow is:

```text
WiFiIdfScanner ─┐
BleNimbleScanner├──> RadioObservation
MockBleScanner ─┘
                       |
                       v
                 Nearby Store
                       |
                       v
                Enrichment
          ┌────────────┼────────────┐
          │            │            │
       IEEE MAC   Bluetooth IDs   Protocol
                                  decoders
                       |
                       v
                  History Store
                       |
                       v
                       UI
```

Real and mock scanners feed the same normalized application model.

The BLE simulator path must use mock observations because the current AI Passport Simulator does not provide a usable BLE Controller. Mock BLE behavior is not hardware validation.

## Donor-first development

Signal Atlas uses a pre-researched, frozen donor map before introducing any new low-level implementation. Codex Autopilot executes that map rather than redoing donor research.

Frozen implementation source order:

1. pinned FoloToy AI Passport baseline for BSP, board facts, radio lifecycle, build, and validation
2. ESP-IDF v5.5.3 for passive NimBLE scan mechanics
3. reelyActive decoders for the frozen iBeacon and Eddystone UID/URL/TLM logic and test vectors
4. pinned Nordic Bluetooth Numbers Database inputs for Bluetooth Assigned Numbers
5. IEEE Registration Authority MA-L / MA-M / MA-S public listings for MAC assignment data
6. `tobozo/ESP32-BLECollector` as **reference-only** for product/data-flow concepts

The project intentionally does **not** inherit the Arduino/SQLite/SD/WROVER runtime architecture of ESP32-BLECollector.

The exact commits, files, licenses, and reuse boundaries are frozen in [`docs/donor-audit.md`](docs/donor-audit.md).

## Project execution documents

These files are the implementation handoff:

- [`AGENTS.md`](AGENTS.md) — mandatory Codex execution rules
- [`docs/donor-audit.md`](docs/donor-audit.md) — completed donor research and frozen reuse map
- [`docs/implementation-spec.md`](docs/implementation-spec.md) — frozen product/architecture/resource specification
- [`docs/gates.md`](docs/gates.md) — authoritative Gate 0–6 PASS criteria and evidence
- [`docs/hardware-acceptance.md`](docs/hardware-acceptance.md) — deferred real-device acceptance
- [`docs/codex-autopilot-goal.md`](docs/codex-autopilot-goal.md) — execution-only Autopilot entry point

Codex is expected to create/update `docs/execution-report.md` during implementation and create `THIRD_PARTY_NOTICES.md` when the final included donor code/data requires notices.

## Resource philosophy

The ESP32-C3 has no PSRAM, so predictable bounded memory is a product requirement.

The implementation should prefer:

- fixed-capacity live stores
- bounded advertisement snapshots
- small queues
- flash-resident generated registry tables
- host-testable pure logic
- explicit task ownership
- measured heap and largest-block evidence

Avoid:

- full-screen RGB565 framebuffers
- runtime JSON databases
- SQLite
- unbounded packet storage
- unnecessary dynamic strings
- audio initialization for this application

## Simulator and hardware verification

The Simulator is used for:

- boot flow
- display layout
- three-button navigation
- Instrument Green UI
- Mini Radar behavior
- mock BLE observations
- host/application state
- history and error states
- simulator-supported Wi-Fi paths
- Full Flash image validation

A physical AI Passport is required for authoritative validation of:

- BLE scanning
- RF performance
- Wi-Fi/BLE coexistence behavior
- real scan density and range
- radio lifecycle memory behavior
- long-running RF stability

Build success and simulator success must never be reported as BLE hardware validation.

## Validation

Follow the upstream validation entry point:

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware
./tools/validate.sh
```

Delivery reports must keep these separate:

```text
Build: PASS / FAIL / NOT RUN
Host tests: PASS / FAIL / NOT RUN
Simulator tests: PASS / FAIL / NOT RUN
Device tests: PASS / FAIL / NOT RUN
Unverified: remaining hardware-only checks
```

## License and provenance

Directly reused or substantially adapted donor code must retain the license notices required by its source project.

A donor/provenance record must identify:

- repository
- exact commit SHA
- license
- relevant source files
- whether the code was reused, adapted, or used only as a reference

The final firmware remains subject to the licenses of the code and data actually included in the build.

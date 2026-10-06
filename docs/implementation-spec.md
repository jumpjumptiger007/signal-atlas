# Signal Atlas v0.1 Implementation Specification

Status: **Frozen for execution**

This document defines the v0.1 implementation contract for Signal Atlas on the FoloToy AI Passport. It complements the repository-root `README.md` and `AGENTS.md`.

The implementation must follow `AGENTS.md` first. When this document conflicts with current board facts, measured results, BSP code, or upstream mandatory engineering rules, the source-of-truth order in `AGENTS.md` applies.

---

## 1. Product boundary

Signal Atlas v0.1 is a completely offline, read-only field instrument for observing and interpreting nearby Wi-Fi and Bluetooth LE broadcasts.

It supports:

- 2.4 GHz Wi-Fi AP/BSSID discovery using normal ESP-IDF STA scans
- passive Bluetooth LE advertisement discovery using NimBLE
- local interpretation of radio metadata
- bounded local history
- offline public-registry lookups
- deterministic iBeacon and Eddystone decoding
- a 240 × 320 three-button UI
- host-testable domain logic
- a Simulator profile with injected BLE mock observations
- a production profile with real Wi-Fi and BLE scanners

It explicitly excludes:

- Wi-Fi association
- BLE connection/GATT interrogation
- monitor mode
- Wi-Fi client sniffing
- packet injection
- attack detection
- Evil Twin detection
- security scoring
- RF Tamagotchi or game mechanics
- RSSI-to-distance estimation
- cloud APIs
- runtime registry downloads
- persistent packet logging
- cross-address tracking of randomized BLE devices
- proprietary product-model fingerprinting in v0.1

---

## 2. Donor-first implementation strategy

The exact reuse plan has already been determined in `docs/donor-audit.md`. Gate 0 verifies that the execution workspace matches that frozen plan before low-level scanner implementation begins.

### 2.1 Preferred donors

#### FoloToy/ai-passport

Use as the primary source for:

- BSP ownership and hardware facts
- display initialization
- LVGL locking rules
- button behavior
- Wi-Fi initialization / shutdown lifecycle
- NimBLE initialization / shutdown lifecycle
- task ownership and failure rollback patterns
- build, validation, merged-image, and simulator workflow

Do not reuse the upstream demo visual shell as the product UI.

#### ESP-IDF 5.5.x

Use as the primary radio API donor for:

- passive NimBLE discovery
- GAP event handling
- BLE advertisement field extraction patterns
- ESP32-C3 compatible Wi-Fi scan APIs
- coexistence-supported API behavior

Only reuse code compatible with the repository's pinned ESP-IDF version.

#### tobozo/ESP32-BLECollector

Use as reference-only for:

- scan-to-identification flow
- bounded device records
- vendor/company lookup concepts
- passive BLE observation concepts
- first-seen / last-seen logic

Reject donor architecture that depends on:

- Arduino runtime
- SQLite
- SD/FAT
- PSRAM/WROVER
- donor-specific UI frameworks
- network database update paths

#### reelyActive BLE decoder projects

Use only the frozen minimal iBeacon and Eddystone UID/URL/TLM decoder logic and associated test vectors identified in `docs/donor-audit.md`.

Do not bring JavaScript runtime assumptions into firmware.

#### Registry sources

Use build-time data from:

- IEEE public MAC registry datasets
- the pinned Nordic Bluetooth Numbers Database inputs defined in `docs/donor-audit.md`

Record license and revision metadata in generated outputs.

### 2.2 Provenance artifact

Use the pre-authored `docs/donor-audit.md` as the donor-selection source of truth. Codex may append implementation provenance but does not redo donor research.

Each donor entry must record:

```text
Repository:
Commit:
License:
Relevant files:
Use:
  - reused
  - adapted
  - reference-only
Reason:
Notes:
```

Direct reuse or substantial adaptation must preserve required notices.

---

## 3. Application architecture

Preferred application structure:

```text
main/
├── app.c
├── app_state.c
│
├── radio/
│   ├── radio.h
│   ├── radio_scheduler.c
│   ├── wifi_scanner.c
│   ├── ble_scanner.c
│   └── mock_scanner.c
│
├── model/
│   ├── observation.h
│   ├── observation.c
│   ├── nearby_store.c
│   └── identity.c
│
├── identify/
│   ├── registry.c
│   ├── ble_ad_parser.c
│   └── protocol_decoder.c
│
├── storage/
│   └── history.c
│
└── ui/
    ├── ui_theme.c
    ├── ui_nearby.c
    ├── ui_detail.c
    ├── ui_history.c
    └── ui_modal.c

tools/
└── generate_registries.py

generated/
└── compact registry tables
```

Equivalent structure is acceptable if it remains simple, bounded, and clearly separates:

- radio backends
- normalized observations
- domain state
- enrichment
- persistence
- UI

Avoid unnecessary service locators, DI frameworks, repositories, factories, or generalized plugin systems.

---

## 4. Core data model

### 4.1 RadioObservation

The production and mock scanners must emit the same normalized observation type.

Conceptual fields:

```c
typedef enum {
    RADIO_KIND_WIFI = 1,
    RADIO_KIND_BLE  = 2,
} radio_kind_t;

typedef struct {
    radio_kind_t kind;

    radio_identity_t identity;

    int8_t rssi;
    uint32_t seen_seq;

    union {
        wifi_observation_t wifi;
        ble_observation_t ble;
    } data;
} radio_observation_t;
```

Exact naming may change, but these properties must remain:

- fixed-size or tightly bounded
- no ownership ambiguity
- no heap-allocated arbitrary strings
- no unbounded packet payloads
- host-testable normalization

### 4.2 Wi-Fi observation

Required bounded fields:

```text
BSSID
SSID up to ESP-IDF maximum
RSSI
primary channel
auth/security mode
address classification flags
```

Optional fields are acceptable only when they are cheap and have clear user value.

### 4.3 BLE observation

Required bounded fields:

```text
address
address type
RSSI
advertising/event properties
local name (bounded)
TX power when present
Company ID when present
Appearance when present
bounded list of Service UUIDs
bounded manufacturer-data preview
bounded service-data preview
```

Suggested initial bounds:

```text
local name: <= 31 bytes
service UUID records: <= 6
manufacturer preview: <= 24 bytes
service-data preview: <= 24 bytes
```

Bounds may be tuned after measurement but must remain explicit.

---

## 5. Identity semantics

### Wi-Fi

Identity key:

```text
BSSID
```

No SSID-level merging in v0.1.

### BLE

Identity key:

```text
address + address_type
```

Do not merge randomized addresses.

### Address privacy

Before IEEE registry lookup:

- detect locally administered/private MAC addresses
- do not perform vendor lookup when registry semantics are invalid

BLE address type should be displayed only at the precision the stack can actually establish.

---

## 6. Evidence model

Each Detail field belongs to one level.

### OBSERVED

Direct packet/scan evidence:

- SSID
- BSSID
- BLE local name
- address
- RSSI
- channel
- security/auth mode
- TX power
- raw Company ID value
- raw Service UUIDs
- raw AD payload previews

### RESOLVED

Deterministic interpretation:

- IEEE registry organization
- Bluetooth Company ID name
- Bluetooth standard Service name
- Bluetooth Appearance name
- iBeacon
- Eddystone UID
- Eddystone URL
- Eddystone TLM

### POSSIBLE

Heuristic interpretation only.

v0.1 should minimize this section.

If no useful inference exists, do not render it.

No heuristic may overwrite or be visually confused with observed/resolved data.

---

## 7. BLE advertising parser

Implement the parser as pure, host-testable logic.

Requirements:

- reject malformed AD structures safely
- never read outside input bounds
- support duplicate fields predictably
- preserve bounded raw preview data
- extract standard AD types needed for v0.1
- avoid dynamic allocation
- tolerate unknown AD types

Required deterministic decoders:

### iBeacon

Recognize valid manufacturer-specific iBeacon framing and extract only fields needed for a concise explanation.

### Eddystone

Support:

- UID
- URL
- TLM

Unknown/unsupported Eddystone frame types should remain raw/resolved only as far as safely known.

---

## 8. Registry representation

### 8.1 Build-time generator

`tools/generate_registries.py` should:

1. consume upstream registry source files
2. normalize names deterministically
3. generate compact C/binary lookup data
4. emit metadata containing source revision/date
5. be reproducible
6. avoid network access during normal firmware compilation unless an explicit refresh step is requested

Registry refresh and firmware build should be separate operations.

### 8.2 IEEE

Implement longest-prefix matching for available:

- MA-S
- MA-M
- MA-L

The implementation must not assume every allocation is a 24-bit OUI.

Preferred data representation:

```text
sorted prefix records
prefix length
organization string offset
deduplicated string pool
```

Public source datasets are converted at build time into compact firmware-friendly lookup tables. The device does not parse the source JSON or CSV files at runtime. The distributable/default build includes the pinned licensed Nordic Assigned Numbers tables. IEEE MA-L/MA-M/MA-S lookup code remains available, but the public source tree and default/public firmware include no IEEE-derived records while redistribution clearance is unresolved. A developer may independently obtain official IEEE CSV inputs and generate an ignored, local-only provider; see `README.md`. This packaging boundary does not alter the approved identity model, lookup algorithm, or product behavior when a local dataset is explicitly enabled.

### 8.3 Bluetooth

Use compact sorted lookup tables for:

- Company IDs
- Service UUIDs
- Appearance values

Runtime lookup should not require loading the registry into heap.

### 8.4 About metadata

The About modal should expose a concise database build identifier, for example:

```text
DB 2026-10-05
```

or a compact generated revision.

---

## 9. Nearby live store

Use a fixed-capacity or equivalently bounded store.

Initial target:

```text
total capacity: ~96
BLE: approximately 64
Wi-Fi: approximately 32
```

A shared pool is preferred when simpler.

### 9.1 Update behavior

Existing identity:

- update RSSI
- update bounded metadata snapshot
- increment observation count
- update last-seen sequence/timebase
- preserve stable user selection

New identity:

- insert if capacity exists
- otherwise apply bounded eviction policy

### 9.2 Eviction order

Prefer:

1. expired
2. stale
3. oldest / weakest suitable record

Do not evict the selected entry unless no other safe candidate exists.

### 9.3 Sorting

Normal Nearby ranking may consider:

- freshness
- RSSI

Do not require exact ranking formula in v0.1.

### 9.4 Selection freeze

When the user presses UP or DOWN in Nearby:

- freeze row order
- continue updating record values
- do not move selected rows due to RSSI changes

Resume reorder after:

- a configurable inactivity timeout, or
- leaving the Nearby list

Keep this logic independent from LVGL where practical.

---

## 10. Radio scheduler

Product states:

```text
ALL
WI-FI
BLE
PAUSED
```

### WI-FI

- Wi-Fi scanner active
- BLE scan stopped

### BLE

- BLE scanner active
- Wi-Fi scan stopped

### ALL

Preferred v0.1 scheduling:

```text
BLE scan window
→ Wi-Fi scan
→ BLE scan window
→ Wi-Fi scan
→ ...
```

The exact timing must be centralized in configuration constants.

The UI must not own scheduler timing.

### PAUSED

Pause scan work while preserving Nearby results.

Backend resource teardown during pause is an implementation detail decided from memory/power evidence.

---

## 11. Scanner interface

The exact API is implementation-specific, but the app should depend on a small backend contract conceptually similar to:

```c
typedef enum {
    RADIO_BACKEND_STOPPED,
    RADIO_BACKEND_STARTING,
    RADIO_BACKEND_SCANNING,
    RADIO_BACKEND_IDLE,
    RADIO_BACKEND_FAILED,
    RADIO_BACKEND_UNAVAILABLE,
} radio_backend_state_t;
```

Events:

```text
BACKEND_READY
SCAN_STARTED
OBSERVATION
SCAN_COMPLETE
BACKEND_ERROR
```

The app must not depend directly on radio-library callbacks.

BLE/Wi-Fi callbacks should only do bounded work and forward events.

---

## 12. UI specification

Visual direction:

**Instrument Green + Mini Radar**

### 12.1 Primary screens

Only:

- Nearby
- Detail
- History

History Detail reuses Detail.

Secondary actions are modal overlays.

### 12.2 Nearby

Header:

```text
SIGNAL ATLAS          [radar]
ALL · SCANNING            27
```

List row:

```text
B  AirPods Pro          -48
W  FRITZ!Box            -55
B  Apple            NEW -61
```

Do not permanently display `SEEN` on every row.

### 12.3 Detail

Single vertically scrollable information document.

Preferred section order:

```text
OBSERVED
RESOLVED
POSSIBLE
SEEN
RAW
```

Hide sections/fields that have no content.

Do not create tabs.

### 12.4 History

Show saved identities, sorted by useful recent-local ordering.

No fabricated dates.

History Detail uses the same Detail screen.

### 12.5 Modal system

Use one lightweight reusable modal for:

- Quick Menu
- Filter
- Clear History confirmation
- About

### 12.6 Controls

```text
UP       previous / scroll up
DOWN     next / scroll down
OK       open / choose / confirm
Long OK  back / close
```

Nearby Long OK opens Quick Menu.

No double-click.

### 12.7 Mini Radar

Target:

```text
~24 × 24 px
~6–8 FPS
```

Implementation:

- small circle
- simple crosshair
- rotating line
- <=2 blips
- local invalidation only

Active only when:

```text
screen == Nearby
scan_state == SCANNING
```

Stop/freeze otherwise.

No framebuffer, no large alpha layer.

Fallback:

```text
Scan Pulse
```

only if measured performance requires it.

---

## 13. History persistence

History is not a raw log.

Target capacity:

```text
~512 identities
```

Persist compact records such as:

```text
schema_version
identity
radio kind
first session
last session
total observation count
last RSSI
compact classification flags/IDs
```

Do not store duplicate human-readable vendor strings when they can be resolved again from Flash tables.

Do not require wall-clock time.

### 13.1 Session model

Maintain a persistent incrementing session ID where useful.

A session begins when the application boots.

This enables correct semantics without RTC.

### 13.2 Storage backend

Prefer an ESP-IDF-native persistent backend.

Initial engineering expectation:

- dedicated namespace or partition
- approximately 128 KiB total reserved area unless measured overhead justifies another value

Do not add SQLite or filesystem complexity for v0.1.

### 13.3 Clear History

Clearing persistent history:

- must not erase unrelated NVS
- must not clear current Nearby observations
- current observations may subsequently re-register as new history entries

Use a confirmation modal with `Cancel` selected by default.

---

## 14. Memory budget

Board baseline:

- ESP32-C3
- no PSRAM
- existing LVGL small-buffer architecture

Signal Atlas-specific engineering targets:

| Area | Target |
| --- | ---: |
| Nearby live store | <= 16 KB |
| Observation queue | <= 6 KB |
| Parser/enrichment scratch | <= 6 KB |
| History working state | <= 8 KB |
| Registry runtime heap | approximately 0 |
| Own-added steady-state RAM | target <= ~48 KB |

Desired measured headroom where practical:

```text
steady-state free heap >= ~48 KB
largest free block >= ~24 KB
```

These are guardrails, not claims.

Always measure:

- free heap
- minimum free heap
- largest free block
- task stack high-water marks

Do not add a full-screen RGB565 framebuffer.

Do not initialize audio/I2S for Signal Atlas unless unavoidable upstream startup behavior requires it.

---

## 15. Flash budget

The complete firmware must fit within a legal 8 MB Flash layout.

Initial design targets:

| Area | Target |
| --- | ---: |
| History persistent area | ~128 KiB |
| IEEE registry | <= ~1.0 MB |
| Bluetooth registries / decoder tables | <= ~0.3 MB |
| Combined identification data | around or below ~1.3 MB |

If a target is missed:

1. optimize representation
2. deduplicate strings
3. remove redundant generated metadata
4. only then reconsider coverage

Do not trade correct registry semantics for cosmetic Flash savings without evidence.

---

## 16. Build profiles

### Production profile

Uses:

- real ESP-IDF Wi-Fi scanner
- real NimBLE BLE observer scanner
- production registry data
- production persistence
- production UI

### Simulator profile

Uses:

- simulator-supported Wi-Fi path
- mock BLE scanner
- the same production domain, parser, registry, persistence, and UI layers

Do not implement a fake BLE Controller.

Compile-time profile separation is acceptable if kept small and explicit.

---

## 17. Simulator test matrix

The Simulator should validate:

| Area | Required |
| --- | --- |
| Boot | yes |
| Instrument Green UI | yes |
| Three-button navigation | yes |
| Nearby list | yes |
| Selection freeze | yes |
| Detail scrolling | yes |
| History | yes |
| Modal system | yes |
| Mini Radar active/paused states | yes |
| Empty state | yes |
| Error state | yes |
| Overflow behavior | yes |
| Mock BLE ingestion | yes |
| Registry lookup | yes |
| History persistence where simulator supports it | yes |
| Simulator-supported Wi-Fi path | yes |
| Full Flash boot | yes |

The Simulator must not be used as proof of:

- real BLE scanning
- BLE controller correctness
- RF range
- antenna performance
- coexistence quality
- real radio memory stability

---

## 18. Host test matrix

At minimum:

### Identity / addressing

- global MAC
- locally administered MAC
- BLE public address
- BLE random address
- identity equality
- identity inequality

### Wi-Fi

- hidden SSID
- normal SSID
- auth mode mapping
- local-admin vendor suppression

### BLE parser

- local name
- missing local name
- TX power
- known Company ID
- unknown Company ID
- Service UUID
- Appearance
- malformed length
- truncated field
- duplicate field
- oversized field
- iBeacon
- Eddystone UID
- Eddystone URL
- Eddystone TLM

### Nearby store

- insert
- update
- dedupe
- RSSI change
- full capacity
- eviction
- selected-entry protection
- stale
- expire
- selection freeze
- reorder after timeout

### History

- new identity
- seen identity
- session increment
- serialization
- restore
- clear
- schema mismatch
- corrupt record handling
- capacity/LRU policy

### Scheduler

- ALL transitions
- WI-FI only
- BLE only
- pause
- resume
- backend error
- backend retry

---

## 19. Hardware Acceptance

The authoritative physical-device verification contract is:

`docs/hardware-acceptance.md`

Do not maintain a duplicate checklist in this specification.

Real BLE/RF, coexistence quality, long-running radio stability, and authoritative hardware memory behavior remain unverified until that document is executed on a physical AI Passport.

---

## 20. Execution Gates

The authoritative Gate 0–6 execution contract is maintained separately in:

`docs/gates.md`

Use that document for:

- Gate entry requirements
- required implementation
- PASS criteria
- evidence requirements
- allowed hardware-unverified status

The separate physical-device verification contract is:

`docs/hardware-acceptance.md`


## 21. Degradation policy

If measured RAM or rendering performance is insufficient, degrade in this order:

1. reduce Nearby live capacity
2. reduce raw BLE preview bytes
3. right-size task stacks and queues using measurements
4. use more aggressive full radio-lifecycle alternation in ALL
5. replace Mini Radar with approved Scan Pulse

Do not first remove correct registry support.

Do not replace deterministic evidence with vague guesses.

---

## 22. Final reporting contract

Every final Autopilot handoff must report:

```text
Build: PASS / FAIL / NOT RUN
Host tests: PASS / FAIL / NOT RUN
Simulator tests: PASS / FAIL / NOT RUN
Device tests: PASS / FAIL / NOT RUN
Unverified: ...
```

Also include:

- implementation summary
- changed-file summary
- implementation provenance and donor-map deviations
- `docs/execution-report.md`
- `THIRD_PARTY_NOTICES.md` when applicable
- Gate 0–6 status
- firmware size and partition usage
- Full Flash artifact paths
- available memory measurements
- exact hardware-only acceptance items
- final git status

Do not tag, publish, release, flash, commit, or push unless the user's active workflow explicitly authorizes that action.

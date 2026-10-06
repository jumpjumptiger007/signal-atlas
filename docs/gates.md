# Signal Atlas v0.1 — Execution Gates

Status: **Frozen execution contract**

This file is the authoritative definition of Gate 0–6 for Codex Autopilot.

`AGENTS.md`, `docs/implementation-spec.md`, and `docs/codex-autopilot-goal.md` must refer to this file rather than independently redefining gate pass criteria.

General rule:

> A successful compile is not sufficient to mark a Gate PASS.

A Gate passes only when its required implementation, tests, and evidence are present.

Codex must create and continuously update:

`docs/execution-report.md`

This is the canonical evidence log for Gate 0–6. Do not scatter authoritative Gate results across ad-hoc notes.

If included donor code/data requires third-party notices, Codex must also create/update:

`THIRD_PARTY_NOTICES.md`

If a hardware-only requirement cannot be executed, it must be explicitly marked `UNVERIFIED — REAL DEVICE REQUIRED`; it must not be silently converted into PASS.

## Environment and progression policy

The non-destructive development/validation environment required by Gate 0–6 is pre-authorized by `AGENTS.md`.

- Missing ESP-IDF v5.5.3 means prepare/activate the pinned workspace-local or user-local SDK; it is not a routine approval blocker.
- A missing pinned Passport Simulator checkout means fetch/check out the frozen revision; it is not a routine approval blocker.
- Ordinary open-source build/test dependency downloads required by the frozen validation plan are allowed.
- Do not use sudo, destructive system-wide package replacement, global shell-profile mutation, or replace unrelated installed SDK versions.
- Record environment setup and exact versions used in `docs/execution-report.md`.

Gates are verification checkpoints, not C2C approval pauses. A Gate may only be marked PASS when its own evidence contract is satisfied, but C2C confirmation is not a prerequisite for moving to the next Gate. If a missing environment can be safely prepared, prepare it and rerun validation automatically. If a genuinely unresolved external condition blocks a Gate but later implementation work is independent of it, Autopilot may continue that independent work while the blocked Gate remains explicitly BLOCKED/UNVERIFIED. Gate 6 and dependent final claims cannot be marked PASS until predecessor evidence is satisfied.

---

## Gate 0 — Baseline Verification and Plan Intake

### Purpose

Bootstrap the pinned FoloToy source tree when necessary, then verify that the execution workspace matches the already-completed planning and donor research.

Gate 0 is **not** a research or redesign phase.

### Required inputs

Read:

- `AGENTS.md`
- `README.md`
- `docs/donor-audit.md`
- `docs/implementation-spec.md`
- `docs/gates.md`
- `docs/development/ai-guide.md`
- additional upstream task-routed documents required by `AGENTS.md`

### Required checks

First determine workspace state.

If already a Git repository:

- record current branch
- record current HEAD
- record `git status --short --branch`
- preserve unrelated user changes

If not a Git repository:

- verify the seven canonical Signal Atlas planning files are present
- classify additional root entries using the Workspace bootstrap policy in `AGENTS.md`
- ignore/preserve benign OS metadata such as `.DS_Store`, `Thumbs.db`, `desktop.ini`, and `._*` without asking for approval
- safely preserve the planning files and any unrelated user content
- bootstrap `FoloToy/ai-passport`
- pin baseline commit `33d3d1d93a1125b356b47b6d83a7a60121be801e`
- create/switch to `feature/radio-explorer`
- restore the canonical planning files and benign metadata
- do not commit
- record resulting branch, HEAD, and `git status --short --branch`

Then:

- verify required Passport skills are available
- verify the supplied FoloToy baseline is compatible with the workspace
- confirm the pinned donor revisions supplied in `docs/donor-audit.md` are usable for the frozen implementation; do not search for replacements unless a concrete incompatibility blocks execution
- inspect the exact BSP/config files needed for implementation
- report any concrete mismatch between the supplied plan and current workspace

### PASS criteria

Gate 0 is PASS when:

- the supplied plan has been read
- a valid Git worktree exists at the workspace root
- HEAD is the frozen baseline or an explicitly documented equivalent execution state based on it
- the active branch is the original `feature/radio-explorer` implementation branch
- workspace state is recorded
- all canonical planning files are present in the worktree
- no unrelated meaningful user content was overwritten
- benign metadata did not cause a routine approval pause
- no unresolved baseline mismatch blocks implementation
- the frozen donor map is usable, or any necessary minimal deviation is explicitly documented

### Evidence

Record:

```text
Bootstrap required: YES/NO
Bootstrap result: PASS/FAIL/NOT NEEDED
Branch:
HEAD:
Expected baseline: 33d3d1d93a1125b356b47b6d83a7a60121be801e
Working tree:
Canonical planning files present: PASS/FAIL
Plan intake: PASS/FAIL
Donor map compatibility: PASS/FAIL
Required skills: PASS/FAIL
Deviations:
```

---

## Gate 1 — Domain Core and Registry Pipeline

### Purpose

Implement the pure/testable application core before hardware backends.

### Required implementation

- normalized `RadioObservation`
- Wi-Fi identity = BSSID
- BLE identity = address + address type
- locally administered/private address detection
- bounded Nearby store
- dedupe/update
- observation counters
- stale/expiry behavior
- bounded eviction
- selected-entry protection
- selection freeze/reorder behavior
- BLE AD parser
- iBeacon decoder
- Eddystone UID decoder
- Eddystone URL decoder
- Eddystone TLM decoder
- IEEE longest-prefix lookup
- Bluetooth Company ID lookup
- Bluetooth Service UUID lookup
- Bluetooth Appearance lookup
- deterministic registry generator

### Donor requirements

Follow `docs/donor-audit.md`.

In particular:

- reelyActive logic/test vectors only for the frozen iBeacon/Eddystone subset
- Nordic pinned database inputs for Bluetooth assigned numbers
- IEEE MA-L / MA-M / MA-S for longest-prefix data

### Required tests

Host tests must cover at least:

- global vs locally administered MAC
- BLE public vs random identity
- identity equality/inequality
- hidden SSID
- malformed/truncated BLE AD
- known/unknown Company ID
- Service UUID
- Appearance
- iBeacon valid/invalid
- Eddystone UID/URL/TLM valid/invalid
- duplicate updates
- RSSI updates
- capacity overflow
- selected-entry protection
- stale/expire
- selection freeze and resume
- IEEE MA-L / MA-M / MA-S longest-prefix behavior

### PASS criteria

- all required pure modules implemented
- no unbounded data structure introduced
- all applicable Gate 1 host tests pass
- generated registry artifacts are deterministic
- generated registry sizes are measured
- registry runtime design does not require loading the full database into heap

### Evidence

Record:

```text
Host tests:
Registry source revisions:
Generated registry sizes:
Nearby record size:
Capacity:
Known failures:
```

---

## Gate 2 — Mock Backend and Final UI

Before declaring Simulator validation unavailable, automatically prepare/activate the pinned ESP-IDF v5.5.3 environment and pinned Simulator checkout under the standing authorization in `AGENTS.md`. Do not wait for user or C2C approval for that non-destructive environment preparation. Do not weaken any existing Gate 2 Simulator runtime requirements.

### Purpose

Complete the product UI and production application flow without relying on BLE hardware.

### Required implementation

- original Signal Atlas UI
- Instrument Green theme
- Nearby screen
- shared Detail screen
- History screen
- shared modal system
- Quick Menu
- Filter modal
- Clear History confirmation
- About modal
- Mini Radar
- three-button navigation
- empty state
- backend failure state
- overflow behavior
- mock scanner backend
- mock BLE observations entering the same production model pipeline

### Mini Radar requirements

- approximately 24 × 24 px
- active only on Nearby while `SCANNING`
- approximately 6–8 FPS target
- local redraw/invalidation only
- no framebuffer
- no large alpha layer
- paused outside scanning state

### Required validation

Host tests:

- UI-independent state transitions
- modal transitions
- selection behavior
- radar active/paused state logic

Simulator:

- boot
- three-button navigation
- Nearby → Detail → back
- Nearby → Quick Menu
- Filter
- History
- History Detail reuse
- Clear History confirmation
- Mini Radar active/paused
- empty state
- mock BLE records
- list overflow
- selection freeze

### PASS criteria

- UI is not an upstream demo-shell recolor
- all three primary screens work
- modal system works
- mock BLE traverses the production pipeline
- no real BLE Controller is initialized in Simulator profile
- applicable host + Simulator checks pass

### Evidence

Record:

```text
Host tests:
Simulator boot:
Navigation:
Mini Radar:
Mock BLE pipeline:
Screenshots/logs if available:
```

---

## Gate 3 — Wi-Fi Backend

### Purpose

Integrate real ESP-IDF Wi-Fi scanning using the frozen FoloToy donor lifecycle.

### Required donor

Primary:

- FoloToy `main/demo_radio.c`
- FoloToy `main/demo_wifi.c`

### Required implementation

- NVS/network preparation
- STA netif lifecycle
- event registration
- STA-mode scanning
- scan completion
- bounded record extraction
- normalized Wi-Fi observations
- hidden SSID
- BSSID
- RSSI
- channel
- auth/security
- local/private address classification
- cleanup
- retry/failure path

### Prohibited

- Wi-Fi association
- credentials
- monitor mode
- client sniffing

### Required validation

- host tests for normalization/mapping
- production build
- Simulator-supported Wi-Fi path
- repeated start/stop where Simulator/build environment supports it
- cleanup/error-path tests where practical

### PASS criteria

- backend follows donor lifecycle rather than reinventing it
- observations enter the common production model
- no network connection is attempted
- build passes
- Simulator-supported Wi-Fi behavior passes

### Evidence

Record:

```text
Donor files used:
Wi-Fi build:
Simulator Wi-Fi:
Lifecycle tests:
Known hardware-only items:
```

---

## Gate 4 — NimBLE Passive Observer Backend

### Purpose

Integrate production passive BLE advertisement scanning.

### Required donors

Lifecycle:

- FoloToy `main/demo_ble.c`

Passive scan:

- ESP-IDF v5.5.3 `examples/bluetooth/nimble/blecent/main/main.c`

### Required implementation

Adapt only:

- NimBLE init/deinit lifecycle
- owned host task/stop acknowledgement
- reset/sync flow
- passive discovery parameters
- GAP advertisement report path
- bounded report normalization

Remove/avoid:

- connection
- GATT client
- peer management
- service discovery
- read/write/subscribe
- security/bonding

### Important scan behavior

Do not blindly retain the donor's controller duplicate filtering if it prevents Signal Atlas from receiving the repeated observations needed for:

- live RSSI updates
- seen counts
- stale/freshness behavior

Application-level dedupe/update belongs in the Nearby store.

### Required validation

- production build
- observer configuration compile validation
- host parser/normalization tests
- lifecycle state tests where hardware-independent

### Simulator rule

Do not initialize real NimBLE BLE Controller in the Simulator profile.

### PASS criteria

Gate 4 can PASS without a physical device if:

- the real production backend compiles
- all hardware-independent BLE logic/tests pass
- the Simulator continues to use MockBleScanner
- BLE RF status remains explicitly unverified

Required status:

```text
BLE RF/HARDWARE: UNVERIFIED — REAL DEVICE REQUIRED
```

### Evidence

Record:

```text
Donor lifecycle:
Donor passive-scan path:
Production build:
Observer config:
Host BLE tests:
Hardware verification: UNVERIFIED
```

---

## Gate 5 — Scheduler, Persistence, and Stress

### Purpose

Finish product lifecycle behavior and exercise bounded-resource stability.

### Required implementation

- `ALL`
- `WI-FI`
- `BLE`
- `PAUSED`
- centralized scan timing constants
- alternating ALL scheduler
- compact persistent history
- persistent session identifier
- new/seen behavior
- history capacity policy
- schema/version handling
- corrupt/incompatible history handling
- clear-history behavior
- heap instrumentation
- largest-free-block instrumentation
- relevant task high-water instrumentation

### Required automated stress

Run at least:

```text
50 scan/update cycles
```

in available host/Simulator environments.

Exercise:

- repeated observation update
- mode changes
- pause/resume
- history writes/updates
- list capacity
- scanner lifecycle paths that can be exercised without hardware

### PASS criteria

- no monotonic application allocation growth in automated tests
- no obvious state leak
- history restore/clear works
- scheduler transitions are deterministic
- bounded capacities are enforced
- memory metrics are exposed/loggable

### Evidence

Record:

```text
50+ cycle result:
History tests:
Scheduler tests:
Free heap evidence available:
Minimum heap evidence available:
Largest block evidence available:
Task stack evidence available:
Known hardware-only memory items:
```

---

## Gate 6 — Release Builds and Artifacts

### Purpose

Produce all artifacts that can be completed without a physical device.

### Required validation

Run applicable upstream validation:

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware
./tools/validate.sh
```

### Required artifacts

- production firmware build
- simulator firmware build
- production merged Full Flash
- simulator merged Full Flash
- host test report
- Simulator validation report
- firmware size report
- partition usage report
- completed `docs/execution-report.md`
- implementation provenance/deviation report
- `THIRD_PARTY_NOTICES.md` when applicable
- hardware-unverified list

### Full Flash requirements

Simulator image:

- ESP32-C3 merged image
- starts at `0x0`
- <= 8 MiB
- contains required bootloader/partition/app data

Production image:

- valid for the target 8 MB layout
- verified by repository build/merge tooling

### PASS criteria

Gate 6 is PASS when all non-hardware deliverables succeed.

Physical BLE/RF validation does not block Gate 6.

The IEEE redistribution review recorded in `docs/donor-audit.md` is a public/commercial release review item. It does not block a local development build, but Codex must carry it into the final unresolved-release-risk section and must not claim that redistribution clearance has been obtained.

### Evidence

Final report:

```text
Build: PASS / FAIL / NOT RUN
Host tests: PASS / FAIL / NOT RUN
Simulator tests: PASS / FAIL / NOT RUN
Device tests: NOT RUN
BLE RF/HARDWARE: UNVERIFIED — REAL DEVICE REQUIRED
```

Also record:

```text
Production Full Flash:
Simulator Full Flash:
Firmware size:
Partition usage:
Final git status:
Implementation deviations:
```

---

# Post-Gate Hardware Acceptance

The separate physical-device verification contract is defined in:

`docs/hardware-acceptance.md`

Hardware Acceptance is not Gate 7 and does not block Gate 0–6 completion.

It converts specific `UNVERIFIED — REAL DEVICE REQUIRED` items into hardware evidence after a physical AI Passport is available.

# Repository Guidelines for AI Agents

This file is the mandatory entry point for AI-assisted work in the Signal Atlas project.

It inherits the FoloToy AI Passport engineering baseline and adds Signal Atlas-specific product, donor, architecture, UI, resource, simulator, and validation rules.


### Document authority

Use each project document for its own domain:

- `AGENTS.md` — mandatory execution behavior, upstream routing, safety, and repository rules
- `docs/donor-audit.md` — authoritative donor selection, pinned revisions, license/reuse boundaries
- `docs/implementation-spec.md` — authoritative product, architecture, data-model, UI, and resource specification
- `docs/gates.md` — authoritative Gate 0–6 PASS criteria and evidence requirements
- `docs/hardware-acceptance.md` — authoritative physical-device acceptance checks
- `docs/codex-autopilot-goal.md` — execution entry point only; it must not override the files above

If two files appear to conflict, follow the file authoritative for that subject and report the inconsistency rather than silently inventing a third interpretation.

## 1. Required context

Before any code change:

1. Determine whether the workspace is already a Git repository.
2. If it is a Git repository, run `git status --short --branch`, record HEAD, and preserve unrelated user changes.
3. If it is not a Git repository, apply the **Workspace bootstrap** procedure below before implementation.
4. Read:
   - `AGENTS.md`
   - `README.md`
   - `docs/donor-audit.md`
   - `docs/implementation-spec.md`
   - `docs/gates.md`
   - `docs/hardware-acceptance.md`
5. Read `docs/development/ai-guide.md`.
6. Read the affected public headers, implementations, and neighboring code.
7. For hardware-facing changes, read:
   - `docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.md`
   - `components/bsp/include/bsp_pins.h`
   - affected BSP source files
8. For build, partitions, or firmware artifacts, read:
   - `docs/development/engineering/build-and-test.md`
   - `docs/development/engineering/firmware-layout.md`
   - `sdkconfig.defaults`
   - `partitions.csv`
9. Inspect only the donor files/reference branches explicitly named by `docs/donor-audit.md` or required by upstream task routing. Do not perform broad donor discovery or reopen architecture research.
10. Check that the five upstream AI Passport skills required by the base repository are installed and available:
   - `passport-develop`
   - `passport-setup`
   - `passport-build`
   - `passport-device-test`
   - `passport-debug`
11. Use only the skills needed for the current task.

Do not load every repository document by default. Follow task-specific routing.


### Workspace bootstrap

The execution workspace may initially contain only the Signal Atlas planning documents and no Git repository/source tree.

The canonical planning files are:

```text
AGENTS.md
README.md
docs/donor-audit.md
docs/implementation-spec.md
docs/gates.md
docs/hardware-acceptance.md
docs/codex-autopilot-goal.md
```

If the workspace is already a valid FoloToy AI Passport Git worktree, do not bootstrap it again.

If the workspace is **not** a Git repository and contains the canonical planning files above, Codex is authorized to bootstrap the source tree as part of Gate 0.

Before bootstrap, classify any additional root entries.

### Benign metadata — preserve and continue automatically

The following do **not** require user approval and must not stop Autopilot:

- `.DS_Store`
- `Thumbs.db`
- `desktop.ini`
- AppleDouble metadata matching `._*`

Leave them in place when safe, or move them temporarily only if required by the bootstrap operation, then restore them. Do not delete them merely to satisfy bootstrap.

Do not ask the user or C2C for approval for these benign metadata files.

### Potential user content — protect and stop destructive actions

Any other unexpected file or directory that could plausibly contain user work, source code, configuration, documents, data, assets, credentials, or build state must be preserved. If bootstrap would overwrite or destroy it, stop the destructive bootstrap step and report the exact conflict.

Do not stop merely because an extra entry exists; stop only when there is a real overwrite/data-loss risk that cannot be avoided safely.

### Bootstrap procedure

1. Copy the seven canonical planning files to a safe temporary location.
2. Preserve benign metadata and any unrelated user content.
3. Confirm that populating the source tree will not overwrite protected user content.
4. Populate the workspace root from:
   `https://github.com/FoloToy/ai-passport.git`
5. Pin the source baseline to:
   `33d3d1d93a1125b356b47b6d83a7a60121be801e`
6. Use `upstream` as the FoloToy remote name where practical.
7. Create/switch to:
   `feature/radio-explorer`
   from the pinned baseline.
8. Restore the seven canonical Signal Atlas planning files over the upstream tree.
9. Preserve/restore benign metadata such as `.DS_Store`.
10. Do not commit.
11. Verify the resulting Git status before any implementation work.

Bootstrap is execution, not research. Do not substitute a newer upstream commit for the pinned baseline unless a concrete implementation blocker is documented.


### Autopilot approval policy

Routine execution must not pause for user approval.

The user has pre-authorized non-destructive development and validation environment preparation required to execute Gate 0–6. When required by the frozen plan, this standing authorization includes:

- downloading and checking out the pinned ESP-IDF v5.5.3 source/tool environment;
- downloading and checking out the pinned FoloToy Passport Simulator revision from `docs/donor-audit.md`;
- downloading ordinary open-source build/test dependencies required by repository validation;
- creating workspace-local or user-local caches, virtual environments, SDK/tool directories, build directories, and temporary directories;
- activating the required SDK for the current execution process; and
- retrying builds/tests after environment correction.

Environment preparation is execution, not a new research phase. Prefer, in order: an already-installed exact compatible toolchain; otherwise a workspace-local checkout/tool environment; otherwise a user-local cache/tool installation. Do not replace or mutate an unrelated existing SDK merely because the pinned version is needed.

This authorization does not permit sudo or privilege escalation, destructive system-wide package replacement, global shell startup-file changes merely to activate a toolchain, changing the user's global/default ESP-IDF installation/version, deleting meaningful user content, obtaining unavailable credentials, physical flashing, commit, push, tag, publish, or release.

Do not ask for permission for:

- benign metadata handling
- normal file creation/modification required by the frozen plan
- ordinary Gate-to-Gate progression
- pinned source/tool downloads covered above
- workspace-local or user-local environment setup covered above
- test/build retries
- localized fixes needed to satisfy Gate criteria
- non-destructive workspace inspection

Ask/stop only when:

- continuing would create a credible risk of overwriting or deleting meaningful user content;
- the frozen plan is impossible to execute without a material product/architecture change;
- the only viable environment setup requires sudo/privilege escalation, destructive system-wide changes, or mutation of unrelated global tool installations;
- credentials, destructive external actions, physical flashing, push/tag/release, or another separately-authorized action would be required;
- no independent work can continue around the blocker.

When independent work remains, continue it instead of waiting.

### C2C review model

C2C is an independent asynchronous planning/review layer, not a synchronous Gate approval layer. Codex Autopilot must not wait for C2C confirmation after a Gate.

At the end of each Gate:

1. run the required validation;
2. update `docs/execution-report.md`;
3. fix blocking implementation/test failures where possible;
4. mark the Gate only according to `docs/gates.md`; and
5. automatically continue when the Gate satisfies its evidence contract.

If C2C later returns a corrective finding, treat it as an independent review finding: fix the affected implementation, rerun all affected Gate validation, update `docs/execution-report.md`, and continue autonomously toward Gate 6.

A missing validation environment may prevent a Gate from being marked PASS, but it is not a reason to request approval if that environment can be prepared under the standing authorization above. If a Gate remains blocked by something that cannot safely be resolved, continue later independent implementation/evidence work where dependency ordering permits. Never falsely mark a blocked Gate or dependent final validation PASS.

Project execution documentation is maintained in canonical English `.md` files unless the user explicitly requests a translation. Do not create or maintain paired `.zh_CN.md` execution documents unless explicitly requested.

## 2. Hardware and source-of-truth baseline

Target:

- ESP32-C3
- 8 MB Flash
- no PSRAM
- ESP-IDF 5.5.3
- ST7789P3 240 × 320 display
- three-button ADC input
- Wi-Fi 2.4 GHz
- Bluetooth LE

Hardware facts follow this priority:

```text
product specification / measured result
  > components/bsp/include/bsp_pins.h
  > BSP public headers and implementation
  > hardware guide
  > README / demo code
```

Never infer board wiring, GPIO assignments, polarity, bus ownership, panel configuration, or hardware limits from a generic ESP32-C3 board.

Reusable board behavior belongs in `components/bsp`.

Signal Atlas pages, state machines, animations, app tasks, scanners, models, persistence, and product-specific logic belong in the application layer unless they are demonstrably reusable board capabilities.

## 3. Signal Atlas product contract

Signal Atlas is:

- completely offline at runtime
- read-only
- a Wi-Fi AP/BSSID explorer
- a passive BLE advertisement explorer
- an explanatory metadata tool

It must not:

- connect to discovered Wi-Fi networks
- connect to BLE devices
- perform GATT interrogation
- use Wi-Fi monitor mode
- sniff ordinary Wi-Fi clients
- upload observations
- require cloud services
- download registries at runtime
- track people across randomized BLE identities
- estimate physical distance from RSSI
- implement Evil Twin or attack detection
- produce security scores
- keep full packet logs
- add RF Tamagotchi or unrelated gamification in v0.1

Do not expand product scope without an explicit user request.

## 4. Mandatory donor-first rule

Low-level behavior must not be reimplemented from scratch when compatible donor code already exists.

The donor audit has already been completed by the planning/review layer. Before implementing scanner, parser, protocol, lifecycle, or lookup code, read `docs/donor-audit.md` and follow its frozen reuse map. Do not repeat donor research unless a concrete incompatibility is discovered.

Follow the frozen mapping in `docs/donor-audit.md`. The execution source order is:

1. pinned FoloToy AI Passport baseline
2. ESP-IDF v5.5.3
3. frozen reelyActive iBeacon/Eddystone logic and test vectors
4. pinned Nordic Bluetooth Numbers Database inputs
5. IEEE MA-L / MA-M / MA-S public listings
6. ESP32-BLECollector as reference-only

Prefer direct reuse or small adaptation only where `docs/donor-audit.md` authorizes it and when:

- the license permits it;
- the architecture matches ESP-IDF / ESP32-C3 constraints;
- the implementation does not introduce incompatible runtime dependencies;
- the code is smaller and safer than a rewrite.

Do not import an entire donor architecture merely because one subsystem is useful.

### ESP32-BLECollector boundary

Suitable concepts or small code sections may include:

- passive BLE scanning patterns
- observation-to-identification flow
- dedupe / first-seen / last-seen concepts
- vendor/company lookup concepts

Do not introduce:

- Arduino framework dependency
- SQLite
- SD/FAT storage
- WROVER/PSRAM assumptions
- donor UI frameworks
- unrelated network update features

### Donor provenance

`docs/donor-audit.md` is a pre-authored planning artifact and is the implementation source of truth for donor selection. Codex may append implementation provenance (for example exact copied ranges or resulting notice files), but must not replace the frozen donor decisions without a concrete blocker.

For each frozen donor actually used in the implementation, append implementation provenance without changing the pre-researched donor decision:

- repository URL
- pinned commit SHA
- source file(s)
- exact code/data actually reused or adapted
- resulting local file(s)
- required notice/license action
- any minimal deviation forced by implementation incompatibility

Preserve all required copyright and license notices for directly reused or substantially adapted code/data. Generate `THIRD_PARTY_NOTICES.md` when the final included material requires notices.

No scanner implementation Gate may pass unless the implementation matches the frozen donor map or documents a concrete, minimal deviation.

## 5. Mandatory UI redesign

Signal Atlas is a derivative application and must have its own UI.

Do not reuse, rename, recolor, or lightly modify the upstream demo test menu or `ui_pixel` demo visual shell as the finished application.

Permitted reuse:

- BSP APIs
- ordinary LVGL widgets
- isolated math/utility logic
- input patterns
- lifecycle/concurrency patterns

The final product UI is:

**Instrument Green + Mini Radar**

Primary views:

1. Nearby
2. Detail
3. History

Secondary UI uses one lightweight modal/overlay system.

Controls:

- UP = previous / scroll up
- DOWN = next / scroll down
- OK click = open / select / confirm
- OK long = back / close
- OK long on Nearby = Quick Menu

Do not add double-click or unrelated long-press interactions without explicit product need.

### Mini Radar constraints

- approximately 24 × 24 px
- visible only on Nearby
- active only in `SCANNING`
- target roughly 6–8 FPS
- redraw only its local region
- no dedicated bitmap framebuffer
- no expensive alpha/compositing effects
- pause/freeze when scanning is paused
- stop on Detail and History
- Scan Pulse is the only approved visual fallback if measured performance requires degradation

The radar is visual feedback only. It must not own or drive radio state.

## 6. Evidence integrity

Every user-facing identification belongs to:

- `OBSERVED`
- `RESOLVED`
- `POSSIBLE`

`OBSERVED` means directly present in radio data.

`RESOLVED` means deterministic lookup or standards-based decoding.

`POSSIBLE` means heuristic inference and must be explicitly labeled as such.

Omit `POSSIBLE` entirely when there is no useful inference.

Never convert an unreliable guess into a factual label.

Bluetooth Company ID describes manufacturer-specific advertising data ownership; do not automatically label the physical device manufacturer from that field alone.

## 7. Identity rules

Wi-Fi identity:

```text
BSSID
```

BLE identity:

```text
address + address_type
```

Do not merge rotating/randomized BLE addresses into a guessed physical-device identity.

`NEW` means absent from persistent local Signal Atlas history.

It does not mean a newly manufactured device, newly powered device, or newly present human.

Do not perform IEEE vendor lookup on locally administered/private MAC addresses.

## 8. Scanner architecture

Real and mock scanners must emit the same normalized observation model.

Preferred conceptual flow:

```text
scanner backend
  -> bounded observation/event
  -> normalize
  -> nearby dedupe/update
  -> enrichment
  -> history
  -> UI model
```

Required backend separation:

- real ESP-IDF Wi-Fi scanner
- real NimBLE BLE scanner
- mock scanner for simulator/test use

UI code must not depend directly on `esp_wifi`, NimBLE GAP callbacks, or simulator-only data.

BLE/Wi-Fi callbacks must stay lightweight and non-blocking.

Callbacks must not:

- touch LVGL
- write Flash
- perform large registry lookups
- build large formatted strings
- perform unbounded allocation
- run expensive protocol parsing

Copy only bounded data and enqueue/post it for application processing.

## 9. Wi-Fi rules

Use the upstream AI Passport Wi-Fi lifecycle and ESP-IDF APIs before writing replacement lifecycle code.

Wi-Fi scanning represents APs/BSSIDs, not nearby Wi-Fi client devices.

v0.1 should cover:

- SSID
- hidden SSID
- BSSID
- RSSI
- channel
- auth/security
- globally/local-administered address status
- IEEE registry lookup when valid

Do not merge multiple BSSIDs merely because they share an SSID.

## 10. BLE rules

Use the upstream AI Passport NimBLE lifecycle and compatible official ESP-IDF passive-scan donor code.

BLE behavior is passive scanning only.

Do not connect or perform GATT procedures.

Keep advertisement snapshots bounded.

v0.1 deterministic decoding includes:

- Company ID
- Service UUID
- Appearance
- iBeacon
- Eddystone UID
- Eddystone URL
- Eddystone TLM

Do not add proprietary Apple/Samsung/Fast Pair/Find My model fingerprinting in v0.1.

## 11. Radio modes and scheduler

Supported product modes:

- `ALL`
- `WI-FI`
- `BLE`
- paused state

`WI-FI`:
- Wi-Fi scanner active
- BLE scan stopped

`BLE`:
- BLE scanner active
- Wi-Fi scan stopped

`ALL`:
- use bounded alternating scan work
- preferred conceptual order:
  `BLE window -> Wi-Fi scan -> BLE window -> Wi-Fi scan`

Centralize timing values as tunable constants.

Do not embed magic scan timings throughout UI or backend code.

## 12. Bounded live data

Prefer fixed-capacity storage.

Initial target:

- total Nearby capacity: approximately 96
- BLE contribution: approximately 64
- Wi-Fi contribution: approximately 32

A shared pool is preferred when simpler.

For this scale, a fixed array and bounded linear lookup are acceptable and may be preferable to a dynamic hash-map implementation.

On overflow, prefer eviction in this order:

1. expired
2. stale
3. oldest/weakest suitable candidate

Do not unexpectedly evict the item currently selected by the user.

When the user begins navigating Nearby, freeze row order temporarily while continuing to update values in place. Resume sorting after inactivity or leaving the list.

Keep this behavior host-testable where practical.

## 13. History

History is a persistent sighting summary, not a packet log.

Initial target:

- approximately 512 identities

Persist compact information such as:

- identity
- protocol
- first session
- last session
- total observation count
- last RSSI
- compact classification IDs/flags

Do not fabricate wall-clock timestamps.

Use session identifiers / monotonic runtime state where needed.

Prefer simple ESP-IDF-native persistence such as an appropriately sized NVS partition/namespace after measuring actual overhead.

Do not introduce SQLite or SD/FAT.

## 14. Registry generation

Large registries are build-time inputs.

Generate compact firmware lookup tables from:

- IEEE public MAC registry data
- Bluetooth Assigned Numbers source data

The device must not parse the source JSON/CSV at runtime.

IEEE lookup must support the applicable longest-prefix model for MA-S / MA-M / MA-L data rather than blindly assuming all assignments are 24-bit OUI entries.

Prefer:

- sorted compact records
- deduplicated string pools
- deterministic generators
- generated metadata with source/revision date
- near-zero runtime heap for registry data

Expose a concise database build identifier in About.

## 15. Memory and Flash rules

This board has no PSRAM.

Do not initialize audio/I2S for Signal Atlas unless unavoidable upstream startup code forces it; the product has no audio feature.

Do not add:

- full-screen RGB565 framebuffer
- runtime JSON database
- SQLite
- unbounded packet history
- large unbounded strings
- unnecessary large/double LVGL buffers

Engineering targets:

- Nearby live store: <= 16 KB
- observation queue: <= 6 KB
- parser/enrichment scratch: <= 6 KB
- history working RAM: <= 8 KB
- registry runtime heap: approximately zero
- own-added steady-state RAM: target <= ~48 KB where practical
- desired steady-state free heap headroom: >= ~48 KB where practical
- desired largest free block: >= ~24 KB where practical

These are engineering guardrails, not fabricated hardware guarantees.

If a target is inappropriate after measurement, document the evidence and adjust deliberately.

Always inspect:

- current free heap
- minimum free heap
- largest free block
- relevant task stack high-water marks

For Flash:

- keep the complete configured image inside 8 MB
- use only a modest dedicated persistent area for Signal Atlas history, initially around 128 KiB unless measurement justifies another value
- keep generated registries compact
- optimize representation before dropping correct registry coverage

## 16. Simulator truthfulness

The current AI Passport Simulator is not authoritative BLE hardware validation.

BLE Controller/RF behavior must remain:

```text
UNVERIFIED — REAL DEVICE REQUIRED
```

Simulator BLE must use the mock scanner path and feed the exact same production model/parsers/history/UI used by real scanners.

Do not create a fake BLE controller layer merely to claim BLE scanner execution.

Simulator can validate:

- startup
- display/UI
- buttons/navigation
- Mini Radar
- mock BLE data
- state machines
- history
- error/empty states
- simulator-supported Wi-Fi path
- Full Flash image behavior

Never report mock BLE or Simulator BLE as real RF verification.

## 17. Testing rules

Keep pure logic independent from ESP-IDF/LVGL where practical and cover it with host tests.

Required host-testable areas include:

- address classification
- identity
- IEEE longest-prefix lookup
- Bluetooth registry lookup
- BLE AD parsing
- iBeacon / Eddystone decoding
- dedupe/update
- fixed-capacity eviction
- selection freeze
- history serialization/versioning
- state transitions
- scheduler decisions
- malformed/oversized observations

Fixtures should include at least:

- globally administered Wi-Fi BSSID
- locally administered Wi-Fi BSSID
- hidden SSID
- public BLE address
- random BLE address
- local name / missing local name
- known / unknown Company ID
- Service UUID
- Appearance
- iBeacon
- Eddystone
- malformed AD structures
- duplicate reports
- RSSI updates
- list overflow
- history restore
- incompatible/corrupt history schema

## 18. Implementation Gates

The authoritative Gate 0–6 definitions, PASS criteria, and evidence requirements are in:

`docs/gates.md`

Codex Autopilot must execute those gates sequentially.

Do not duplicate or reinterpret gate criteria here. If this file and `docs/gates.md` differ on gate details, `docs/gates.md` controls.


## 19. Hardware Acceptance

Physical-device verification is defined only in:

`docs/hardware-acceptance.md`

Absence of a physical AI Passport does not block Gate 0–6.

Do not duplicate or reinterpret the hardware acceptance checklist here. BLE/RF behavior remains `UNVERIFIED — REAL DEVICE REQUIRED` until the corresponding checks in `docs/hardware-acceptance.md` are actually executed.

## 20. Degradation order

If measured memory or rendering performance is insufficient, use this order:

1. reduce live-record capacity
2. reduce raw BLE preview bytes
3. right-size queues/task stacks from measured high-water marks
4. make ALL mode more aggressively alternate complete radio lifecycles
5. replace Mini Radar with the approved Scan Pulse fallback

Do not degrade registry correctness first.

Do not replace explicit evidence labels with vague inference.

## 21. Upstream runtime invariants

Retain the upstream AI Passport requirements:

- LVGL is not thread-safe.
- Non-LVGL contexts must use `bsp_lvgl_lock()` for LVGL access.
- Button callbacks must remain non-blocking.
- Stop producers/tasks/timers that can access a page before deleting its objects.
- Keep testable timing/state logic outside LVGL/ESP-IDF where practical.
- Preserve unrelated user changes.
- Never commit credentials, private keys, personal data, or unsanitized logs.
- Do not erase NVS merely to hide initialization or partition errors.
- Do not treat a successful build as device validation.

Project execution documentation is maintained in canonical English `.md` files unless the user explicitly requests translations.

## 22. Validation and delivery

Use the upstream validation entry point:

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware
./tools/validate.sh
```

Run focused checks while iterating and the complete applicable gate before delivery.

Final handoff must report separately:

```text
Build: PASS / FAIL / NOT RUN
Host tests: PASS / FAIL / NOT RUN
Simulator tests: PASS / FAIL / NOT RUN
Device tests: PASS / FAIL / NOT RUN
Unverified: remaining hardware / instrument / user checks
```

Also report:

- implementation summary
- changed files
- implementation provenance and any donor-map deviation
- `docs/execution-report.md`
- `THIRD_PARTY_NOTICES.md` when applicable
- Gate 0–6 status
- firmware size and partition usage
- Full Flash artifact paths
- available memory evidence
- hardware acceptance checklist
- final git status

Do not create commits, push, tag, publish, release, or flash unless the user explicitly requests it or the active authorized workflow requires it.

If a firmware implementation is complete and a device is available later, follow the upstream on-device testing authorization flow before flashing.

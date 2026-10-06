# Signal Atlas — Codex Control Room Autopilot Execution Goal

Implement **Signal Atlas v0.1** end-to-end for the FoloToy AI Passport.

The planning/review layer has already completed product research, donor research, architecture, UI direction, resource guardrails, test planning, and acceptance planning.

**Your role is execution. Do not redo the research or redesign the product.**

## Mandatory intake

1. Determine whether the workspace root is already a Git repository.
2. If it is not a Git repository, execute the workspace-bootstrap procedure in `AGENTS.md`:
   - preserve the planning files safely;
   - classify additional root entries;
   - preserve and ignore benign OS metadata such as `.DS_Store`, `Thumbs.db`, `desktop.ini`, and `._*` without asking for approval;
   - populate the root from `FoloToy/ai-passport`;
   - pin `33d3d1d93a1125b356b47b6d83a7a60121be801e`;
   - use/create `feature/radio-explorer`;
   - restore the planning files and benign metadata;
   - do not commit.
3. Stop destructive bootstrap actions only when an unexpected entry creates a credible overwrite/data-loss risk for meaningful user content. Mere presence of an extra harmless metadata file is not a blocker.
4. Once a Git worktree exists, inspect:
   ```bash
   git status --short --branch
   git rev-parse HEAD
   ```
5. Preserve unrelated user changes.
6. Read and obey, in this order:
   - `AGENTS.md`
   - `README.md`
   - `docs/donor-audit.md`
   - `docs/implementation-spec.md`
   - `docs/gates.md`
   - `docs/hardware-acceptance.md`
   - `docs/development/ai-guide.md`
   - task-routed upstream documents required by `AGENTS.md`
7. Verify the upstream-required AI Passport skills are available.
8. Perform **Gate 0 — Baseline verification and plan intake**.

Gate 0 is not a research phase. Bootstrap the pinned baseline only when necessary, confirm the supplied plan is compatible with the resulting workspace, report concrete mismatches, and then execute it.

## Frozen donor map

Follow `docs/donor-audit.md`.

In particular:

- use FoloToy BSP in place
- adapt FoloToy `demo_radio.c` for NVS/network preparation
- adapt FoloToy `demo_wifi.c` for Wi-Fi lifecycle/scanning
- adapt FoloToy `demo_ble.c` for NimBLE lifecycle/stop ownership
- adapt only the passive scan + advertisement-report path from ESP-IDF v5.5.3 `blecent`
- do not carry connection/GATT logic from `blecent`
- use ESP32-BLECollector as reference-only, not as the runtime architecture
- port only the frozen iBeacon logic/test vector from reelyActive's Apple decoder
- port only Eddystone UID/URL/TLM logic/test vectors from reelyActive's Eddystone decoder
- generate Bluetooth Company/Service/Appearance tables from the pinned Nordic database inputs
- generate IEEE longest-prefix tables from MA-L/MA-M/MA-S public listings
- keep Simulator BLE on `MockBleScanner`; do not initialize a fake/real BLE Controller in the Simulator profile

Do not choose alternate donors merely because they are easier. If a frozen donor is concretely incompatible, document the issue and make the smallest necessary deviation.

## Frozen product/UI

Execute the supplied `AGENTS.md` and `docs/implementation-spec.md` without reopening:

- completely offline
- read-only Wi-Fi AP/BSSID exploration
- passive BLE advertisement exploration
- `OBSERVED / RESOLVED / POSSIBLE`
- exact identity semantics
- Instrument Green
- Mini Radar
- Nearby / Detail / History
- lightweight modal system
- bounded storage
- no BLE connection/GATT
- no Wi-Fi association
- no monitor mode
- no cloud
- no v0.1 proprietary device fingerprinting
- no cross-random-address tracking

## Execution mode

Proceed autonomously through Gate 0–6 exactly as defined in `docs/gates.md`.

Do not pause for routine approvals.

Gates are internal verification checkpoints, not approval pauses. After each Gate, validate it, update `docs/execution-report.md`, fix blocking failures, and automatically continue to the next Gate. Do not ask the user or C2C to approve normal Gate progression.

C2C may independently review execution while Autopilot continues. A later C2C corrective message is a review finding: fix it, rerun affected validation, update evidence, and continue toward Gate 6.

C2C confirmation is never required for ordinary Gate progression. The user has granted standing authorization for non-destructive environment preparation required by Gate 0–6. Without asking for routine approval, Codex may:

- fetch/check out pinned ESP-IDF v5.5.3;
- fetch/check out the pinned Passport Simulator revision;
- obtain ordinary open-source build/test dependencies required by the frozen plan;
- create workspace-local or user-local tool/cache/build environments;
- activate the pinned SDK for the current execution process; and
- rerun failed validation after environment correction.

Constraints: no sudo or privilege escalation; no destructive system-wide package replacement; do not replace or modify unrelated installed ESP-IDF versions; do not modify global shell startup files merely to activate the SDK; no physical flashing, commit, push, tag, publish, or release.

If an exact required environment is missing, first prepare it under this standing authorization and continue. Do not turn a safely resolvable environment prerequisite into an approval request.

For each gate:

- implement the frozen plan
- reuse the frozen donor code/patterns first
- write only the required glue/product logic
- run focused tests
- fix failures
- record evidence
- continue independent work around genuine blockers

Never wait for C2C approval to enter the next Gate after the current Gate satisfies its evidence contract. If validation is blocked only by a missing safely fetchable/installable environment, prepare that environment and rerun validation automatically. C2C review findings arriving later must be fixed and all affected Gate validation rerun.

Do not perform broad new research unless an implementation blocker requires a narrowly scoped compatibility check.

## Validation

Run:

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware
./tools/validate.sh
```

and the host/Simulator matrices in `docs/implementation-spec.md`.

Produce:

- production build
- simulator build
- production merged Full Flash
- simulator merged Full Flash
- host test results
- simulator validation results
- continuously updated `docs/execution-report.md`
- implementation provenance updates
- `THIRD_PARTY_NOTICES.md` when applicable
- firmware size / partition evidence
- hardware-unverified list

The Simulator cannot validate the BLE Controller/RF path.

Keep:

```text
BLE RF/HARDWARE: UNVERIFIED — REAL DEVICE REQUIRED
```

until the separate `docs/hardware-acceptance.md` contract is actually executed.

Do not flash, commit, push, tag, publish, or release unless separately authorized.

## Final handoff

Report:

```text
Build: PASS / FAIL / NOT RUN
Host tests: PASS / FAIL / NOT RUN
Simulator tests: PASS / FAIL / NOT RUN
Device tests: PASS / FAIL / NOT RUN
Unverified: ...
```

Also provide:

- implementation summary
- changed files
- implementation provenance and any deviation from `docs/donor-audit.md`
- Gate 0–6 status with evidence
- firmware size
- partition usage
- production Full Flash path
- simulator Full Flash path
- available RAM/heap measurements
- unresolved implementation risks
- hardware acceptance status/reference from `docs/hardware-acceptance.md`
- final git status

Begin with Gate 0 verification, then continue automatically through Gate 6.

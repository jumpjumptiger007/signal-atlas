# Signal Atlas v0.1 — Hardware Acceptance

Status: **Deferred until a physical FoloToy AI Passport is available**

This document contains only checks that require or materially benefit from real hardware.

Passing Gate 0–6 does not imply that this document has passed. Gate status is recorded in `docs/execution-report.md`.

---

## 1. Flash and boot

- flash the production Full Flash image using the authorized device-test workflow
- cold boot successfully
- no boot loop
- no watchdog reset
- correct Signal Atlas startup screen
- no demo shell appears
- NVS initialization succeeds without erasing unrelated data

Record:

```text
Firmware artifact:
Commit:
Device identifier:
Flash method:
Boot result:
```

---

## 2. Display and controls

Verify:

- 240 × 320 layout
- no clipping
- no corruption
- Instrument Green contrast is readable
- UP works
- DOWN works
- OK works
- Long OK works
- Quick Menu opens
- Detail scrolling works
- History navigation works

Mini Radar:

- visible only on Nearby
- runs only while scanning
- stops/freeze on Pause
- stops outside Nearby
- does not cause visible UI starvation
- does not create obvious tearing/flicker

---

## 3. Wi-Fi RF acceptance

Use at least one known nearby AP and compare against a trusted reference device.

Verify:

- AP discovered
- SSID
- BSSID
- primary channel
- auth/security mode
- RSSI is plausible
- hidden SSID behavior where available
- locally administered/private BSSID suppresses invalid IEEE vendor lookup
- globally administered BSSID performs correct longest-prefix lookup where registry data exists

Record reference observations.

---

## 4. BLE RF acceptance

Use a trusted BLE scanner such as nRF Connect or equivalent as the comparison reference.

Verify at least:

- known advertiser discovered
- address
- address type handling
- RSSI
- local name when advertised
- TX power when advertised
- Company ID value
- Company ID resolved name
- Service UUID
- Appearance where present
- bounded raw preview

Where possible verify real examples of:

- iBeacon
- Eddystone UID
- Eddystone URL
- Eddystone TLM

Do not interpret Company ID as proof of the physical manufacturer.

Do not merge rotating BLE private addresses.

---

## 5. Mode and coexistence acceptance

Verify:

### WI-FI mode

- Wi-Fi scanning operates
- BLE scanning is stopped

### BLE mode

- BLE scanning operates
- Wi-Fi scanning is stopped

### ALL mode

- BLE and Wi-Fi work through the intended bounded alternating scheduler
- UI remains responsive
- mode does not cause repeated radio initialization failures
- both classes of observations continue to appear over time

### PAUSED

- scan activity pauses
- Nearby data remains visible
- Mini Radar stops
- resume works

---

## 6. Selection stability

Create an environment with multiple changing RSSI values.

Verify:

- automatic sorting occurs when idle
- pressing UP/DOWN freezes row order
- selected item does not jump because another device changes RSSI
- RSSI/metadata can still update in place
- sorting resumes after the defined inactivity/exit behavior

---

## 7. History acceptance

Verify:

- first exact identity is marked new
- repeat observation becomes seen
- reboot preserves history
- session model behaves correctly without a real-time clock
- clear-history confirmation defaults to Cancel
- clear history does not remove current Nearby observations
- clear history does not erase unrelated NVS

BLE randomized-address behavior must remain identity-based, not physical-device-based.

---

## 8. Memory and lifecycle acceptance

Record at minimum:

```text
Boot free heap:
Steady-state free heap:
Minimum free heap:
Largest free block:
Relevant task stack high-water marks:
```

Exercise:

- >= 50 real scan cycles
- repeated WI-FI → BLE → ALL → PAUSE transitions
- repeated Wi-Fi scanner stop/start
- repeated NimBLE scanner stop/start
- repeated Detail/History navigation during scans

Acceptance:

- no crash
- no WDT reset
- no monotonic heap loss
- no severe largest-block degradation trend
- no stuck scanner state
- no UI lockup

Engineering targets from `docs/implementation-spec.md` should be treated as guardrails, not automatic failure thresholds without context.

---

## 9. Duration test

Run at least:

```text
30 minutes continuous operation
```

Prefer ALL mode in a normal RF environment.

Verify:

- no reboot
- no watchdog
- no persistent UI degradation
- no obvious heap leak
- Nearby continues updating
- History remains writable
- mode/pause controls still work at the end

---

## 10. Hardware acceptance report

Final hardware report must state:

```text
Boot: PASS / FAIL
Display/buttons: PASS / FAIL
Wi-Fi RF: PASS / FAIL
BLE RF: PASS / FAIL
ALL scheduler/coexistence: PASS / FAIL
History persistence: PASS / FAIL
50+ cycle lifecycle: PASS / FAIL
30-minute stability: PASS / FAIL
Memory evidence: PASS / FAIL
Remaining unverified items:
```

Only after these checks may the corresponding BLE/RF statements be labeled:

```text
HARDWARE VERIFIED
```

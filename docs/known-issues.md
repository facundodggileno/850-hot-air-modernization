# Known issues and failure analysis — open

This document records **observations**, hypotheses and missing evidence separately. No root cause is considered confirmed unless identified as such.

## P0 — heater may run without physical airflow (UNRESOLVED)

**Observed:** during some COOL / resume sequences, the UI may say AIR 100% although the handle emits little or no air, while a heat value continues to be displayed / heater can remain active. Pressing HEAT again sometimes appears to restore behavior.

**What code reveals:** AIR 100% sets the D3 pump command HIGH; there is no physical air/pressure sensor. Timer1 pulse-count checks for AIR below 100% only establish software-driven gate activity.

**Possible causes, not proven:** pump TRIAC/optocoupler intermittency, low phase-angle torque/stalling, conflicting state transitions, wiring/connector, 5 V rail/EMI.

**Priority:** investigate using [isolated pump diagnostic](../firmware/diagnostics/Hony850Pro_PUMP_ISOLATED_DIAG_V1.ino) with heater **physically disconnected**. Need actual airflow measurement, power-stage feedback and an independent heater interlock before heater tests.

## P0 — overheating / FAULT 2 after airflow changes (REPRODUCED)

V1.3.12 log: heater command drops to 0 during a transition yet RAW rises from ~446 °C to **519.7 °C**; FAULT 2 latches at the 485 °C threshold. Inertia, measurement placement and actual heater power status are not independently discriminated.

V1.3.13 log: after a user PAUSE, RAW rises and trips FAULT 2 near **487 °C**, peaking around **495.7 °C**. Power command 'duty=0' does not prove zero heater current.

## P1 — MAX6675 sudden dropouts / inaccurate samples (REPRODUCED)

Reported temporary disconnected MAX6675 5 V supply; after repair, the empirical outlet-vs-RAW relationship remained approximately 0.68. Live logs include 'nan' and one-off drops, e.g. around 27 °C while the surrounding samples are much hotter. V1.3.15 added a drop guard and median, but further spikes still appear.

**Not established:** whether electrical noise, thermocouple placement/copper junctions, 5 V supply, module fault, or interaction with airflow changes dominates. A repeated plausible sample can still be erroneous.

## P1 — repeated startup / serial anomalies

Earlier V1.3.8 erroneously declared a thermocouple FAULT before its three-sample validation window completed; later branches corrected that. Multiple BOOT headers were recorded with the Nano powered by the station and attached to USB; cause was not isolated. USB DTR may reset the Nano; tied 5 V rails or ground noise may contribute, but do not assume this is the sole cause.

## P1 — state-transition bugs

Earlier V1.3.5 intermittently froze during cooldown → AIR40 → 300 °C → HEAT. V1.3.11 removed arbitrary AIR 100%-only constraints but **exceeded Nano Flash** (user compiler: 32,006 bytes, 104% of 30,720-byte limit). Later branches reduced program complexity; this does not prove the original bug is eliminated.

## P2 — thermocouple calibration and heater integration

The factor '0.68' approximates measured outlet response under specific conditions; it is not an intrinsic sensor calibration. Yihua ~650 W heater adapted into the original Hony air path may create uneven thermal transfer and excessive stored heat. External sensor placement/airflow must be standardized for repeatable tests.

## P2 — H11AA1 zero-cross detector heating

Excess temperature of H11AA1 circuitry was reported; proposed LTV-814 design is not verified and should not be documented as installed.

## Minimum evidence to capture on next tests

- D3 logic level **and** actual pump-side movement/pressure; D4/D5 logic plus measured heater electrical power.
- Time-aligned RAW MAX, median/PID temperature, AIR setting, actual flow sensor result, stand state, HEAT request, arm stage, relay feedback, fault code and reset cause.
- Exact sequence of user button presses and knob transitions.
- MCU power rails and isolation strategy.

Full raw logs: [tests/logs](../tests/logs/).

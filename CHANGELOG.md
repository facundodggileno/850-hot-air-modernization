# Changelog

## 2026-11-12 — Documentation snapshot (not a firmware release)

- Updated the repository from an early ~250 W concept to the actual Hony 850 / Yihua ~650 W experimental prototype.
- Added current BOM with documented vs planned parts.
- Added control architecture, pinout, thermal calibration, safety model, known failure cases, test evidence and development decisions.
- Preserved exact lab source snapshots V1.3.15 and V1.3.12, earlier V1.2.6 reference, and isolated pump diagnostic.
- Preserved logs of the AIR-change and temperature faults.
- **Deliberately excluded circuit diagram/schematic until hardware stabilization.**

## Experimental firmware history (selected)

- **V1.2.6:** Earlier Nano reference with PID/cooldown and Yihua heater.
- **V1.3.3:** 10-cycle autotune experiment with 100% heater capacity; recorded PID Kp 1.994, Ki 0.168, Kd 1.707.
- **V1.3.5:** Experimental 0.68 outlet-temperature scale; intermittent hang observed on cooldown → AIR40 → 300 °C → HEAT.
- **V1.3.7–1.3.10:** MAX power investigation, WDT500, startup self-test, and cooldown resume iterations.
- **V1.3.11:** Allowed AIR20–100 heating; user AVR build reported **32,006 B vs 30,720 B limit**, so it did not fit.
- **V1.3.12:** Slim diagnostic build; boot stress passed in user log; FAULT 2 / overtemperature observed.
- **V1.3.13:** Auto-resume after airflow changes, trace of manual button/cooldown, further FAULT 2.
- **V1.3.15:** Repromod-inspired SAFE latch, ~3 s airflow-change pause, sensor-drop guard and richer logging; **still not stable**.

The history above records observed progress, **not test certification**.

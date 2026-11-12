# Thermal characterization — provisional / device-specific

The MAX6675 reads a K-type thermocouple positioned inside the modified handpiece. The external probe is placed in the nozzle air stream. These positions sample different temperatures; the 0.68 relationship is **not** a correction to MAX6675 sensor gain or universal physical law.

## External probe comparison (historical)

The user checked the external thermometer in boiling water (~96–97 °C in local conditions). Reported results include:

| MAX RAW set/read (°C) | Airflow setting | External nozzle reading (°C) |
|---:|---:|---:|
| 80 | 100% | 60 |
| 150 | 100% | 104 |
| 230 | 100% | 157 |
| 300 | 100% | ~196 (other run ~222) |
| 350 | 100% | ~239 |
| 380 | 100% | ~253–256 |
| 420 | 100% | ~285 |
| 440 | 100% | ~300 |
| 350 | 40% | ~235 |
| 380 | 40% | ~248 |

Readings were taken across multiple sessions with differing measurement placement, dwell and control software. Do not mix them into a high-accuracy calibration curve without synchronized measurements and standardized positioning.

**Experimental display scaling: estimated outlet ≈ MAX RAW × 0.68.** The inverse is applied for the UI's default estimated-outlet setpoint. The actual absolute overtemperature protection uses unscaled RAW.

## PID measurements

A 10-cycle autotune experiment with the replacement heater yielded approximately **Kp 1.994, Ki 0.168, Kd 1.707**. Normal closed-loop output was reported stable during some AIR100 tests, but overshoot appeared, especially during AIR changes and possible airflow loss. Gains are **not approved for unattended operation** or other heaters.

## Next characterization protocol

Repeat only after physical airflow proving and independent cutoff are added; measure RAW and a calibrated reference under known stable airflow, fixed probe position, steady-state dwell, and recorded ambient. Capture heater current and actual pressure/flow. Document sensor wiring/cold junction placement and EMI mitigation. Do not compensate for an unsafe flow condition by changing display scaling alone.

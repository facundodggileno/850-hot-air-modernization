# Experimental test evidence — not certification

All observations originate in prototype bench logs / user-reported external probe tests. As of 2026-11-12 no independent electrical acceptance, current sensing, physical airflow verification or certified temperature calibration has been completed.

## Startup self-test

- V1.3.12 log: 12,288 synthetic state-machine cases pass, zero-cross observed ~101 events/s, 124/124 pump gate begin/end events in partial-air stage; RAM reported 1,214 bytes available.
- V1.3.13 log: 12,288 pass, ~100 zero-cross events/s, 124/124 gates; RAM reported ~1,208 bytes.
- V1.3.15 log: a run passed 12,288 cases, ~100 ZC/s, 124/124 pump gates; RAM reported ~1,143 bytes.
- **Meaning:** the model and pulse counters ran. **It does not prove the pump is moving air or the heater cut physically.**

## Reproduced FAULT 2 events

| Firmware | Sequence | RAW / result |
|---|---|---|
| V1.3.12 | AIR40 heat, later flow transition and cooldown | 485 °C trips FAULT 2; RAW peaks ~519.7 °C |
| V1.3.13 | HEAT → AIR70 → PAUSE, later HOT/cooldown | FAULT 2 at ~487 °C; RAW peaks ~495.7 °C |
| V1.3.15 | Multiple AIR changes/resume and transient MAX drop guard events | Some heater output commands remained high after suspected sensor/flow anomalies; physical flow and heater current not captured |

## Compilation history

- V1.3.11: Arduino IDE reported 32,006 bytes vs allowed 30,720 bytes, failed size limit.
- V1.3.12/V1.3.13/V1.3.15 executed and emitted logs on Nano, but **there is no reproduced toolchain build/version lock or independent CI result in this repository**.
- Syntactic/model test passes are not interchangeable with real-AVR compilation, ISR timing measurement or hardware validation.

## Selected raw logs

- [V1.3.12](../tests/logs/2026-11-08-v1.3.12-thermal-fault2.txt)
- [V1.3.13](../tests/logs/2026-11-08-v1.3.13-flow-resume-fault2.txt)
- [V1.3.15](../tests/logs/2026-11-08-v1.3.15-safe-capture.txt)

Each row of the serial CSV should be interpreted according to its own header; fields differ among firmware versions. 'airActual' is a commanded value, not measured flow.

## Missing validation

Real thermal sensor repeatability with calibrated probe; pump restart/flow at every phase setting; live relay contact/power measurement; fail-safe MCU reset/off handling; fault injection; EMC and long-duration endurance; independent thermal protection.

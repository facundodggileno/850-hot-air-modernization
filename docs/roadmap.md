# Roadmap — Nano-first; schematic postponed

## Phase A — complete the Arduino Nano control baseline (CURRENT)

- [x] Prototype Nano, LCD, encoders, MAX6675, zero-cross and pump/heater outputs.
- [x] Log state machine and prototype PID/cooldown.
- [x] Prototype self-test, WDT500, SAFE latch and serial fault snapshots.
- [x] Record reproducible startup and partial-air test results.
- [ ] Eliminate heater power while the pump has **no measured airflow**.
- [ ] Instrument and prove pump control from AIR 20–100%, including stop/restart.
- [ ] Validate actual relay contact behavior, optotriac power stage and startup/reset OFF.
- [ ] Stop plausible-but-wrong MAX readings from becoming unsafe heater demand.
- [ ] Fix cooldown and manual resume logic without bypassing safety.
- [ ] Measure margin: Flash/SRAM/ISR jitter/I2C hangs and avoid dynamic allocation.
- [ ] Implement reproducible build and test harness for exact Nano core/library versions.
- [ ] Long-duration and fault-injection tests **only with independent barriers in place**.

## Phase B — independent protection and hardware stabilization

- [ ] Install and validate independent airflow/pressure proving.
- [ ] Add independent thermal cutoff and heater power/relay feedback.
- [ ] Resolve H11AA1 heating and low-voltage/USB return path.
- [ ] Verify protective earth, insulation, fuses, wire ratings, EMI/EMC and mechanical heater fit.
- [ ] Freeze the electrical BOM and physical pin assignment.
- [ ] Record a complete as-built circuit with verification measurements.

## Phase C — public reproducibility

- [ ] **Publish final circuit schematic, after Phase B.** Not included in this snapshot.
- [ ] Step-by-step build guide and validated BOM with supplier part numbers.
- [ ] Bench test acceptance criteria and verified test results.
- [ ] Release/version tags and license choice.
- [ ] Optional PCB and mechanical mounting files.

## Phase D — only after Nano baseline

- [ ] Decide whether Mega2560 or ESP32-S3 is warranted.
- [ ] Port state machine into a hardware abstraction layer.
- [ ] For ESP32-S3: address 3.3 V I/O, MAX6675/LCD level shifting, optocoupler drive, RTOS task/watchdog design and independent interlocks.
- [ ] Preserve regression logs/behavior and validate equivalence before enabling heater.

There is **no committed date** for public release or schematic.

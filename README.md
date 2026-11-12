# Hony 850 Pro — 850 Hot Air Modernization

**An experimental open-source retrofit for 850-style hot-air rework stations.** The reference prototype is a Hony 850 with an Arduino Nano ATmega328P. The long-term goal is an affordable, documented, serviceable controller; the current priority is to make the Nano implementation reliable **before** considering Mega 2560 or ESP32-S3.

**Status (2026-11-12): LAB / NOT SAFE FOR GENERAL USE.** Firmware V1.3.15 is an investigation snapshot, **not a verified stable release**. A serious unresolved issue is that the interface may indicate commanded airflow even when the pump is not physically moving air. Unstable MAX6675 samples and high-temperature excursions have also been observed. Do not power the 650 W heater for unattended testing or rely on software as the only safety barrier.

[Documentación en español](README.es.md) · [BOM](BOM.md) · [Architecture](docs/architecture.md) · [Safety](docs/safety.md) · [Known issues](docs/known-issues.md) · [Test results](docs/tests-and-results.md) · [Roadmap](docs/roadmap.md)

## Reference prototype (as built / reported)

- **Base:** Hony 850 station, original diaphragm-style air pump and existing enclosure.
- **Heater:** replacement Yihua 8786D-style 220 VAC element, approximately 650 W, approximately 75 ohms measured; replaces the failed ~250 W original heater. This is a modified combination, not the original product rating.
- **Control:** 5 V Arduino Nano ATmega328P, updated bootloader (Optiboot reported).
- **Temperature:** K-type thermocouple in the handpiece and MAX6675 breakout; VCC reconnected to 5 V. Sensor was moved ~5 mm toward airflow. There are copper-wire sections/splices in the thermocouple run.
- **UI:** 16x2 I2C LCD (PCF8574/HW-61 class), two rotary encoders and buttons, stand microswitch.
- **Power switching:** heater MOC3083 zero-cross optotriac + BTA12-600B; pump MOC3023 random-phase optotriac + triac driven with zero-cross/Timer1; separate relay output controlled through BC337. Actual wiring, relay contacts and fail-safe behavior still require physical verification.
- **Zero-cross:** H11AA1 detector presently used; excessive component heating has been reported. A replacement detector is **proposed, not installed**.

## Firmware snapshots

- [V1.3.15 — current diagnostic branch](firmware/lab/Hony850Pro_V1_3_15_LAB_REPROMOD_SAFE_FLOW3S.ino): SAFE latch, 3 s airflow-change pause, live PID, watchdog, self-test; unresolved pump / temperature fault.
- [V1.3.12 — comparison diagnostic](firmware/lab/Hony850Pro_V1_3_12_LAB_DIAG_SLIM_NANO.ino): smaller Nano test branch, useful to compare earlier fault behavior.
- [V1.2.6 — historical reference](firmware/reference/Hony850Pro_V1_2_6_YIHUA_SAFE_COOL68.ino): earlier implementation, **not** a validated safety fallback.
- [Isolated pump diagnostic](firmware/diagnostics/Hony850Pro_PUMP_ISOLATED_DIAG_V1.ino): exercises pump commands with heater control outputs kept low; the heater must still be **physically disconnected** before tests.

See [firmware notes](firmware/README.md) for build and version status. No AVR toolchain or physical verification was completed during this documentation update.

## What the prototype currently does

PID heater control; programmable airflow; display, two encoders and stand sensing; automatic cooldown; resume from cooldown; 3-second interruption/re-arm when changing airflow; MAX6675 validity checks; temperature-based fault conditions; 500 ms AVR watchdog; zero-cross heartbeat; startup model tests (12,288 synthetic cases) and Timer1 pulse counters.

These features are **software claims**, not a guarantee that the connected actuator performed them. In particular, the variable historically logged as 'airActual' is **commanded airflow**, not a measured flow.

## Development strategy

1. Establish real pump airflow and power-stage behavior at 20–100%; reproduce bugs safely with the heater supply disconnected.
2. Fix the existing Nano state machine, sampling, I2C resilience, and hardware/software fault distinction.
3. Add and validate **independent airflow and thermal protection** before calling any heater-enabled build safe.
4. Measure complete electrical characteristics, freeze BOM, and **only then publish the schematic/circuit diagram**.
5. Consider an ESP32-S3 port after the Nano design reaches a reproducible and characterized baseline.

## Repository map

- BOM.md — parts, evidence/status and unknowns.
- docs/ — architecture, safety, known issues, measurements, tests, roadmap and decisions.
- firmware/ — exact lab snapshots and isolated diagnostics.
- tests/logs/ — dated captures containing the observed faults.
- hardware/ — hardware notes and pin assignment; **no circuit schematic yet**.
- mechanical/ — future mechanical assets.

## Safety and licensing

**Mains voltage and a high-power heater can cause electric shock, burns and fire.** This is a development record, not a wiring guide, reference design or certified product. A software command to switch off a TRIAC or relay does not prove physical disconnection. The project needs independent flow detection and fail-safe heater disconnection.

Repository license is not yet selected. Do not assume a license for source redistribution beyond what GitHub permits by default.

Maintained by Facundo D. Gileno / PC Service Computación.

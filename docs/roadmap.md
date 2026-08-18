# Project Roadmap

This document tracks the planned development stages for the 850 Hot Air Modernization project.

## Phase 1 — Characterize the original station

- Document the original wiring and control architecture.
- Measure heater power, pump behavior and relevant voltages.
- Identify the temperature sensor type and signal range.
- Identify which parts of the original circuit can be reused safely.

## Phase 2 — Prototype control hardware

- Build the first controller around an Arduino Nano.
- Implement heater control.
- Implement pump / airflow control.
- Add handpiece stand detection.
- Validate fail-safe behavior before closed-loop testing.

## Phase 3 — Firmware

- Temperature acquisition and filtering.
- Closed-loop temperature regulation.
- User-adjustable temperature and airflow.
- Stand detection state machine.
- Immediate heater shutdown on stand detection.
- Automatic cooldown and pump shutdown.
- Sensor and over-temperature fault handling.

## Phase 4 — Integration

- Final wiring diagram.
- Final BOM.
- Enclosure / front-panel integration.
- Long-duration thermal testing.
- Repeatability and safety testing.

## Phase 5 — Portability

- Document differences between compatible 850-style stations.
- Separate hardware-specific code from control logic.
- Port firmware to an ESP32-class controller if useful.

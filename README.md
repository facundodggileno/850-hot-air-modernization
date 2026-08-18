# 850 Hot Air Modernization

Open-source modernization project for classic **850-style hot-air rework stations** that use a pump in the main unit.

The current development platform is a **Hony 850**, but the project is being documented with the goal of making the design adaptable to other similar 850-style stations whenever their electrical architecture is compatible.

> **Project status:** experimental / work in progress. The hardware and firmware are still being designed and tested.

## Goals

The objective is to keep the useful parts of a traditional 850 station while replacing or improving its control system with modern features:

- Closed-loop temperature control.
- Adjustable airflow control.
- Automatic handpiece/stand detection.
- **Immediate heater shutdown** when the handpiece is placed in the stand.
- Automatic cooldown: the air pump keeps running after heater shutdown until the handpiece reaches a safe temperature.
- Automatic pump shutdown after cooldown.
- Simple digital user interface and display.
- Safety-oriented firmware behavior.
- First implementation based on an **Arduino Nano**.
- Firmware architecture intended to be portable later to an **ESP32**.

## Current test station

The prototype is currently being developed around a Hony 850-style station with:

- Pump located inside the station body.
- Nominal station rating around 300 W.
- Heater measured at approximately 250 W on the current unit.
- Existing analog controls to be progressively replaced or interfaced with a digital controller.

These values describe the current test unit and **must not be assumed to be identical on every 850-style station**.

## Planned operating sequence

A key feature of this modernization is automatic cooldown:

1. The station operates normally while the handpiece is in use.
2. A stand sensor detects when the handpiece is returned to its holder.
3. Heater power is disabled immediately.
4. The air pump continues running to cool the heater and handpiece.
5. Temperature is monitored during cooldown.
6. Once a safe temperature is reached, the pump is turned off automatically.
7. Removing the handpiece from the stand returns the station to its normal operating state.

The exact thresholds, timing and safety logic will be documented as testing progresses.

## Repository structure

```text
.
├── README.md
├── BOM.md
├── CHANGELOG.md
├── docs/
│   ├── roadmap.md
│   └── safety.md
├── hardware/
│   └── README.md
├── firmware/
│   └── README.md
└── mechanical/
    └── README.md
```

Additional folders will be added when schematics, wiring diagrams, PCB files, firmware releases, measurements and printable parts are ready.

## Documentation philosophy

The goal is not only to publish a finished modification. This repository will also document:

- Original station measurements.
- Reverse-engineering notes.
- Design decisions and alternatives considered.
- Failed approaches when they are useful to understand the final design.
- Safety considerations.
- Firmware behavior.
- Wiring and assembly instructions.
- Bill of materials and component alternatives.

This should make the project reproducible instead of requiring readers to reconstruct the development process from scattered forum posts or conversations.

## Safety warning

**This project involves mains voltage, high current, high temperatures and heating elements capable of causing fire, electric shock, burns or equipment damage.**

Do not reproduce any part of the project unless you understand the relevant electrical and thermal risks. The repository is a development record and is not currently a certified commercial design.

See [`docs/safety.md`](docs/safety.md) as the project develops.

## License

A license has not yet been selected. Licensing will be decided before the project reaches a stable public release.

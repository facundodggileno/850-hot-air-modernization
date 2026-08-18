# 850 Hot Air Modernization

An open-source project to give **old and inexpensive 850-style hot-air rework stations modern features** without having to replace the entire station.

The current prototype is being developed using a **Hony 850**, but the goal is to make the project useful for other similar 850-style stations whenever possible.

> **Project status:** Work in progress. The hardware and software are still being developed and tested.

## What is the idea?

Classic 850-style hot-air stations are simple, cheap and often still very useful, but their controls and safety features are outdated compared with modern stations.

Instead of replacing everything, this project aims to **reuse as many useful original parts as possible** and replace the old control system with a modern digital one.

The goal is to make these stations easier and safer to use while keeping the project affordable and relatively easy to reproduce.

## Planned features

The project is planned to include:

* **PID temperature control** to keep the selected temperature more stable.
* Digital temperature adjustment.
* Digital airflow adjustment.
* **Rotary encoders** instead of the original analog control knobs.
* A display showing temperature, airflow and station status.
* Automatic detection when the hot-air handle is placed in its holder.
* Immediate heater shutdown when the handle is placed in the holder.
* Automatic cooldown using the air pump.
* Automatic pump shutdown once the handle reaches a safe temperature.
* Additional software safety protections.
* Arduino Nano as the first controller.
* Possibility of adapting the project to ESP32 later.

## How will it be built?

The intention is **not to discard everything inside the original station**.

Parts that are still useful, such as the air pump, heater, transformer or other suitable components, may be reused depending on the station.

The original analog control electronics will be replaced by a new digital control system.

During the first development stages, the new control board will be built using **perforated prototyping board**, making it easier to modify and test the circuit.

Once the design is proven and stable, a dedicated PCB may be designed.

## Temperature control

Temperature will be controlled using a **PID controller**.

In simple terms, the controller continuously checks the actual air temperature and adjusts heater power automatically to keep it as close as possible to the temperature selected by the user.

This should provide more stable and predictable heating than the original basic analog control.

## Automatic cooldown

One of the main improvements will be automatic handle detection.

When the hot-air handle is placed in its holder:

1. The station detects that the handle has been returned.
2. Heater power is turned off immediately.
3. The air pump continues running.
4. The remaining heat is removed from the heater and handle.
5. Once a safe temperature is reached, the pump turns off automatically.

This is similar to the behavior found in many newer hot-air stations.

## Current development station

The first prototype is being developed using a **Hony 850-style hot-air station**.

The current unit uses:

* An air pump located inside the station.
* A station rated at approximately 300 W.
* A heater measured at approximately 250 W.

These specifications only describe the station currently being used for development. Other 850-style stations may be different internally.

## Who is this project for?

The documentation is intended to be understandable for **electronics repair technicians and hobbyists with basic or intermediate electronics knowledge**.

The objective is to avoid requiring advanced programming or engineering knowledge just to reproduce the modification.

Whenever possible, the documentation will include:

* Photos.
* Wiring diagrams.
* Component lists.
* Clear connection instructions.
* Measurements.
* Firmware ready to upload.
* Explanations of what each part does.
* Troubleshooting information.

## Repository structure

```text
.
├── README.md
├── BOM.md
├── CHANGELOG.md
├── docs/
├── hardware/
├── firmware/
└── mechanical/
```

More files will be added as the project develops.

## Project goal

The main objective is simple:

**Take an old, inexpensive hot-air station and give it some of the useful features found in modern stations without making the modification unnecessarily expensive or difficult to reproduce.**

## Safety warning

This project involves **mains voltage, high temperatures and heating elements**.

Incorrect modifications can cause electric shock, burns, fire or damage to equipment.

Do not reproduce the project unless you understand the electrical and thermal risks involved.

The project is currently experimental and is not a certified commercial design.

## License

A license has not yet been selected. It will be defined before the project reaches a stable public release.


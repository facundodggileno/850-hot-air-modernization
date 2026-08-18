# Safety

This project modifies equipment that combines **mains voltage, a high-power heater, moving air and very high temperatures**.

The project is currently experimental. These notes are not a substitute for electrical safety training, appliance certification or applicable local regulations.

## Main hazards

- Electric shock from mains-side circuitry.
- Fire caused by uncontrolled heater operation.
- Burns from the handpiece, nozzle or heated work area.
- Damage caused by insufficient airflow while the heater is powered.
- Incorrect assumptions about wiring differences between different 850-style stations.

## Design principles

The control system should be designed so that foreseeable faults tend toward a safe state.

Planned principles include:

- Heater shutdown when the handpiece is placed in its stand.
- Continued airflow during cooldown.
- Heater disable on invalid or implausible temperature readings.
- Explicit handling of controller startup and reset states.
- Independent consideration of failure modes in heater and pump control stages.
- No assumption that software alone provides adequate mains-side protection.

## Before reproducing the project

Verify the wiring, heater, pump, sensor and mains architecture of your own station. Do not copy connections solely because the enclosure or model number looks similar.

More detailed electrical protection requirements will be added once the final hardware architecture is validated.

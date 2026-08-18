# Firmware

The first firmware implementation is planned around an **Arduino Nano**.

The software will be structured so that hardware-specific I/O can be separated from the control logic, making a later port to an ESP32-class microcontroller easier.

## Planned responsibilities

- Temperature acquisition and filtering.
- Closed-loop heater control.
- Airflow / pump control.
- User input handling.
- Display handling.
- Handpiece stand detection.
- Automatic cooldown state machine.
- Fault detection and safe shutdown behavior.

Firmware source code and build instructions will be added once the first prototype is ready for reproducible testing.

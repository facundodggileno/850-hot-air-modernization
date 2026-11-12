# Firmware snapshots — LAB only

**None of these source files is an approved stable release.** These are exact development snapshots preserved to compare behavior. The AVR core/library versions and binary build hashes have not yet been locked.

| Snapshot | Path | Why it matters |
|---|---|---|
| V1.3.15 | [LAB Repromod SAFE / 3 s flow](lab/Hony850Pro_V1_3_15_LAB_REPROMOD_SAFE_FLOW3S.ino) | Current experimental snapshot; unresolved lack of airflow/false readings |
| V1.3.12 | [LAB diagnostic slim](lab/Hony850Pro_V1_3_12_LAB_DIAG_SLIM_NANO.ino) | Historical comparison after 1.3.11 overflowed Nano Flash |
| V1.2.6 | [Earlier Yihua heater baseline](reference/Hony850Pro_V1_2_6_YIHUA_SAFE_COOL68.ino) | Earlier controls, not automatically safer |
| Pump diagnostic V1 | [Isolated pump test](diagnostics/Hony850Pro_PUMP_ISOLATED_DIAG_V1.ino) | Alternates AIR100/70/40/100/40/100; heater AC must be physically disconnected |

## Intended environment

- Arduino Nano ATmega328P (5 V), updated watchdog-compatible bootloader as reported.
- Arduino AVR Boards package and Arduino IDE.
- hd44780 library (Bill Perry), using hd44780_I2Cexp.
- MAX6675 library providing max6675.h.
- Wire.h and avr/wdt.h included with AVR framework.
- Serial monitor 115200 baud for V1.3.15.

**Before flashing:** inspect all pin assignments, relay driver polarity, actual device and bootloader. The firmware has not been built or tested in this documentation session.

## Cautions

The software uses direct AVR Timer1 registers and interrupts; moving it to Mega2560 or ESP32-S3 is **not** a blind board selection. An idle display or 100% AIR command is not a real flow sensor reading. Code builds are not a substitute for validating physical mains cut-off.

For diagnosis, de-energize and physically isolate heater AC first; do not depend on D4/D5 being LOW as the only protection.

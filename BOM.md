# Bill of Materials — Hony 850 Pro (prototype, not a purchase-ready kit)

Updated 2026-11-12. Quantities reflect one reference prototype, not universally compatible 850 stations.

**Status meanings:** REPORTED-IN-USE = present according to project testing/wiring notes, but its exact electrical installation is not independently audited; REPORTED = measured/described; PLANNED = not installed or not confirmed. Verify datasheets, mains clearances, current rating and part markings before any reproduction.

| Qty | Part / reference | Status | Purpose / evidence / caveat |
|---:|---|---|---|
| 1 | Hony 850 station, enclosure and original air pump | REPORTED-IN-USE | Mechanical/pneumatic base; pump behavior at low phase-angle output is unresolved |
| 1 | Arduino Nano ATmega328P 5 V, updated bootloader | REPORTED-IN-USE | Main controller; 16 MHz, 32 KB Flash / 2 KB SRAM physical |
| 1 | Yihua 8786D-type ~650 W / 220 VAC heater assembly | REPORTED-IN-USE | Replacement after original ~250 W failed; measured ~74–85 ohms across project versions; actual hot-side temperature still uncertain |
| 1 | K-type thermocouple in handpiece | REPORTED-IN-USE | RAW temperature sensor, repositioned ~5 mm |
| 1 | MAX6675 thermocouple breakout | REPORTED-IN-USE | 5 V module power was temporarily disconnected, later restored |
| 1 | 16x2 I2C LCD PCF8574 / HW-61 class | REPORTED-IN-USE | hd44780_I2Cexp library |
| 2 | Mechanical rotary encoders + pushbuttons | REPORTED-IN-USE | Temperature and AIR control |
| 1 | Handpiece stand microswitch | REPORTED-IN-USE | Requests shutdown / cooldown |
| 1 | H11AA1 zero-cross detector | REPORTED-IN-USE | Connected to D2; excessive heating was observed |
| 1 | MOC3023 random-phase optotriac | REPORTED-IN-USE | Pump phase-angle gate |
| 1 | Pump TRIAC (BTA12 family per build notes) | REPORTED-IN-USE | Exact installed suffix and current rating need verification |
| 1 | MOC3083 zero-cross optotriac | REPORTED-IN-USE | Heater gate control |
| 1 | BTA12-600B | REPORTED-IN-USE | Replacement heater TRIAC |
| 1 | Omron-series safety relay | REPORTED-IN-USE (wiring unverified) | Output on D5; physical contact layout, coil voltage, weld detection and mains isolation must be checked |
| 1 | BC337 transistor relay driver | REPORTED-IN-USE | Coil drive; flyback protection and bias network must be verified |
| 1 | Local 5 V supply/regulator | REPORTED (exact model TBD) | Nano, MAX and LCD; avoid combining supply rails with USB without power-path design |
| — | Existing station transformer / mains wiring | REPORTED, NOT SPECIFIED | Do not assume other 850 stations have identical wiring |
| — | Mains resistors/snubber for zero-cross/TRIAC | REPORTED, values pending audit | Existing H11AA1 resistor arrangement was reported to run hot; no substitute design is approved |
| — | Aluminum tape / adapted hot-air head | REPORTED-IN-USE | Mechanical heater integration, thermal endurance requires validation |
| 1 | LTV-814 replacement zero-cross detector and resistors | PLANNED | Proposed to replace H11AA1 stage; **not installed/validated** |
| 1 | Independent airflow/pressure proving device | PLANNED — CRITICAL | No actual airflow feedback exists |
| 1 | Independent thermal cutoff / thermal fuse | PLANNED — CRITICAL | Must operate without MCU or sensor software |
| 1 | Power-stage feedback / contact monitoring | PLANNED — CRITICAL | Currently no verification that relay/triac physically opened |
| — | Dedicated PCB and final circuit schematic | DEFERRED | After hardware validation only |
| — | ESP32-S3 or Mega2560 | FUTURE OPTION | Not a current-build component |

## Known design constraints

- The code counts TRIAC firing commands; it does **not** measure real airflow or heater current.
- AIR percentages are controller settings, not calibrated volumetric flow.
- MAX6675 wiring includes copper transitions; thermocouple reference-junction errors and EMC have not been quantified.
- Heater, optotriac, PCB/stripboard layout, wiring insulation, mains fuse, earth connection and thermal cutoff need a physical safety audit.
- No supplier SKU or cost is asserted without confirmation. This is a **working BOM**, not a wiring diagram.

Related: [pinout](hardware/pinout.md), [safety](docs/safety.md), [known issues](docs/known-issues.md).

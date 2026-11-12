# Arduino Nano pin allocation — firmware V1.3.15

**Firmware pin definitions only. Not a circuit schematic, safe mains wiring plan or verified physical harness.**

| Nano pin | Direction | Firmware assignment | Notes |
|---|---|---|---|
| D2 | IN / interrupt | Zero-cross (H11AA1) | RISING transition expected |
| D3 | OUT | Pump TRIAC optocoupler MOC3023 | Continuous HIGH at commanded AIR100; Timer1 gate for partial AIR |
| D4 | OUT | Heater optocoupler MOC3083 | Commanded LOW for heat-off |
| D5 | OUT | Relay coil driver BC337 | Commanded LOW for heat-off |
| D6 | OUT | Buzzer/speaker | Optional user alert |
| D7 | IN | Stand microswitch | Active HIGH in firmware with external pull-up noted |
| D8 | IN | Temperature encoder CLK | |
| D9 | IN | Temperature encoder DT | |
| A3 | IN | Temperature encoder button | |
| A0 | IN | Air encoder CLK | |
| A1 | IN | Air encoder DT | |
| A2 | IN | Air encoder button | |
| D10 | OUT | MAX6675 CS | Software SPI |
| D12 | IN | MAX6675 SO | Software SPI |
| D13 | OUT | MAX6675 SCK | Software SPI |
| A4 | I2C | LCD SDA | HW-61 PCF8574-class backpack |
| A5 | I2C | LCD SCL | 16×2 display |

Power and grounds are intentionally **not depicted** as a schematic. Nano VCC nominal 5 V; MAX6675 module uses 5 V as tested. USB-connected PC and onboard power were suspected of interacting; a properly isolated serial adapter may be needed.

All outputs, especially D4/D5 OFF polarity and startup state, must be confirmed on actual electronics before claiming safety.

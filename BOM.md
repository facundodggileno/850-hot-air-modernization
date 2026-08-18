# Bill of Materials

> **Status:** work in progress. This BOM will be updated as the electrical design is validated.

## Core control hardware

| Item | Qty | Status | Notes |
|---|---:|---|---|
| Arduino Nano | 1 | Planned | Initial development controller |
| Handpiece stand sensor | 1 | Planned | Used to trigger heater shutdown and cooldown |
| Temperature sensing interface | 1 | TBD | Final implementation not yet selected |
| Heater power control stage | 1 | TBD | Must be suitable for the station heater and mains-side safety requirements |
| Pump control stage | 1 | TBD | Must match the pump used in the target station |
| User interface / display | 1 | TBD | Final display and controls to be selected |
| Low-voltage power supply | 1 | TBD | For controller and interface electronics |

## Notes

- Parts listed as **TBD** are intentionally not specified until the circuit has been measured and validated.
- Equivalent components may be documented where appropriate.
- Never assume that every 850-style station uses identical heater, pump or mains circuitry.

# Prototype hardware notes (no circuit diagram yet)

This directory deliberately contains **no schematic and no mains wiring instructions** at this stage. The hardware is still being changed, and the heater/pump power stages and independent safety interlocks have not been validated.

Available now: [Arduino Nano pin assignment as defined by the V1.3.15 firmware](pinout.md). It is a pin list, **not a verified as-built circuit**.

The prototype includes an adapted ~650 W heater, H11AA1 zero-cross stage, a separately switched pump via TRIAC and heater stage via MOC3083/BTA12, plus a relay command. Actual terminal connections, AC isolation, earth, fuse and suppression values require in-person verification before a diagram can be published.

Next step: physically validate BOM, wire routing, return currents, output polarity, independent overheat cutoff, pressure/airflow interlock and relay status feedback. Only then author schematics/PCB files.

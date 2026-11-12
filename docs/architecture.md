# Control architecture — current Arduino Nano prototype

## Objectives and hardware

The controller is an ATmega328P Nano (5 V / 16 MHz). Unlike an ESP32-S3, the Nano is single-core, has no FreeRTOS task isolation and limited SRAM. The present design combines UI, MAX6675 sampling, PID, zero-cross interrupts, Timer1 phase control, watchdog and output gating.

The prototype drives a Hony 850 pump plus adapted Yihua ~650 W heater. Reference firmware: [V1.3.15](../firmware/lab/Hony850Pro_V1_3_15_LAB_REPROMOD_SAFE_FLOW3S.ino).

## Processing paths

| Function | Implementation | Important limitation |
|---|---|---|
| Temperature acquisition | MAX6675 software SPI, nominal 250 ms polling | Repeated plausible readings do not establish genuine sensor accuracy |
| Temperature control | PID, Kp 1.994 / Ki 0.168 / Kd 1.707, anti-windup, predictive brake | Tuned on this particular thermal configuration; susceptible to sensor/airflow transients |
| Outlet estimation | RAW × 0.68; inverse scaling for UI setpoint | Empirical approximation only |
| Pump 100% | D3 driven continuously HIGH | Command is not proof of motion |
| Pump 20–95% | Zero-cross interrupt on D2; Timer1 phase-angle gate pulses on D3 | TRIAC gate counts are not air-flow or motor-current measurements |
| Heater | Zero-cross switching through D4 optotriac and D5 relay command | Physical opening of relay / off state of triac is unverified |
| Input/UI | Two encoders, buttons, stand switch, 16×2 I2C LCD | LCD/hardware issues can still interfere with software loop |
| Safety | Central permission check plus fault latch, AVR watchdog ~500 ms, zero-cross heartbeat ~300 ms | Depends on controller/ISR availability; not an independent safety circuit |

## Expected user-visible state transitions

1. On startup: heater stays OFF, perform initial tests; requires **manual HEAT request** afterward.
2. On HEAT request: full airflow preflow, then phase test for partial AIR, then settle; heater only becomes eligible after approximately **3 seconds**.
3. On AIR change while heat is requested: suspend heater, retain the request, repeat the ~3 second transition, resume automatically if conditions are satisfied.
4. On PAUSE/stand return: cancel heat request and start cooldown, ideally with full pump output until RAW cools below the configured threshold.
5. On FAULT: latch reason, command heater and relay OFF, request pump full power while cooling. A fault is not a normal pause.

**Observed deviation:** a real failure has sometimes left the pump stationary when the UI indicates full airflow/cooldown. A software state or signal is not independently measured physical flow.

## Key parameters in V1.3.15

- UI setpoint 80–300 °C (outlet estimate by default).
- AIR 0–100% setting; heat permission requires AIR ≥20%; 5% increments.
- MAX6675 poll and PID nominally every 250 ms; LCD refresh 150 ms; Serial summary every 500 ms.
- AIR change hold ~3000 ms: 1000 ms preflow, 1000 ms phase-gate verification where applicable, remainder settle.
- Cooldown stop 68 °C RAW; absolute cutoff 485 °C RAW.
- Heater duty soft cap on return and proportional cap at partial AIR are experimental guard rails, not verified safe thermal power limits.

## Diagnostic nomenclature

Logs may contain 'airSet' and 'airActual'. **Neither is a flow measurement** in the current hardware. Interpret 'airActual' as *commanded pump control mode*. For future firmware, rename to 'airCommanded' and introduce separate 'flowVerified' only after sensor integration.

## Future partitioning

Keep the Nano until the control is characterized, then abstract:
- MCU-independent state machine, thermal controller and fault logic.
- HAL for ZERO-CROSS, TRIAC gate, heater relay, temperature acquisition and I2C.
- Diagnostics with monotonic timestamps and fault snapshots.

Mega2560 would supply memory/timers; ESP32-S3 would add multi-core/FreeRTOS, but neither fixes a physically stalled pump without independent feedback.

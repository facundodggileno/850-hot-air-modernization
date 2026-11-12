# Safety model — experimental firmware, NOT a verified safety system

**Critical warning:** 220 VAC, high-power heating (~650 W) and a pump capable of losing effective airflow. Software cutoff and counters do **not** replace mains-rated physical disconnection, a thermal fuse, competent wiring inspection, earthing, insulation, and independent flow proof. The current prototype has demonstrated risky conditions; never operate it unattended.

## Fault handling in V1.3.15

Faults are latched; 'setFault' requests heater control and relay outputs LOW, forces pump command to 100% and logs a snapshot (RAW temperature, commanded heater duty, selected AIR, arm stage and uptime).

| Code | Constant | Intent and threshold / detection |
|---:|---|---|
| 1 | FAULT_TC | Invalid/lost MAX6675 reading; timeout during heat request about 750 ms |
| 2 | FAULT_OVERTEMP | RAW temperature ≥485 °C |
| 3 | FAULT_RUNAWAY | RAW exceeds RAW setpoint by 40 °C for ~750 ms once armed |
| 4 | FAULT_NO_RISE | Heat ≥25%, target error ≥20 °C and rise <3 °C in 8 s |
| 5 | FAULT_CONTROL_LOST | After 10 s within ±10 °C, error ≥40 °C for 10 s |
| 6 | FAULT_WATCHDOG | AVR reset history indicates watchdog reset (bootloader-dependent) |
| 7 | FAULT_ZEROCROSS | Zero-cross input absent/stale during requested operation (about 150 ms) |
| 8 | FAULT_STALL | Main-loop heartbeat stale >300 ms as detected by zero-cross ISR |
| 9 | FAULT_LCD | LCD initialization / recovery fault |
| 10 | FAULT_SELFTEST | Startup test failure |
| 11 | FAULT_PUMP_GATE | Missing/mismatched Timer1 pump TRIAC firing count |
| 12 | FAULT_RESET_LOOP | Repeated resets during startup tests |
| 13 | FAULT_ESTOP | TEMP + AIR buttons held simultaneously ~120 ms |
| 14 | FAULT_TC_SPIKE | Anomalous upward RAW step >42 °C per acquisition while supervised |

**Note:** Some flags are conditional on state and time window; thresholds describe the present code and **must not** be treated as validated safe operating limits.

## Non-latching controls

- Stand sensor: immediate heater command off at active level, 35 ms debounce for confirmed resting state.
- Cooldown: request full pump after stopping heater; stop pump after cooling below ~68 °C RAW.
- Air-change transition: heater command disabled for about 3 s, then auto-resume if HEAT request remains and checks pass.
- Thermal drop guard: sudden downward RAW sample >22 °C triggers a short temporary heater hold (~750 ms); PID uses a 3-sample median and a slowly decaying thermal reference.
- PID anti-windup, predictive braking, warm-restart power cap.
- Minimum heat AIR setting 20% — **not equivalent to proof of sufficient flow**.
- WDT hardware ~500 ms resets Nano; zero-cross ISR attempts output cut on stale main-loop heartbeat ~300 ms.

## What is NOT protected/verified

1. **Real airflow:** zero-cross and timer gate counts can be correct while the pump is stationary. The 'airActual' field is not a sensor.
2. **Relay welded or heater TRIAC shorted:** LOW at D4/D5 is an instruction, not feedback that mains power stopped.
3. **Sensor providing a plausible but false reading:** three samples and fresh timestamps cannot certify thermal correctness.
4. **Hardware start/reset outputs:** output-safe state depends on electrical pull-downs, driver design, boot pin states and relay wiring, not merely initial firmware instructions.
5. **Power path conflict / EMI:** Nano 5 V supply, USB 5 V and ground paths were suspected; no isolation or power-switch design is validated.
6. **Watchdog coverage:** watchdog responds to certain software stalls, but not valid-looking yet dangerous decisions or loss of airflow.
7. **Independent thermal cutoff / independent flow interlock:** not installed and verified.

## Required acceptance tests before heater-enabled validation

- Physically disconnect heater AC and confirm **relay and triac output behavior** across startup, restart, stand, pause, AIR changes and all faults.
- Validate measured pump airflow/pressure over intended control range, especially AIR 25–40%; verify restart at low phase settings.
- Characterize actual heater supply cut-off using appropriate isolated measurement techniques.
- Add independent thermal protection and flow interlock that remove heat **even if the MCU, GPIO or software fails**.
- Use a proper isolated UART/USB link if observing the live controller with mains powered.

No firmware release should be marked SAFE / STABLE until those tests pass. This documentation does not provide a circuit schematic or mains wiring instructions.

# Engineering decisions and deferred changes

**2026-11-12 — Remain on Nano for now.** Mega2560 offers more timers and memory; ESP32-S3 offers more memory/compute and two cores. Neither fixes an unmeasured stopped pump. Keep the current Nano build as the baseline until state transitions, actual airflow and physical cutoff are understood.

**2026-11-08 — Full heater heat capacity.** Original ~250 W element failed and was replaced with a Yihua ~650 W heater. This considerably increases the importance of air path and power disconnection validation.

**2026-11-08 — MAX6675 VCC restored.** After discovering that the 5 V lead was disconnected, the ~0.68 empirical thermal relationship persisted. Do not conclude it is an intrinsic thermocouple calibration error.

**2026-11-08 — Transition pause.** User requested ~3 s heater-off when AIR changes, then automatic resume unless manually paused; airflow settings 20–100% must remain available for diagnosis. Implemented in V1.3.15, still under test.

**2026-11-08 — Repromod SAFE principles.** Adopt priority off, latched faults, deliberate reset and fault snapshot concept; Nano cannot reproduce independent second-core safety supervision. Never represent it as certified safety.

**2026-11-12 — No schematic yet.** Only pin assignments and BOM can be documented reliably. Do not publish circuit diagram until the hardware is inspected, the safety layers are implemented and circuit values validated.

**License:** not yet selected. Avoid asserting a specific license without maintainer approval.

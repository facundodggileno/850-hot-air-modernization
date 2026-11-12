# Hony 850 Pro — Modernización de estación de aire caliente 850

Proyecto de desarrollo experimental para modernizar estaciones tipo Hony 850, con **Arduino Nano primero** y una posible migración a ESP32-S3 una vez estabilizado el diseño.

**Estado: LAB / NO APTO PARA USO GENERAL (12-11-2026).** La última rama de pruebas es V1.3.15. **No es una versión segura ni estable**: se observaron anomalías de lectura del MAX6675, picos de temperatura y ocasiones en las que el software indicaba AIR 100 % pero la bomba no entregaba aire físicamente.

## Prototipo actual

- Base Hony 850, bomba original.
- Heater original de ~250 W averiado, sustituido por elemento Yihua ~650 W para 220 VAC.
- Arduino Nano ATmega328P 5 V, bootloader actualizado.
- Termocupla K y MAX6675; alimentación de 5 V reconectada; termocupla desplazada ~5 mm al flujo.
- LCD I²C 16x2, dos encoders, pulsadores y sensor del soporte.
- Bomba con MOC3023 + TRIAC y control por fase mediante Timer1.
- Heater con MOC3083 + BTA12-600B; relé accionado mediante BC337.
- Detector de cruce por cero H11AA1. Su calentamiento anormal está pendiente de solución.

## Qué funciona en código

Control PID, ajuste de caudal, cooldown, retorno a calefacción desde cooldown, pausa aproximada de 3 segundos al cambiar AIR, watchdog AVR 500 ms, verificación inicial de MAX y zero-cross, prueba sintética de 12.288 casos, contadores de disparo Timer1 y fallos enclavados inspirados en Repromod SAFE.

**Importante:** los contadores de disparo y el porcentaje mostrado no miden caudal real. Tampoco se confirma con ellos que el relé haya cortado físicamente los 220 V.

## Mediciones y configuración experimental

- PID histórico: **Kp 1,994; Ki 0,168; Kd 1,707**. Es provisional.
- Relación empírica: **T salida estimada ≈ T RAW del MAX × 0,68**. No es una calibración universal ni garantía de la temperatura real del heater.
- Lecturas RAW superiores a 500 °C aparecieron en pruebas; no repetir estas condiciones sin barreras independientes.

## Documentación del proyecto

- [BOM y estado de los componentes](BOM.md)
- [Arquitectura del control](docs/architecture.md)
- [Protecciones y sus límites](docs/safety.md)
- [Bugs y riesgos abiertos](docs/known-issues.md)
- [Pruebas y registros](docs/tests-and-results.md)
- [Calibración térmica](docs/thermal-calibration.md)
- [Pines del Nano (no es diagrama de circuito)](hardware/pinout.md)
- [Hoja de ruta](docs/roadmap.md)
- [Versiones de firmware](firmware/README.md)

**Aún no se publica diagrama de circuito:** se agregará cuando el hardware, protecciones y cableado estén comprobados.

**Advertencia:** intervención sobre red eléctrica de 220 V y heater de alta potencia. No es una modificación certificada. Desconectar físicamente la alimentación del heater para las pruebas de control. El software por sí solo no alcanza como protección.

Mantenedor: Facundo D. Gileno / PC Service Computación.

# Hony 850 Pro ESP32-S3 V3.4.2 — Primera release estable ESP32

**Tag previsto:** `esp32-v3.4.2` · **Fecha:** 2026-11-22 · **Estado del proyecto:** versión de referencia estable designada por el autor.

Es la primera publicación de la línea ESP32-S3 en el repositorio. La serie Arduino Nano V2.x permanece disponible como historia técnica y no comparte pinout con esta versión.

## Incluye

- Firmware modular Arduino-ESP32 3.x con 25 archivos en `firmware/esp32/Hony850Pro_ESP32S3_v3_4_2/`.
- Pinout estable **exactamente derivado de `Config.h`**: [diagrama SVG](../../hardware/esp32-s3-wiring-v3.4.2.svg), [tabla CSV](../../hardware/esp32-s3-pinout-stable.csv) y [detalle técnico](../../hardware/esp32-s3-pinout-stable.md).
- Temperatura y AIR con tres canales persistentes en NVS; cambio de canal con AIR corto, acceso al menú con AIR largo al soltar y guardado manteniendo 10 s.
- TEMP corto alterna HEATER ON/OFF; al apagar conserva aire para trabajar sin calor. Otro TEMP corto inicia calor tras preflujo. Apoyar y levantar el mango borra el apagado manual y rearma el arranque automático.
- Stand, protecciones térmicas, detección de termocupla, cruce por cero, watchdog, cooldown, preflujo, PID, autotune y FAST HEAT.
- LCD1602 PCF8574T write-only (RW a GND) con actualización diferencial cada 150 ms; MAX6675 a intervalos de conversión de al menos 250 ms y PID sincronizado con nuevas muestras; buzzer al alcanzar consigna.
- Control MOC3023 de bomba y MOC3083 del calefactor con relé, habilitados por defecto en esta configuración de usuario (`HONY_ENABLE_MAINS_OUTPUTS=1`, `HONY_ENABLE_PUMP_OUTPUT=1`).

## Límites del firmware

| Parámetro | Valor |
|---|---:|
| Consigna máxima de salida estimada | 480 °C |
| Corte termocupla interna RAW | 750 °C |
| Lectura válida MAX6675 | hasta 800 °C |
| Margen para autorizar consigna RAW | 35 °C |
| AIR mínimo para calefacción | 20 % |

Se informó una **prueba de 480 °C reales de salida, estable, con medición externa**. La calibración RAW→salida del código tiene puntos medidos principalmente hasta 300 °C y extrapola por encima de ese rango; no se presentan esa curva ni el ensayo comunicado como calibración universal o certificación.

## Instalación

1. Abrir `firmware/esp32/Hony850Pro_ESP32S3_v3_4_2/Hony850Pro_ESP32S3_v3_4_2.ino` con todos los `.h` y `.cpp` de la misma carpeta.
2. Utilizar **ESP32-S3** y el core **Arduino-ESP32 3.x**.
3. Instalar `hd44780` (biblioteca con `hd44780_I2Cexp`) y una implementación compatible de `max6675.h`. `Wire`, `Preferences` y las APIs del ESP-IDF forman parte del entorno ESP32.
4. Seguir el [pinout ESP32-S3](../../hardware/esp32-s3-pinout-stable.md). La documentación Nano V2.0.5 **no** debe usarse como lista de cableado de esta versión.
5. Validar en banco con control de potencia y protección física conforme al montaje; no deducir el comportamiento real de relé, triac, flujo de aire o aislamiento solamente del firmware.

**Alcance de validación:** la versión se declara estable como baseline por decisión del autor, con verificaciones eléctricas/físicas reportadas. En este flujo de publicación se revisan archivos y lógica de modo estático; **no se ha demostrado una compilación con todas las bibliotecas ni un ensayo físico independiente de esta revisión**.

**Decisión permanente de alcance:** no hay sensor de caudal, flujo ni paso de aire. AIR % es mando de bomba, no una medición de flujo.

**Autores y equipo:** [ver CONTRIBUTORS.md](../../CONTRIBUTORS.md).

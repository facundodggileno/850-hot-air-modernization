# Registro de versiones

## 2026-11-22 — ESP32-S3 V3.4.2 (primera release estable de ESP32)

- Versión de referencia estable seleccionada por el autor. [Notas completas](docs/releases/esp32-v3.4.2.md).
- Firmware completo de 25 archivos en [firmware/esp32](firmware/esp32/Hony850Pro_ESP32S3_v3_4_2); etapa histórica Nano conservada.
- [Diagrama SVG exacto de los GPIO](hardware/esp32-s3-wiring-v3.4.2.svg) y [CSV](hardware/esp32-s3-pinout-stable.csv) derivados de `Config.h`. Los GPIO de la primera ilustración se corrigieron.
- TEMP corto alterna calefactor y modo solo aire sin apagar bomba; apoyar y levantar mango invalida OFF manual. Se puede reencender sin esperar enfriamiento completo, con preflujo.
- Tres canales NVS, AIR corto/largo/10 s, FAST HEAT, LCD optimizado, MAX6675 y PID sincronizado, buzzer de consigna y protecciones.
- Potencia y bomba habilitadas por defecto; consigna 480 °C, RAW 750 °C, rango MAX hasta 800 °C y margen 35 °C. Sin sensores de caudal.
- Ensayo externo 480 °C estable reportado; curva RAW/salida de alta temperatura aún extrapolada.

El historial **no** significa que todas las versiones pasaron el mismo nivel de ensayos.

## 2026-11-19 — V2.0.5 (último código publicado, pendiente de validación física integral)
- Firmware modular dividido en 25 archivos dentro de `firmware/current`.
- Menú PID: mantener AIR; pulsación corta AIR = atrás, TEMP = confirmar.
- Consulta de Kp, Ki y Kd; autotune de 3 ciclos, con autorización explícita y limitada para realizarlo con mango apoyado en stand.
- Cooldown al llegar a 100 °C RAW y activación automática con histéresis desde 110 °C RAW.
- Publicadas las 12 hojas del diagrama, galería de fotografías del montaje y documentos académicos.
- Eliminadas las propuestas de sensores de caudal; decisión definitiva de no instalarlos.

## 2026-11-19 — V2.0.2 (ensayo físico reportado en modo normal)
- Se solucionaron errores de inclusión de Wire y símbolos AVR `__heap_start`/`__brkval`.
- Se reportó funcionamiento del calentamiento, controles y cambio a caudal menor sin reproducir fallas anteriores.

## 2026-11-19 — V2.0.0 a V2.0.1
- Migración de sketch monolítico a módulos: control de hardware, sensor, pantalla, entradas, PID, EEPROM, seguridad y orquestador.

## 2026-11-18 a 19 — V1.4.x
- Versiones monolíticas previas con autotune, arranque, watchdog y ajustes. Consultar el registro de la carpeta de campo.

## 2026-11-08 a 12 — V1.2.x / V1.3.x
- Prototipo, pruebas térmicas, diagnósticos de bomba, problemas de MAX6675 y ajustes de protección.
- Los logs y sketches históricos se conservan en el repositorio para comparación; **no** son firmwares estables alternativos.

# Decisiones de diseño

- **2026-11-22 — ESP32-S3 versión vigente:** se designa la V3.4.2 como primera versión de referencia estable ESP32, con su pinout [verificado contra `Config.h`](../hardware/esp32-s3-pinout-stable.md) y 25 archivos. La etapa Nano es histórica.
- **2026-11-22 — mando de calefactor:** TEMP corto alterna HEATER ON/OFF, manteniendo aire; la orden manual OFF no se invalida girando los encoders ni cambiando canal, solo con otro TEMP corto o nuevo ciclo apoyar/levantar el stand.
- **2026-11-22 — release y límites:** salidas habilitadas por defecto por decisión y pruebas reportadas por el autor, consigna de salida 480 °C, RAW corte 750 °C, validez MAX6675 hasta 800 °C y margen interno 35 °C. La designación estable no implica certificación independiente.
- **2026-11-22 — alcance:** no instalar ni proponer un sensor de caudal o paso de aire. AIR % no mide flujo real.

### Decisiones históricas del Arduino Nano

- **2026-11-19 — arquitectura vigente:** Arduino Nano ATmega328P y firmware modular V2.0.5 como base actual; preservar historial monolítico sin sustituir la revisión actual por la V1.3.x.
- **2026-11-19 — sensores de aire:** decisión definitiva de **no instalar ni proponer sensores de caudal, flujo o paso de aire**. El porcentaje AIR es una orden. No confundir con realimentación.
- **2026-11-19 — presentación/documentación:** diagrama V2.0.5 con conexiones y niveles de confianza; carpeta de campo y slides en primera persona, fotos reales de la estación y del primer/final calefactor.
- **2026-11-19 — versión verificada:** V2.0.2 ensayada físicamente en modo normal, V2.0.5 simulada y pendiente de validación física.
- **2026-11-19 — interfaz:** AIR corto vuelve y TEMP corto confirma en menú PID; AIR largo abre menú, autotune de tres ciclos con posibilidad de stand sólo bajo excepción temporal.
- **2026-11-19 — cooldown:** final a 100 °C RAW; reactivación a 110 °C RAW. Las temperaturas RAW no son equivalentes a salida de boquilla.
- **2026-11-12 — herencia de fallos:** registrar fallos históricos de bomba/termopar/triac, sin asegurar que un bug antiguo persiste en V2.0.5 sin prueba.

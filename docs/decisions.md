# Decisiones de diseño

- **2026-11-19 — arquitectura vigente:** Arduino Nano ATmega328P y firmware modular V2.0.5 como base actual; preservar historial monolítico sin sustituir la revisión actual por la V1.3.x.
- **2026-11-19 — sensores de aire:** decisión definitiva de **no instalar ni proponer sensores de caudal, flujo o paso de aire**. El porcentaje AIR es una orden. No confundir con realimentación.
- **2026-11-19 — presentación/documentación:** diagrama V2.0.5 con conexiones y niveles de confianza; carpeta de campo y slides en primera persona, fotos reales de la estación y del primer/final calefactor.
- **2026-11-19 — versión verificada:** V2.0.2 ensayada físicamente en modo normal, V2.0.5 simulada y pendiente de validación física.
- **2026-11-19 — interfaz:** AIR corto vuelve y TEMP corto confirma en menú PID; AIR largo abre menú, autotune de tres ciclos con posibilidad de stand sólo bajo excepción temporal.
- **2026-11-19 — cooldown:** final a 100 °C RAW; reactivación a 110 °C RAW. Las temperaturas RAW no son equivalentes a salida de boquilla.
- **2026-11-12 — herencia de fallos:** registrar fallos históricos de bomba/termopar/triac, sin asegurar que un bug antiguo persiste en V2.0.5 sin prueba.

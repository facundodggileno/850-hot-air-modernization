# Incidencias y limitaciones documentadas

| Prioridad | Asunto | Estado y tratamiento |
|---|---|---|
| P0 | Corte de calefactor físico no comprobado en todos los fallos | Inspeccionar relé, triac y protección térmica independiente |
| P0 | Calentamiento de resistencias del H11AA1 | Revalidar valores y disipación, verificar la placa real |
| P0 | V2.0.5 autotune en stand | Prueba física de entradas, salidas, revocación de permiso y cooldown pendiente |
| P1 | Lecturas MAX6675 inconsistentes en pruebas anteriores | Inspección de cableado/ruido, masa y termopar; medir contra sonda externa |
| P1 | AIR% vs movimiento real de bomba | El porcentaje es mando. El diseño **no lleva ni llevará sensor de flujo**; ensayar bomba por otros métodos de taller |
| P1 | Calibración temperatura | Factor 0,68 aproximado; depende del punto y metodología de medición |
| P1 | Flash/SRAM con librerías reales | Compilar con Arduino AVR core real y documentar memoria antes de aprobación |
| P2 | Conexiones AC y valores de resistencia incompletos en el PDF | Inspeccionar físicamente antes de declarar esquema eléctrico de producción |

Logs y diagnósticos históricos se conservan como evidencia en `tests/` y `firmware/lab/`. Una versión nueva no implica que todos los defectos históricos continúen reproducibles.

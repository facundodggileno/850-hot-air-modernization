# Resultados y alcance de validación

### Niveles de evidencia

| Versión | Evidencia disponible | Qué NO demuestra |
|---|---|---|
| V1.2.x / V1.3.x | Logs históricos y pruebas de taller con incidencias | Estabilidad y seguridad de V2.0.5 |
| V1.4.x | Evolución de interfaz, autotune y protecciones | Certificación eléctrica |
| **V2.0.2** | Usuario confirmó funcionamiento físico normal (calor, controles y ajuste a menor AIR) | Cobertura de todos los fallos o seguridad independiente |
| **V2.0.5** | Sintaxis C++11 / stubs, simulación de transiciones de menú, autotune y cooldown documentadas | **Compilación real Arduino IDE, uso prolongado y comportamiento de 220 V** |

### Ensayos térmicos de referencia (desarrollo histórico)

Mediciones con termocupla externa: consigna 300 °C a AIR 100/40 % ≈196 °C en salida; 350 °C a AIR 100/40 % ≈239/235 °C; 380 °C a AIR 100/40 % ≈253–256/248 °C; 420 °C AIR 100 % ≈285 °C. Estos datos corresponden a pruebas previas y **no** validan automáticamente la versión V2.0.5 ni la consigna corregida actual.

Factor experimental posterior `T_corr ≈ 0,68 × T_RAW`, ganancias de referencia `Kp=1,994, Ki=0,168, Kd=1,707`. Véase [calibración](thermal-calibration.md).

### Comprobaciones pendientes

Compilación AVR real / memoria, tiempo y maniobras en la estación real, mediciones externas, corte del relé bajo fallo, integridad de alimentaciones y temperaturas de componentes. Los números de timer y la indicación AIR% representan **comandos**, no una medición de aire. El proyecto no contempla sensores de caudal.

[Historial](../CHANGELOG.md) · [Checklist](COMMISSIONING_ES.md) · [Validación incluida en el código V2.0.5](../firmware/current/Hony850Pro_Nano_Modular_v2_0_5/VALIDACION.md)

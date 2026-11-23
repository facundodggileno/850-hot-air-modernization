# Hony 850 Pro — Firmware ESP32-S3 estable V3.4.2

**Esta es la línea ESP32 activa del proyecto.** Abrir el archivo [`Hony850Pro_ESP32S3_v3_4_2.ino`](Hony850Pro_ESP32S3_v3_4_2/Hony850Pro_ESP32S3_v3_4_2.ino) manteniendo juntos los 25 archivos de esa carpeta.

- Placa: ESP32-S3, core **Arduino-ESP32 3.x**.
- Dependencias: `hd44780` de Bill Perry (I2Cexp), `max6675.h` compatible. `Wire`, `Preferences` y GPTimer se incluyen con el core/ESP-IDF.
- [Pinout exacto (GPIO)](../../hardware/esp32-s3-pinout-stable.md) · [Diagrama SVG](../../hardware/esp32-s3-wiring-v3.4.2.svg) · [Notas de primera release](../../docs/releases/esp32-v3.4.2.md).
- Hardware de 3,3 V a nivel lógico, LCD1602 con PCF8574T modificado y RW a GND. Sensor MAX6675 en GPIO16/17/18 y stand GPIO9 con entrada `INPUT`, sin pull-up interno.
- `HONY_ENABLE_MAINS_OUTPUTS=1` y `HONY_ENABLE_PUMP_OUTPUT=1`. El firmware activa salidas; **no constituye una homologación eléctrica**.

### Uso rápido

| Control | Acción |
|---|---|
| Girar TEMP | Cambiar consigna |
| TEMP corto en pantalla principal | Alternar calefactor ON/OFF; aire permanece activo |
| TEMP largo en pantalla principal con calefactor apagado | Consultar PID |
| Girar AIR | Cambiar porcentaje de mando de bomba |
| AIR corto | Cambiar CH1/CH2/CH3 |
| AIR mantener de 1,2 s a menos de 10 s y soltar | Abrir menú |
| AIR mantener 10 s | Guardar consigna y AIR en el canal actual |
| Apoyar mango | Cortar calor y enfriar |
| Levantar mango tras haber estado apoyado | Reanudar calor automáticamente, cancelando apagado manual |
| Dos botones mantenidos | E-STOP por software |

El TEMP corto puede volver a iniciar calor aunque el mango todavía esté caliente, sujeto al preflujo de 3 s y al resto de interlocks. Si se apaga calor manualmente y se gira TEMP/AIR o cambia CH, **no se enciende solo**: debe tocar TEMP o completar el ciclo de stand.

Los tres canales se guardan en NVS; el primer inicio busca datos de la estructura anterior de cinco canales y migra CH1–CH3 si es posible.

### Seguridad/calibración

Consigna máxima 480 °C estimados; RAW interno máximo 750 °C; MAX6675 válido hasta 800 °C; margen interno 35 °C. Con la curva existente, la estimación de salida a alta temperatura **no reemplaza** una medición externa. El corte físico/eléctrico independiente y aislamiento requieren comprobación.

No existe ni se proyecta sensor de caudal o paso de aire.

# Calibración térmica y PID

La termocupla mide la zona donde fue montada, no directamente la temperatura exacta del aire en la boquilla. La modificación del mango incorporó un calefactor Yihua 8786D y reubicó el punto de medida aproximadamente 5 mm. Una conexión/cableado inadecuado de la termocupla puede causar diferencias significativas.

## Conversión observacional

`Temperatura corregida ≈ 0,68 × Temperatura RAW MAX6675`. El PID calcula en RAW usando consigna/0,68. **La protección absoluta utiliza el límite RAW de 595 °C y no debe multiplicarse por 0,68.**

| Consigna histórica (°C) | AIR ordenado | Medición externa reportada (°C) |
|---:|---:|---:|
| 300 | 100 % | ~196 |
| 300 | 40 % | ~196 |
| 350 | 100 % | ~239 |
| 350 | 40 % | ~235 |
| 380 | 100 % | ~253–256 |
| 380 | 40 % | ~248 |
| 420 | 100 % | ~285 |
| 440 | 100 % | ~300 (sobreimpulso reportado) |

Datos de una fase de desarrollo previa; no son una tabla de certificación. En la UI V2.0.5 el máximo programado es 360 °C corregidos.

## PID de partida

`Kp = 1,994`, `Ki = 0,168`, `Kd = 1,707` como valores de referencia; un ensayo reportó pico de ~328 °C al solicitar 300 °C en una configuración anterior. Se debe repetir autotune y validación térmica después de cada cambio físico relevante.

No se emplea ningún sensor de caudal, ni se planifica agregarlo. Los porcentajes AIR describen el mando aplicado a la bomba.

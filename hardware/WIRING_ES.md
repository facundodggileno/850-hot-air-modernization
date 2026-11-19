# Hardware Hony 850 Pro — Conexiones y alcance de verificación

**Referencia completa:** [PDF de 12 hojas V2.0.5](Hony850Pro_Esquematico_Conexiones_Completas_V2_0_5.pdf). **Primero leer seguridad:** [docs/safety.md](../docs/safety.md).

## Bloques usados

| Bloque | Señal/función | Evidencia |
|---|---|---|
| Nano ATmega328P | Control a 5 V, D2–D13, A0–A5 | Definición exacta en `Config.h` |
| MAX6675 + K | SO/CS/SCK, termocupla adaptada | Firmware y montaje reportado |
| LCD 16×2 I²C | A4/A5, UI AIR/TEMP | Firmware y fotografías |
| H11AA1 | ZC en D2 | Etapa reportada; resistores de lado de red y temperatura bajo revisión |
| MOC3023 / TRIAC bomba | D3, control por fase de la bomba original | Firmware y hardware reportado; red gate pendiente |
| MOC3083 / BTA12-600B | D4, control calefactor | Hardware reportado; valores de resistencias y snubber pendientes |
| Relé 5 V + BC337 | D5, corte de autorización del calefactor | Control definido; borneras/contactos concretos a comprobar |
| Stand / buzzer | D7 / D6 | Firmware y prototipo reportado |

### Asignación completa de pines

| Pin Nano | Función |
|---|---|
| D2 | H11AA1 cruce por cero |
| D3 | LED de MOC3023 — bomba |
| D4 | LED de MOC3083 — calefactor |
| D5 | BC337 — relé calefactor |
| D6 | Alarma buzzer |
| D7 | Stand, HIGH mango apoyado (requiere polarización hardware adecuada) |
| D8/D9/A3 | TEMP CLK / DT / switch |
| A0/A1/A2 | AIR CLK / DT / switch |
| D10/D12/D13 | MAX6675 CS / SO / SCK |
| A4/A5 | LCD SDA / SCL |

## Partes de 220 V: datos incompletos

Los siguientes datos **no son suficientemente firmes como para publicar un esquema de cableado de red prescriptivo**: valores de resistencias para los LEDs de los optotriacs, red de puerta TRIAC, snubbers, mapeo de relé y borneras, protección térmica, fusible y puesta a tierra. Corresponde inspeccionar y medir el prototipo; no inventar valores por analogía con otra placa. El PDF señala estas limitaciones.

Se reportaron **dos resistencias de 12 kΩ / 2 W** en una iteración del detector H11AA1 con temperatura elevada. No debe asumirse que sean el valor final. El calefactor original tenía aprox. 85 Ω, sustituido por Yihua adaptado de 74,9 Ω según medición posterior; se documentan distintas mediciones históricas, no una ficha de fábrica.

El comando de la bomba (`AIR`) no mide su movimiento físico. No hay ni se proyecta sensor de caudal ni de flujo: esta es una **decisión de alcance**. En ningún caso el software puede reemplazar la revisión del circuito de potencia.

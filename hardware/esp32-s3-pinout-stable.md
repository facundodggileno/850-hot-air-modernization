# Pinout estable Hony 850 Pro — ESP32-S3 V3.4.2

**Estado:** esta asignación de GPIO es la referencia del firmware ESP32-S3 V3.4.2, declarado estable para el proyecto el 22/11/2026. La fuente de verdad es [`Config.h`](../firmware/esp32/Hony850Pro_ESP32S3_v3_4_2/Config.h). No mezclar con el pinout histórico del Arduino Nano.

**Diagrama vectorial:** [mapa de conexiones ESP32-S3](esp32-s3-wiring-v3.4.2.svg) · **Datos importables:** [CSV](esp32-s3-pinout-stable.csv). El dibujo muestra **redes lógicas y bloques funcionales**, no pistas de PCB ni un cableado certificado de 220 VAC.

| GPIO | Símbolo de `Config.h` | Destino y señal |
|---:|---|---|
| 1 | `LCD_SDA` | LCD1602 / PCF8574T: SDA I2C; lógica 3,3 V |
| 2 | `LCD_SCL` | LCD1602 / PCF8574T: SCL I2C; lógica 3,3 V |
| 4 | `ZC` | H11AA1: Entrada cruce por cero; lógica 3,3 V |
| 5 | `PUMP` | MOC3023 / TRIAC bomba: Salida control por fase |
| 6 | `HEATER` | MOC3083 / TRIAC calefactor: Salida control por ciclos completos |
| 7 | `RELAY` | BC337 / relé calefactor: Salida lógica transistor |
| 8 | `BUZZER` | Buzzer / driver: Salida GPIO; no carga directa 5 V |
| 9 | `STAND` | Microswitch soporte: INPUT; HIGH=apoyado; polarización externa |
| 10 | `TEMP_CLK` | Encoder TEMP: CLK / INPUT_PULLUP |
| 11 | `TEMP_DT` | Encoder TEMP: DT / INPUT_PULLUP |
| 12 | `TEMP_SW` | Encoder TEMP: SW / INPUT_PULLUP |
| 13 | `AIR_CLK` | Encoder AIR: CLK / INPUT_PULLUP |
| 14 | `AIR_DT` | Encoder AIR: DT / INPUT_PULLUP |
| 15 | `AIR_SW` | Encoder AIR: SW / INPUT_PULLUP |
| 16 | `MAX_CS` | MAX6675: CS / salida |
| 17 | `MAX_SO` | MAX6675: SO / entrada |
| 18 | `MAX_SCK` | MAX6675: SCK / salida |
| 21 | `TC_EXTERNAL_CS` | Reservado para un segundo termómetro; no activado en esta release |
| 39 | `TC_PREHEATER_CS` | Reservado; no activado en esta release |

## Reglas de interfaz eléctrica

- **ESP32-S3: GPIO a 3,3 V.** No aplicar 5 V directamente a las entradas del ESP32.
- **LCD:** `Wire.begin(1, 2, 100000)`; expansor PCF8574T ajustado a lógica I²C de 3,3 V. LCD1602 con VDD/backlight de acuerdo con el módulo montado. El pin R/W del LCD está cableado a GND (modo escritura).
- **MAX6675:** CS/SO/SCK por GPIO16/17/18 y alimentación/niveles compatibles con la lógica del ESP32.
- **Encoders TEMP y AIR:** `INPUT_PULLUP`, pulsadores activos a LOW.
- **Stand:** `pinMode(STAND, INPUT)`, **sin `INPUT_PULLUP` interno**. Se espera HIGH = mango apoyado. Requiere la polarización física adecuada; no conectar una línea flotante.
- **ZC / H11AA1:** GPIO4 recibe la salida acondicionada y aislada del detector, jamás una conexión directa a CA.
- **Etapas de potencia:** GPIO5 manda la etapa de bomba MOC3023; GPIO6 manda calefactor por MOC3083; GPIO7 manda el transistor del relé. Los GPIO son señales lógicas, no alimentación de carga.
- **Buzzer:** GPIO8 controla la alarma mediante driver apropiado.
- **GPIO19/20:** no asignados a los periféricos de control; en muchos ESP32-S3 se destinan al USB nativo.
- **GND lógico común** solamente en la parte de baja tensión correspondiente. Nunca usar GND lógico como neutro ni como tierra de seguridad de red.

## Parámetros de la release estable

- Tres canales NVS, TEMP corto = calefactor ON/OFF manteniendo aire; un ciclo completo de stand rearma el calor automático.
- AIR corto = cambiar canal; AIR largo entre 1,2 y 10 s al soltar = menú; AIR mantenido 10 s = guardar canal.
- FAST HEAT, PID, MAX6675, watchdog, preflujo, cooldown, detección de stand y alarmas.
- Consigna máxima seleccionable 480 °C; corte de sonda interna RAW 750 °C; rango válido MAX6675 hasta 800 °C, margen interno 35 °C.
- Las salidas de calefactor/relé y de bomba vienen habilitadas (`HONY_ENABLE_MAINS_OUTPUTS=1`, `HONY_ENABLE_PUMP_OUTPUT=1`). La habilitación de software **no verifica por sí sola** aislamiento, desconexión física ni seguridad de operación.
- Se reportó una medición externa estable de 480 °C, pero la curva de conversión interna->salida por encima de 300 °C sigue siendo extrapolada hasta obtener pares RAW/externo adicionales.
- El proyecto **no usa ni incorporará sensores de caudal o paso de aire**. AIR % representa exclusivamente la orden de accionamiento.

## Documentación histórica

El [pinout del Arduino Nano](pinout-reference.csv) y el [esquema de 12 hojas V2.0.5](Hony850Pro_Esquematico_Conexiones_Completas_V2_0_5.pdf) se conservan **solo para la revisión Nano**. No conectarlos a un ESP32-S3 usando los números de pin de esas hojas.

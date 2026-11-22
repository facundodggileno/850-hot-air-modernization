# Hony 850 Pro — ESP32-S3 v3.0.0 (port preliminar)

Migrado de firmware Nano modular V2.0.5. No es un reemplazo listo para uso con red:
**el port no ha sido compilado con el toolchain ESP32-S3 ni probado físicamente**.
Esta carpeta de revisión está en una rama separada; el Nano original de `main`
no ha sido modificado.

## ESTADO ELÉCTRICO: bloqueado por defecto
`Config.h` define `HONY_ENABLE_MAINS_OUTPUTS` en **0** por defecto.
En ese estado las salidas PUMP, HEATER y RELAY permanecen LOW; el
operador puede comprobar pantalla, sensores, encoders, stand y sincronismo
sin energizar las salidas de mando. Nunca interpretarlo como una prueba
de corte físico o de seguridad de 220V.

## Arduino IDE
- Instalar placas de Espressif **Arduino-ESP32 3.x** con ESP-IDF 5.x.
- Placa: **ESP32-S3 Dev Module**. Flash/PSRAM según versión real N16R8.
- Instalar biblioteca `hd44780` de Bill Perry.
- Instalar biblioteca `MAX6675` compatible con constructor
  `MAX6675(sck, cs, so)` y `readCelsius()`.
- Abrir el archivo `Hony850Pro_ESP32S3_v3_0_0.ino` de ESTA carpeta.
- Configurar USB CDC On Boot si usás el USB nativo para el monitor serie.
- Monitor serie 115200 baudios.

## Conexiones acordadas
| Señal | ESP32-S3 GPIO |
|---|---:|
| ZC H11AA1 | 4 |
| PUMP MOC3023 | 5 |
| HEATER MOC3083 | 6 |
| RELAY driver BC337 | 7 |
| SPEAKER BC337 | 8 |
| STAND (HIGH = apoyado) | 9 |
| TEMP CLK / DT / SW | 10 / 11 / 12 |
| AIR CLK / DT / SW | 13 / 14 / 15 |
| MAX6675 CS / SO / SCK | 16 / 17 / 18 |
| LCD SDA / SCL | 1 / 2 |

Reservados sin funciones implementadas: CS termocupla sobre placa
GPIO21, CS preheater GPIO39; GPIO40..42, 47/48 posibles display SPI.
El display adicional y las dos termocuplas futuras aún no están programados.

## Alimentación de la Hony
- Fuente original aislada 5V/2A y buck 5V -> 3.3V.
- ESP32 pin 3V3 alimentado desde buck externo, verificar 3.30-3.35V
  bajo carga y no mezclar con USB VBUS sin inspeccionar ruta de alimentación.
- PCF8574T a 3.3V con SDA/SCL pull-ups a 3.3V; LCD1602 VDD a 5V,
  VSS GND y R/W soldado a GND (modificacion hecha en el taller).
- MAX6675, encoders y pull-ups stand/ZC a 3.3V.
- Relé 5V y speaker 5V con drivers BC337.
- GND común solo en la zona aislada de baja tensión.
- Comprobar que el control de retroiluminación del LCD no inyecte
  5V hacia un pin PCF8574T alimentado a 3.3V.

## LCD con R/W desconectado
La biblioteca hd44780_I2Cexp usa mapeo explícito para **escritura
exclusivamente**: dirección 0x27, P0=RS, P2=E, P4..P7=D4..D7,
P3=backlight activa HIGH. No se usa autodetección capaz de leer BF.
Confirmar mapeo real, polaridad de backlight y niveles lógicos del LCD.
El controlador LCD concreto debe reconocer HIGH de 3.3V aunque esté a 5V.

## Cambios frente a V2.0.5
- Pinout reemplazado por GPIO nativos ESP32-S3.
- Timer1 AVR -> **GPTimer** ESP-IDF con retardo desde cruce por
  cero + pulso 180us, manteniendo AIR 100% como gate HIGH continuo.
- Calefactor conserva control por pares de semiciclos completos.
- Reemplazo de <avr/io.h>, <avr/interrupt.h>, <avr/wdt.h>,
  <avr/pgmspace.h>, registro SRAM y PSTR/snprintf_P.
- `EEPROM` -> `Preferences` (NVS ESP32). No importa automaticamente
  los parametros PID previos del Nano; defaults Kp1.994 Ki0.168 Kd1.707.
- `esp_task_wdt` (2 segundos) en lugar de watchdog AVR de 500ms.
- Deteccion de motivo reset ESP32.
- El autotune con mango en stand ya NO está permitido.
- Se conserva el manejo de PRE-FLOW, COOLDOWN, PID, fallos,
  autotune 3 ciclos y menus; requiere validacion en hardware.

## Lista antes de cualquier ensayo con potencia
1. Medir y verificar el buck, 3.3V, picos, USB backfeed.
2. Pantalla, botones, encoders, stand y termocupla SIN 220V conectados.
3. Probar ZC con fuente de pulsos aislada, y medir con osciloscopio
   retardo de encendido de bomba, pulso ~180us, AIR 20..100%.
4. Verificar funcionamiento sin glitches durante interrupciones,
   escritura NVS, reinicio/watchdog y variaciones de alimentacion.
5. Verificar cada resistencia de entrada a MOC3023/MOC3083 y bases BC337
   para tensiones y corrientes realmente instaladas a 3.3V.
6. Confirmar que heater y relé quedan OFF con GPIO flotantes,
   reinicio, falta de firmware y fallo de alimentacion.
7. Inspeccionar fusible, aislamientos, proteccion termica independiente
   y corte real de relé, con personal competente y bancada segura.
8. Unicamente con todas las verificaciones, cambiar deliberadamente
   `HONY_ENABLE_MAINS_OUTPUTS` a 1 y validar gradualmente el montaje.

**No existe ni se planifica sensor de caudal/flujo de aire.**
El porcentaje AIR representa SOLO el mando solicitado, no una medicion.

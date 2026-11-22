# Validación pendiente ESP32-S3 v3.0.0

**Estado: código fuente candidato, no compilado ni ensayado en ESP32-S3.**

| Módulo / función | Estado |
|---|---|
| GPIO 1,2,4..18 | Definidos; continuidad con placas reales por comprobar |
| LCD PCF8574T 3.3V, LCD 5V, RW GND | Configuración explícita de escritura; sin prueba |
| Lectura MAX6675 | Fuente del Nano conservada; sin prueba |
| Encoders TEMP y AIR | Fuente conservada; sin prueba |
| Microswitch stand | La lógica obliga OFF también en autotune |
| GPTimer 1MHz | Programación inicial; sin oscilograma |
| MOC3023 180us y control por fase | **No verificado eléctricamente** |
| MOC3083 por semiciclos | **No verificado eléctricamente** |
| BC337 relé / buzzer | Corriente base con 3.3V por medir |
| PID, autotune, modos | Código heredado; ensayos pendientes |
| EEPROM -> NVS | Lectura/escritura migrada; sin prueba |
| Watchdog 2s | Port preliminar; reset/recuperación pendiente |
| Seguridad térmica independiente | Revisión física pendiente |
| Estado de salidas AC por defecto | Configurado para LOW; requiere medida |
| Termocuplas futuras y SPI grande | Reservadas, no implementadas |

No considerar una prueba lógica ni una compilación como demostración
de corte físico, protección térmica o seguridad a 220 V.

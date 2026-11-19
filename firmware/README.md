# Firmware Hony 850 Pro

## Versión actual

**[V2.0.5 — Arduino Nano, código modular](current/Hony850Pro_Nano_Modular_v2_0_5/)**. Conservar los **25 archivos juntos** dentro de `Hony850Pro_Nano_Modular_v2_0_5/` al abrir el `.ino`. [Leer instrucciones detalladas](current/Hony850Pro_Nano_Modular_v2_0_5/README.md) y [evidencias de validación](current/Hony850Pro_Nano_Modular_v2_0_5/VALIDACION.md).

La V2.0.5 es **candidato de laboratorio**, no versión certificada ni ensayada íntegramente en la estación. La V2.0.2 fue la versión con funcionamiento físico normal reportado.

## Arduino IDE

- Placa: Arduino Nano ATmega328P a 16 MHz.
- Bibliotecas: `hd44780` (Bill Perry, con I2Cexp), `MAX6675` con API `readCelsius()`; `Wire`/`EEPROM` del core AVR.
- `avr-g++`/`arduino-cli`: compilación final con bibliotecas reales **aún no verificada** en esta publicación.
- Con la etapa calefactora físicamente desconectada probar pantalla, mandos, stand y termocupla.

## Versiones históricas

Los archivos en `lab`, `reference` y `diagnostics` son registros anteriores; no deben mezclarse con el firmware actual. Ver [CHANGELOG](../CHANGELOG.md).

**Fuera del alcance:** no se instala ni planifica ningún sensor de caudal, flujo o presión de aire.

# Hony 850 Pro · Arduino Nano · Firmware modular V2.0.5

**Candidato de laboratorio / entrega de código — NO validado físicamente.**

Reescritura modular de la lógica de las versiones V1.4.x. El archivo `.ino`
solo coordina la aplicacion: **es obligatorio conservar los `.h` y `.cpp` en
la misma carpeta del sketch**. No mezclarlo con archivos `.ino` de versiones
anteriores.

## Abrir y compilar

1. Extraer la carpeta completa `Hony850Pro_Nano_Modular_v2_0_5`.
2. Abrir `Hony850Pro_Nano_Modular_v2_0_5.ino` en Arduino IDE.
3. Seleccionar **Arduino Nano / ATmega328P / 16 MHz**. Para Nanos antiguos,
   probar la opcion de **Old Bootloader** si la carga falla.
4. Instalar las bibliotecas `hd44780` (Bill Perry, incluye `hd44780_I2Cexp`)
   y una biblioteca MAX6675 compatible con `<max6675.h>` y `readCelsius()`
   (como la de Adafruit). `EEPROM` y `Wire` pertenecen al core AVR.
5. Compilar el proyecto **completo**. No crear otra pestaña `.ino` con el
   firmware viejo. No conectar la alimentacion de potencia para la primera
   verificacion de arranque y UI.

## Requisitos implementados

- Interfaz de **80 a 360 °C corregidos**, pasos de 5 °C.
- Calculo de control en grados RAW (`objetivo RAW = consigna / 0,68`).
  A 360 °C corregidos, el objetivo teorico es **529,4 °C RAW**.
- Autotune PID de **3 ciclos medidos**, con fase previa de estabilizacion,
  almacenamiento constante (sin arreglos de muestras) y guardado EEPROM
  **solo tras finalizar con coeficientes validos**.
- Lee registros PID de V1.4.x (magic `0x8504`, version 1, checksum).
  En ausencia de datos validos, usa PID de referencia Kp=1,994;
  Ki=0,168; Kd=1,707. No sobrescribe EEPROM al arrancar.
- Watchdog HW de 500 ms en funcionamiento; 4 s durante la inicializacion.
  Heartbeat adicional comprobado desde la interrupcion de cruce por cero.
- Autodiagnostico secuencial sin detenerse en el primer aviso: MAX6675,
  cruce por cero, respuesta LCD, comprobacion preliminar de SRAM y reset WDT.
  Muestra **todos los avisos en un periodo total maximo de 4 s** y termina
  de arrancar, sin autoactivar calefaccion.
- Lectura **RAW instantanea en BOOT**, con contador de 0/3 a 3/3 y
  `RAW:ERR` si el MAX devuelve un valor no valido.
- Preflujo no bloqueante de **3 segundos** al comenzar calefaccion y tras
  cambiar AIR. Comprobacion de pulsos Timer1 para AIR menor de 100 %.
- Cooldown automatico con bomba al 100 %: **finaliza en 100 °C RAW (68 °C corregidos)**.
  Si esta apagado y no se pidio bomba manual, el enfriamiento se reactiva al llegar a **110 °C RAW**; la separacion de 10 °C evita oscilaciones.
- Fallos enclavados; no se vuelve a calentar automaticamente tras fallo,
  reset WDT ni rearmado manual.
- Botones TEMP+AIR simultaneos: apagado inmediato del mando de calor,
  FAULT EStop si se mantienen al menos 120 ms.
- Serial a 115200 baudios, telemetria CSV cada segundo.

**Importante:** el caudal de aire mostrado es una orden de control, no una
medicion real. Un Timer1 funcionando NO prueba que la bomba mueva aire.
Tampoco se mide el estado real de un triac o rele pegado. Debe emplearse
una proteccion termica independiente del Nano y verificar electricamente
el mando de bomba y la desconexion efectiva del calefactor. No se contempla
instalar ningun sensor de caudal o de flujo en este proyecto.

## Encoders / botones

| Control | Funcion |
|---|---|
| TEMP girar | Cambia consigna corregida, 80–360 °C |
| TEMP toque | Calefaccion ON / STOP + cooldown |
| TEMP mantener | Atajo: ver Kp/Ki/Kd cuando el calor esta apagado; con FAULT rearme si cumple condiciones |
| AIR girar | Bomba 0–100 %, paso 5; al bajar de 20 % mientras calienta detiene calor |
| AIR toque | Bomba manual cuando esta frio y sin calefaccion |
| **AIR mantener** | Abre **menu PID** con **VER PID ACTUAL** y **HACER AUTOTUNE** (inclusive durante calor para ver PID) |
| Menu PID: girar AIR o TEMP | Alternar entre VER PID ACTUAL y HACER AUTOTUNE |
| **Menu PID: TEMP toque** | **OK / seleccionar opcion** |
| **Menu PID: AIR toque** | **Volver al control normal** |
| VER PID ACTUAL: girar AIR/TEMP | Avanzar por Kp, Ki, Kd. Indica si viene de EEPROM o valores por defecto |
| **VER PID ACTUAL: TEMP toque** | **Salir al menu PID** (si se entro por TEMP largo, volver al control normal) |
| VER PID ACTUAL: AIR toque | Atras (mismo destino que TEMP toque) |
| HACER AUTOTUNE | Requiere calefaccion apagada, MAX valido, ZC disponible y sin FAULT. La configuracion y arranque del autotune admiten mango en stand de manera exclusiva |
| Config autotune | Pantalla **AIR primero, TEMP despues**. Girar AIR regula caudal y TEMP regula temperatura |
| Config autotune: TEMP toque | OK: iniciar autotune de 3 ciclos; AIR toque vuelve al menu PID |
| Durante autotune | TEMP o AIR toque cancela, sin guardar EEPROM |
| TEMP + AIR | Corte inmediato de calor; tras 120 ms FAULT EStop |

## Pinout (idéntico a V1.4.x)

| Componente | Pin Nano |
|---|---|
| H11AA1 cruce por cero | D2 (interrupcion RISING) |
| MOC3023 bomba | D3 |
| MOC3083 calefactor | D4 |
| Rele calefactor (BC337) | D5 |
| Buzzer | D6 |
| Switch stand | D7, HIGH = mango apoyado (requiere polarizacion hardware) |
| Encoder TEMP CLK / DT / pulsador | D8 / D9 / A3 |
| Encoder AIR CLK / DT / pulsador | A0 / A1 / A2 |
| MAX6675 CS / SO / SCK | D10 / D12 / D13 |
| LCD I2C 16x2 | A4 (SDA), A5 (SCL) |

## Modulos

| Archivo | Responsabilidad |
|---|---|
| `Config.h`, `Types.h` | Pines, calibracion, rangos, tipos de datos y fallos |
| `Hardware.h/.cpp` | Mando de potencia, TRIAC por zero-cross, Timer1, watchdog, contadores ISR |
| `Thermal.h/.cpp` | MAX6675, RAW, validez, 3 muestras, mediana y deteccion de saltos |
| `Controls.h/.cpp` | Encoders, pulsacion corta/larga, emergencia, stand |
| `Boot.h/.cpp` | Estados de arranque y avisos de 4 s total |
| `PidControl.h/.cpp` | PID normal con anti-windup y frenado |
| `AutoTune.h/.cpp` | Autotune de 3 ciclos, sin memoria proporcional a la duracion |
| `PidStore.h/.cpp` | EEPROM empaquetada, version y checksum |
| `Safety.h/.cpp` | Evaluacion pura de interlocks y supervisiones termicas |
| `Display.h/.cpp` | LCD de 16x2, valores RAW en BOOT y FAULT |
| `App.h/.cpp` | Orquestacion, modos, flujo, cooldown, transiciones y Serial |
| `.ino` | Punto de entrada: `setup()` y `loop()` |

## Codigo de fallos

`1` termocupla, `2` sobretemperatura, `3` runaway, `4` sin incremento,
`5` perdida de control, `6` reset watchdog, `7` cruce por cero, `8` stall,
`9` LCD, `10` selftest, `11` pulsos bomba, `12` reinicios, `13` EStop,
`14` salto termocupla. Algunos codigos historicos se reservan para
compatibilidad y no se generan actualmente.

El FAULT **no** desaparece por avanzar la presentacion. Los *avisos historicos*
de BOOT son los que terminan en un maximo de 4 s. Ante FAULT activo,
las salidas del calefactor permanecen deshabilitadas.

## Condiciones antes de habilitar calefaccion

- Autodiagnostico completado, sin fallos activos.
- MAX con 3 lecturas validas y actualizadas; RAW por debajo de 595 °C.
- Cruce por cero reciente, LCD operativa, SRAM preliminar suficiente.
- Mango fuera del stand durante HEAT normal. Excepcion exclusivamente para autotune iniciado deliberadamente en el stand, mientras se mantenga activo; AIR >=20 %, preflujo de 3 s completado. Sin caudal fisico verificado no puede considerarse seguro ese modo.
- Cuando AIR <100 %, pulsos del Timer1 generados y verificados.
- Ausencia de boton de emergencia, spikes, sobretemperatura o fallos termicos.

El limite absoluto es **595 °C RAW**, independiente del factor 0,68.
Los coeficientes y el factor de conversion son empiricos, no aseguran
precision de la temperatura de salida.

## Comprobaciones realmente realizadas

- Analisis de sintaxis de **todos los `.cpp` y el `.ino`** con g++ C++11
  sobre cabeceras **simuladas** de Arduino AVR, hd44780 y MAX6675.
- Pruebas de logica de BOOT, limites del PID, EEPROM/checksum, Safety,
  autotune de 3 ciclos.
- Simulacion del coordinador de la estacion con seis escenarios:
  arranque y solicitud de calor tras preflujo, EStop, stand, lectura MAX
  invalida, ZC ausente y sobretemperatura. Se verifico corte de mando.
- **NO** se ha compilado con `avr-g++`/Arduino IDE real; no es posible informar
  Flash/SRAM finales. **NO** se ha probado en la placa real ni con 220 V.

**Version V2.0.5 = candidato de laboratorio, NO firmware certificado ni seguro
para funcionar sin protecciones fisicas.**

## Correccion V2.0.1 (conservada)

`Display.h` incluye `<Wire.h>` antes de la biblioteca `hd44780_I2Cexp`. Corrige `error: 'Wire' was not declared in this scope` al compilar los multiples modulos del proyecto. Ningun parametro de calefaccion, PID, seguridad o pinout se modifica.

Compilacion REAL con avr-g++ / core Arduino pendiente.

## Correccion V2.0.2 — Enlace de simbolos AVR

En `App.cpp`, los simbolos de memoria de `avr-libc` (`__heap_start` y
`__brkval`) ahora se declaran en el ambito global con `extern "C"`,
antes del namespace anonimo. Se corrigio la funcion `freeRam()` para
referenciar esos simbolos sin que el enlazador busque nombres C++
erroneos. Se mantiene la inclusion de `<Wire.h>` de V2.0.1.

No cambia los pines, PID, autotune, limites ni la logica de protecciones.
Pendiente confirmar compilacion real con Arduino AVR / librerias reales.

## Historial: cambio V2.0.3 — Menu PID por pulsacion larga de AIR (controles anteriores)

- Pulsacion larga AIR abre menu de dos opciones: VER PID ACTUAL / HACER AUTOTUNE.
- V2.0.3 usaba AIR corto como OK y TEMP corto como atras; **esto quedo reemplazado en V2.0.5**.
- Ver PID muestra Kp, Ki, Kd reales cargados, y si proceden de EEPROM. No inicia autotune ni cambia coeficientes.
- En configuracion del autotune la pantalla pone AIR a la izquierda y TEMP a la derecha.
- Mantiene atajo previo TEMP largo para ver coeficientes mientras la estacion no calienta.
- No modifica pines, regulacion PID, guarda EEPROM, preflujo ni seguridades.

## Cambio V2.0.5 — Navegacion PID y excepcion delimitada de stand

- En menues, AIR toque siempre es ATRAS y TEMP toque es OK. AIR largo abre menu PID desde Normal.
- VER PID: TEMP toque sale al menu anterior; girar AIR o TEMP pasa por KP/KI/KD.
- AutoTune se configura entrando con TEMP (OK) y se inicia con TEMP (OK); AIR cancela.
- Se permite *iniciar* autotune con mango en stand, pero **NO** HEAT manual en stand.
- El permiso excepcional de stand solo existe durante autotune RUNNING, con solicitud
  de calor, sensor fresco, cero cruces sano y preflujo completo. Se revoca al parar,
  fallar, terminar o cambiar a cooldown. La regulacion normal no admite stand.
- Botones simultaneos, fallos de termocupla, cruce por cero y sobretemperatura
  conservan prioridad y cortan calor incluso durante autotune.
- **Advertencia de hardware**: el stand y los conductos pueden sobrecalentarse si el
  ventilador deja de impulsar aire. El software no detecta caudal real, por lo que
  la excepcion NO esta validada para operacion sin supervisar. Necesita proteccion
  termica independiente y comprobacion fisica del flujo de aire.

# Informe de revision de codigo — V2.0.2

## Verificaciones terminadas

1. Los 10 modulos `.cpp` y el `.ino` superan comprobacion sintactica C++11
   con cabeceras de sustitucion (no es una compilacion con bibliotecas reales).
2. La prueba de autotune alcanza 3 ciclos completos, produce coeficientes
   dentro del rango permitido y el registro EEPROM se recupera con checksum.
   Al corromper una celda de EEPROM, el registro se rechaza.
3. El boot espera un maximo de 4s por MAX, 1s para comprobar ZC y
   hasta 4s *totales* para avisos; simulado con MAX y ZC fallidos.
4. Prueba de simulacion del App con pinout original a caudal 100%:
   a. Inicio sin calentar antes del pedido manual.
   b. Inicio de control SOLO despues de preflujo de 3s.
   c. EStop: mando del rele queda OFF.
   d. Stand: mando del rele queda OFF y no hay auto-reinicio.
   e. MAX invalido: mando del rele queda OFF.
   f. Sin ZC: mando del rele queda OFF.
   g. RAW >=595 C: mando del rele queda OFF.
5. Verificado que `PidEeprom` reside en modulo propio `PidStore.h`,
   eliminando el problema de declaracion previo del sketch monolitico.

## Pendientes imprescindibles antes de uso real

- Compilacion con Arduino AVR core y bibliotecas reales, inspeccion de Flash,
  SRAM y stack (Nano 328P: 32KB de Flash, 2KB SRAM, con bootloader reducido).
- Ensayo con fuente de laboratorio, sin tension de red en la seccion de potencia.
- Verificacion de polaridades D3/D4/D5, D7 y señales de cruce por cero.
- Verificacion de fase del Timer1 en AIR 20, 40, 60, 95 y 100%.
- Comprobacion del watchdog tras bloqueo en la libreria LCD/I2C y la proteccion
  del bootloader a resets WDT.
- Proteccion termica independiente y verificacion del comportamiento real
  del circuito de potencia y del funcionamiento de la bomba. No se contempla
  agregar un sensor de caudal ni de flujo.

No atribuir el resultado de las simulaciones a una validacion electrica.

## Cambio 2.0.1
Corregido el orden de cabeceras de `Display.h`: `<Wire.h>` se incluye antes de `<hd44780ioClass/hd44780_I2Cexp.h>`. La prueba previa con stubs no encontro este defecto de la biblioteca real. La solucion se verifico por inspeccion del orden de inclusiones; no hay compilador AVR instalado para confirmar el binario.

## Cambio V2.0.2

Corregido error de enlazado `undefined reference to (anonymous namespace)::__brkval`
/ `__heap_start` mediante declaraciones `extern "C"` globales.
Se prueba con stubs de Arduino y la compilacion de todos los modulos en
modo host; no hay herramientas AVR reales instaladas en el entorno.

## Cambio V2.0.3 — Menu PID

Cambios limitados a App, Display, Types, README y nombre del sketch.
Control AIR largo abre menu PID; hay opciones separadas de ver Kp/Ki/Kd y
configurar autotune. Pide calefaccion apagada para iniciar autotune.
Pantalla de configuracion: AIR primero, TEMP despues. No se modifica la
logica de Hardware/Safety/Thermal/PidStore ni sus umbrales.

## Pruebas realizadas para V2.0.3

- Los 10 archivos C++ y el sketch pasaron analisis sintactico C++11 con stubs Arduino/AVR.
- Enlace host de todos los modulos y prueba ejecutada de transiciones: AIR largo abre PID, se alternan las dos opciones, se entra a configurar autotune y se vuelve, VER PID muestra Kp/Ki/Kd, el atajo TEMP largo sigue funcionando, y la opcion de autotune no se abre mientras HEAT esta activo.
- Formato LCD verificado por revision: lineas de 16 columnas, AIR antes de TEMP en setup y en progreso de autotune.
- No fue posible compilar con el core AVR real ni ejecutar en placa. Las pruebas host no certifican funcionamiento fisico.

## Cambio V2.0.4 — Cooldown 100 RAW / aprox. 68 C corregidos

- Config.h: RAW_COOL_STOP se modifica de 68 RAW a **100 RAW**.
- Config.h: RAW_COOL_START pasa de 80 a **110 RAW** para histeresis.
- App.cpp: si el equipo esta en reposo, sin bomba manual y temperatura RAW >=110, se enclava el cooldown y se mantiene la bomba al 100% hasta RAW <=100.
- En cooldown, si el sensor deja de estar validado, el mando de bomba sigue al 100%. El fallo del sensor sigue impidiendo calefaccion.
- No se modificaron la conversion 0,68, PID, autotune 3 ciclos, EEPROM, preflujo 3 s, pines ni protecciones de potencia.
- Comprobaciones host (C++11 con stubs) y pruebas del estado de cooldown; **sin compilacion AVR real ni validacion fisica**.

## Cambio V2.0.5 — UI y autotune en soporte

- UI: menu AIR=atras; TEMP=OK; pantalla PID TEMP=salir.
- La autorizacion a calentar en stand solo puede provenir de beginTune y solo
  mientras TuneStatus::Running. En HEAT normal el stand sigue cortando.
- Los puntos de corte trip/cooldownStart/finishTune/clearFault borran el permiso.
- Se deben ensayar en la placa real la condicion en stand, salida del autotune
  y el corte por cada FAULT; los tests host no verifican flujo fisico.

### Pruebas host ejecutadas para V2.0.5

- C++11: sintaxis de todos los `.cpp` y del `.ino` con stubs Arduino AVR.
- Enlace de todos los modulos y ejecucion del test de interfaz: AIR corto vuelve, TEMP corto selecciona;
  TEMP corto sale de la pagina PID, giro de encoder avanza Kp/Ki/Kd.
- Entrada a autotune en stand solo despues de seleccionar desde menu; preflow de 3 s
  antes de autorizar HEAT; al cancelar con AIR, comando del calefactor a cero y override borrado.
- Calefaccion normal denegada en stand, incluido despues de cancelar autotune.
- FAULT Termocupla revoca override; reingresar al stand durante calefaccion normal inicia cooldown.
- Regresion cooldown V2.0.4: 100 RAW corta bomba, 110 RAW reinicia, dato invalido mantiene bomba.
- ZIP: 25 archivos, integridad completa y contenidos iguales a carpeta de origen.
- **No** se ha compilado ni medido memoria con toolchain AVR real; **no** se ha verificado
  hardware ni caudal real de aire.

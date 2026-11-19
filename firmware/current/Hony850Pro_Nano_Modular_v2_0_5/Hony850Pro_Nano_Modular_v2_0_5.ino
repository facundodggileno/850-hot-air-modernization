/*
  HONY 850 PRO - NANO MODULAR V2.0.5
  Reescritura de cero, basada en el pinout y comportamiento acordados.
  Dependencias: hd44780 (Bill Perry), MAX6675 (Adafruit), Wire, EEPROM.
  ATmega328P Nano 16 MHz / 50 Hz de red.

  El codigo NO puede medir caudal fisico, triac pegado ni la temperatura
  real de boquilla. La escala 0.68 es una calibracion empirica.
  Para operacion segura se necesita interlock de aire REAL y corte termico
  independiente del Nano. No usar para soldadura hasta validacion fisica.
*/
#include "App.h"
App station;
void setup(){station.begin();}
void loop(){station.tick();}

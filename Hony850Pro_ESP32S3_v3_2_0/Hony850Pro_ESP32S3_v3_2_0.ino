/*
 HONY 850 PRO - ESP32-S3 V3.2.0
 PORT EN PRUEBAS, etapa de potencia bloqueada por defecto.
 LCD1602 5V, PCF8574T 3.3V, R/W LCD a GND.
 SIN sensor de caudal; AIR% solo es consigna a bomba.
 Biblioteca hd44780 + MAX6675 (readCelsius), Arduino-ESP32 3.x.
*/
#include "App.h"
App station;
void setup(){station.begin();}
void loop(){station.tick();delay(1);}

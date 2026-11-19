# Próximas mejoras

## Prioridad alta

- [ ] Compilar V2.0.5 con Arduino AVR real y dependencias fijadas; documentar Flash y SRAM.
- [ ] Verificar en banco real el arranque, LCD, dos encoders, fallo de MAX, watchdog, emergencia y stand.
- [ ] Ensayar el autotune de 3 ciclos, su cancelación en stand y la revocación de permisos.
- [ ] Medir disipación del H11AA1, inspeccionar circuitos de potencia y confirmar físicamente que el relé desconecta la calefacción.
- [ ] Evaluar e implementar una protección térmica **independiente del firmware** para la unidad de trabajo.

## Prioridad media

- [ ] Consolidar una BOM de valores de resistencias verificados en las placas.
- [ ] Revisar calibración con termómetro externo, misma boquilla y distancia de medición definida.
- [ ] Agregar registros de prueba con identificación de hardware y firmware, temperatura y resultados.
- [ ] Generar archivos CAD/EDA editables **sólo cuando correspondan al cableado real confirmado**, no a un diagrama hipotético.

## Fuera del alcance, definitivamente

**No habrá ningún sensor de caudal, flujo ni paso de aire** y no se planea incorporarlo en versiones futuras. El control de bomba seguirá siendo de lazo abierto. Tampoco se presenta como hecho la construcción de una PCB o un esquema de fabricación que aún no esté respaldado por el montaje.

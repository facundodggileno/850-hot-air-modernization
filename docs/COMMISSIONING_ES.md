# Checklist de puesta en marcha y validación

**Estado:** plan de verificación. Las casillas **no** indican ensayos completados hasta aportar registro con fecha y evidencia. Por trabajar con 220 V, la sección de potencia requiere revisión por personal cualificado y protecciones externas independientes.

## Con la alimentación de potencia físicamente aislada

- [ ] Medir ausencia de continuidad entre red, GND lógico y carcasa donde corresponda; inspeccionar protección a tierra.
- [ ] Confirmar fuente aislada 5 V, polaridad, consumo y ripple en Nano, LCD y MAX6675.
- [ ] Confirmar salidas D3/D4/D5 en condiciones OFF y en arranque/reset (sin potencias conectadas).
- [ ] Comprobar D7 HIGH = apoyado y desactivación en todos los estados HEAT normales.
- [ ] Comprobar D2 del H11AA1 **sólo mediante método aislado adecuado**, nunca con entrada de laboratorio común conectada a red directamente.
- [ ] Verificar lectura RAW, fallos de termocupla, botones, encoders y pantalla sin corrupción.
- [ ] Verificar que TEMP+AIR solicita emergencia y que no se rearma automáticamente tras FAULT.
- [ ] Medir el comportamiento de la lógica de cooldown 110/100 °C RAW por simulación segura.
- [ ] Verificar que la excepción de stand del autotune no se conserva después de cancelar, terminar o entrar en fallo.

## Etapas de potencia — revisión eléctrica previa obligatoria

- [ ] Confirmar el valor, potencia nominal, montaje y temperatura de los resistores del H11AA1; revisar disipación.
- [ ] Confirmar el TRIAC real de la bomba, red de disparo, aislamiento de MOC3023, fusible y cableado.
- [ ] Confirmar red de MOC3083/BTA12 y selección de componentes frente a calefactor ~650 W.
- [ ] Verificar materialmente el relé y que la desconexión física del calefactor se produce cuando corresponde.
- [ ] Incorporar/verificar protección térmica **independiente del Nano** y desconexión ante fallo.
- [ ] Inspeccionar separación de red y baja tensión, cable de protección PE, alivios mecánicos y temperatura del gabinete.
- [ ] Evaluar estabilidad térmica con sonda externa y sistema supervisado, sin funcionamiento desatendido.

No habrá sensor de caudal ni de flujo como parte del proyecto. **AIR% es una orden, no evidencia de movimiento físico**. La validación de la bomba se realiza por inspección y ensayo de banco, sin atribuirle retroalimentación inexistente.

## Evidencia mínima a archivar

Fecha, revisión de código, versión del montaje, entradas/salidas observadas, capturas seriales, fotos, condiciones de prueba, temperatura de boquilla medida con sonda externa, tiempos, resultado y observaciones de seguridad. Registrar expresamente la diferencia entre prueba de software, medición física y requisito pendiente.

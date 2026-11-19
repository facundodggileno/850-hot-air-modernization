# Seguridad eléctrica y térmica — prototipo experimental

> [!CAUTION]
> **RIESGO DE DESCARGA ELÉCTRICA, QUEMADURAS E INCENDIO.** Hony 850 Pro utiliza 220 VAC y un calefactor adaptado de aproximadamente 650 W. El software no demuestra desconexión física de una carga ni conformidad eléctrica. Esta documentación **no** es una homologación ni autoriza operación desatendida.

## Limitaciones de la evidencia actual

1. La lectura MAX6675 depende de la posición del termopar, cableado y ruido eléctrico; el factor 0,68 es empírico, no certificación de precisión.
2. El Nano gobierna optotriacs y un relé; la orden OFF no comprueba contactos pegados, TRIAC averiado o cableado incorrecto.
3. Los pulsos del temporizador prueban una **orden** de disparo, no el movimiento físico de la bomba.
4. Hubo un TRIAC de calefactor averiado y temperaturas elevadas en resistencias del circuito de cruce por cero; no son problemas teóricos.
5. El stand y el watchdog son defensas de software, no barreras térmicas independientes.

## Validaciones físicas necesarias

- Verificación de fuente aislada, PE, fijaciones, fusible, separación/aislamiento de tensión de red y disipación.
- Confirmación de redes de resistencia/gate/snubber, contacto de relé y apagado físico del calefactor.
- Protección térmica independiente del microcontrolador y adecuada al montaje, a verificar en la estación real.
- Pruebas supervisadas con sonda externa y registro de cambios, evitando operación desatendida.

**Decisión de proyecto:** no se incorpora ni se incorporará ningún sensor de caudal ni de flujo de aire. El mando de bomba no debe describirse como medición ni como garantía de enfriamiento. No compensar una anomalía de bomba simplemente cambiando límites del PID o el porcentaje indicado.

La documentación de red se limita a lo respaldado por el prototipo y las mediciones disponibles; [PDF V2.0.5](../hardware/Hony850Pro_Esquematico_Conexiones_Completas_V2_0_5.pdf) distingue conexiones registradas y detalles que aún requieren inspección.

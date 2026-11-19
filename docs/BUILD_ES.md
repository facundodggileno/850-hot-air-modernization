# Cómo reproducir el prototipo — guía de construcción

**Alcance:** reproducir conceptualmente el montaje de la Hony 850 Pro conservando bomba, carcasa y mango, con control digital Nano. Esta guía **no sustituye un diseño eléctrico certificado ni suministra valores no medidos**.

## Preparación

- Revisar primero la [seguridad](safety.md) y la [BOM](../bom/BOM.csv).
- Inspeccionar visualmente la estación Hony 850 donante; identificar su bomba de diafragma, chasis, interruptor, cableado de red, puesta a tierra y mangera de aire.
- Comparar las [fotos reales](photos/README.md) y el [esquema V2.0.5](../hardware/Hony850Pro_Esquematico_Conexiones_Completas_V2_0_5.pdf).
- No inferir un valor de componente por el color de un cable o por una imagen genérica.

## Arquitectura y montaje documentado

1. **Control 5 V.** Arduino Nano, LCD 16×2 I²C (SDA A4, SCL A5), dos encoders TEMP y AIR, buzzer y switch de soporte. [Pinout](../hardware/pinout-reference.csv).
2. **Termocupla.** Termopar K en mango y módulo MAX6675 (D10/D12/D13). En el prototipo se reubicó la unión ~5 mm, por lo que se necesita calibrar para el propio montaje.
3. **Detección de red.** H11AA1 con aislamiento óptico; salida lógica a D2. Los valores del circuito del lado de CA deben comprobarse físicamente: hubo resistencias que calentaban excesivamente.
4. **Potencia.** Se documenta el uso de MOC3023/TRIAC en bomba y MOC3083/BTA12-600B más relé para calefactor. No crear esta etapa copiando solamente el mapa funcional. Confirmar componentes, tensión, disipación, separación galvánica y valores faltantes antes de cablear.
5. **Calefactor.** Resistencia cerámica adaptada Yihua 8786D medida ~74,9 Ω. A 220 V equivale a ~646 W por cálculo, no a una certificación ni una ficha de proveedor.
6. **Interfaz.** LCD y controles en el frontal del gabinete Hony; el stand informa la posición del mango. Durante uso normal, el firmware deniega calefacción con el mango apoyado.
7. **Código.** Usar los 25 archivos en [firmware/current](../firmware/current/Hony850Pro_Nano_Modular_v2_0_5), sin combinarlos con los sketches anteriores.

## Primer arranque y evaluación

Seguir [puesta en marcha](COMMISSIONING_ES.md). Con **potencia del calefactor desconectada físicamente**, comprobar la lógica, pulsaciones, I²C, interrupción de cruce y transiciones de software. No confundir los ensayos de host con ensayos de potencia.

## Exclusiones deliberadas

No se utiliza ni se planea instalar **ningún sensor de caudal, flujo ni paso de aire**. La indicación porcentual AIR es una orden de la bomba, no una medida. No se debe escribir en la documentación que ese sensor se añadió ni que forma parte de un plan futuro.

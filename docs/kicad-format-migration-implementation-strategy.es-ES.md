# Estrategia de implementación para migraciones de formato KiCad

Este texto está sincronizado estructuralmente con la referencia inglesa; los términos
técnicos de KiCad, tokens y nombres de archivos se mantienen deliberadamente sin traducir.

Este documento convierte las diferencias de formato de
`kicad-file-format-version-differences.md` en una guía de implementación para migraciones
entre versiones mayores adyacentes de KiCad.

La frontera principal es KiCad 5 a KiCad 6. KiCad 4 y 5 usan archivos legacy para esquemas
y bibliotecas de símbolos (`.sch`, `.lib`, `.dcm`), mientras que KiCad 6 y versiones
posteriores usan archivos S-expression (`.kicad_sch`, `.kicad_sym`). Los archivos PCB y
footprint ya son S-expression en estas versiones, pero sus conjuntos de funciones y números
de versión siguen necesitando reglas explícitas de reescritura.

Última actualización: 2026-06-04.

## Alcance

Este documento es una hoja de ruta de implementación, no una promesa de soporte de release.
Describe cómo implementar:

- Migraciones hacia delante como KiCad 6 a 7 y KiCad 5 a 6.
- Migraciones hacia atrás como KiCad 7 a 6, KiCad 6 a 5 y KiCad 5 a 4.
- Puntos de extensión para versiones adyacentes posteriores como 7 a 8, 8 a 9,
  9 a 10 y 10 a la rama de desarrollo actual.

El convertidor C++ actual es principalmente un motor de downgrade. Si la versión de formato
origen es anterior a la versión objetivo solicitada, actualmente copia el archivo sin cambios
en lugar de actualizarlo. Por tanto, el soporte de upgrade debe añadirse como una ruta de
migración separada, no debilitando las reglas de downgrade.

## Mapa de versiones objetivo

| Objetivo KiCad | Biblioteca de símbolos | Esquema | PCB / footprint | Worksheet | Reglas de diseño |
| --- | ---: | ---: | ---: | ---: | ---: |
| 4.0 | Legacy `.lib` 2.3 | Legacy `.sch` V2 | `4` | Legacy drawing sheet | Ninguna |
| 5.0 / 5.1 | Legacy `.lib` 2.4 | Legacy `.sch` V4 | `20171130` | Legacy drawing sheet | Ninguna |
| 6.0 | `20211014` | `20211123` | `20211014` | `20210606` | `20200610` |
| 7.0 | `20220914` | `20230121` | `20221018` | `20220228` | `20200610` |
| 8.0 | `20231120` | `20231120` | `20240108` | `20231118` | `20200610` |
| 9.0 | `20241209` | `20250114` | `20241229` | `20231118` | `20200610` |
| 10.0 | `20251024` | `20260306` | `20260206` | `20231118` | `20200610` |

## Forma del motor de migración

Implementar las migraciones como una canalización con adaptadores de familia de archivos:

1. Detectar el tipo de documento por el root S-expression head o por extensión/header legacy.
2. Analizarlo como un árbol mutable o un typed intermediate model.
3. Seleccionar una ruta desde la familia/versión origen hasta la familia/versión objetivo.
4. Aplicar pasos de migración ordenados. Cada paso debe estar protegido por tipo de
   documento, versión origen y versión objetivo.
5. Escribir la familia de archivo objetivo con la versión y extensión objetivo.
6. Emitir warnings para cada eliminación con pérdida, valor por defecto de respaldo o
   conversión semántica no implementada.

No implementar la migración como una simple reescritura del número de versión superior.
Cada frontera de versión puede introducir o quitar tokens, estructuras de layout, atributos
de objeto o incluso familias completas de archivos.

## Reglas entre familias

### Familia legacy KiCad 4/5

KiCad 4 y 5 requieren writers legacy para salidas de esquema y biblioteca de símbolos:

- Esquema: `.sch`, con `EESchema Schematic File Version 2` para KiCad 4 y
  `Version 4` para KiCad 5.
- Biblioteca de símbolos: `.lib` más `.dcm` opcional, normalmente
  `EESchema-LIBRARY Version 2.3` para KiCad 4 y `Version 2.4` para KiCad 5.
- Proyecto: legacy `.pro`.

Los datos de esquema y símbolo V6+ no pueden degradarse a KiCad 4/5 podando algunos nodes
S-expression. La implementación necesita un serializer legacy real o un warning claro de
que la conversión no está soportada.

### Familia S-expression KiCad 6+

KiCad 6 y posteriores usan archivos S-expression para esquema, biblioteca de símbolos, PCB,
footprint, worksheet y reglas de diseño. La mayoría de migraciones adyacentes puede
gestionarse como reescrituras de árbol controladas por versión:

- Añadir o eliminar feature nodes según la versión objetivo.
- Renombrar campos cuando cambió el formato.
- Convertir formatos boolean-list a legacy presence atoms, o al revés.
- Normalizar estructuras UUID, ID, font, text y property para el objetivo.

## Estrategia de migración hacia delante

La migración hacia delante debe conservar la semántica origen y sintetizar valores por
defecto seguros para el objetivo. No debe inventar intención de diseño nueva.

Comportamiento recomendado:

- Si origen y objetivo pertenecen a la misma familia, analizar y escribir el formato
  objetivo más nuevo aplicando reescrituras de compatibilidad conocidas.
- Si falta un campo en el origen, omitirlo cuando el formato nuevo lo permita; si no,
  escribir valores por defecto similares a KiCad.
- Si la migración cruza de KiCad 4/5 legacy a KiCad 6+ S-expression, usar importers
  dedicados para `.sch`, `.lib`, `.dcm` y `.pro`.
- Para imports legacy difíciles, usar KiCad como referencia: cargar el proyecto antiguo
  en la versión KiCad adecuada o en KiCad 6+, guardar y comparar la estructura generada
  con la salida del convertidor.

### Upgrade de KiCad 6 a KiCad 7

Es un upgrade S-expression dentro de la misma familia. Puede implementarse como una
reescritura estructurada más una actualización de versión objetivo.

Acciones clave:

- Actualizar versiones objetivo a symbol `20220914`, schematic `20230121`, board /
  footprint `20221018` y worksheet `20220228`.
- Conservar objetos V6 existentes de esquema, símbolo, board, footprint, worksheet y DRC.
- Añadir defaults solo donde KiCad 7 exige un valor que KiCad 6 no escribía.
- Convertir campos antiguos compatibles a la nueva escritura cuando sea necesario, por
  ejemplo `netclass_flag` a `directive_label`.
- Normalizar la geometría de text box a la representación KiCad 7 `at` / `size` si se
  encuentra una representación antigua `start` / `end`.
- Mantener válida la información de simulación V6, pero no fabricar datos completos de
  simulation model KiCad 7 sin suficientes campos.
- Conservar o sintetizar defaults relacionados con DNP solo cuando el tipo de objeto
  objetivo los soporte.
- Para PCB y footprints, conservar objetos V6 y habilitar formas compatibles con V7 para
  stroke formatting, footprint private layers, dimensions, teardrop keywords, net ties y
  pad/via zone-layer connections cuando haya datos suficientes.
- Para worksheets, escribir la versión V7 y conservar font data solo si se puede expresar
  de forma compatible con KiCad 7.

Fixtures de validación:

- Un esquema V6 con labels, fields, hierarchical sheets y simulation fields.
- Un board V6 con vias, pads, zones, dimensions y footprint text.
- Una biblioteca de símbolos V6 con properties y primitives simples.

### Upgrade de KiCad 5 a KiCad 6

Es la frontera de migración hacia delante más importante porque los archivos de esquema y
símbolo cambian de familia.

Adaptadores requeridos:

- Migración de proyecto `.pro` a `.kicad_pro` JSON.
- Legacy `.sch` V4 a `.kicad_sch` `20211123`.
- Legacy `.lib` / `.dcm` 2.4 a `.kicad_sym` `20211014`.
- `.kicad_pcb` / `.kicad_mod` `20171130` a `20211014`.
- Legacy drawing sheet a `.kicad_wks` `20210606`, si se soporta conversión worksheet.

Acciones clave:

- Analizar records legacy de esquema en un typed model antes de escribir S-expressions
  KiCad 6. Evitar reemplazo de texto por líneas en `.sch`.
- Mapear símbolos de esquema legacy a symbol instances KiCad 6 con UUIDs, properties,
  paths, sheet instances y library identifiers.
- Generar UUIDs estables donde KiCad 6 los exige. Usar UUIDs deterministas derivados de
  rutas origen e identidad de objeto cuando importe la reproducibilidad.
- Convertir legacy library aliases, fields, drawing primitives, pins y documentation
  records en símbolos `.kicad_sym`.
- Conservar descriptions, keywords y documentation links de `.dcm` como metadatos de
  símbolos KiCad 6 cuando sea posible.
- Convertir legacy project settings a `.kicad_pro` y `.kicad_prl` solo para settings con
  equivalentes claros. Advertir por settings UI o cache ignorados.
- Actualizar la versión PCB a `20211014` conservando features KiCad 5 como custom pads,
  multi-layer keepouts, 3D model offsets y footprint text lock state.

Áreas de pérdida esperadas:

- Algunos legacy project settings pueden no tener equivalente JSON KiCad 6.
- El comportamiento legacy library rescue/cache puede requerir remapeo explícito de símbolos.
- Construcciones de esquema V5 dependientes de reglas antiguas de lookup de bibliotecas
  pueden requerir warnings o datos sidecar de biblioteca de símbolos.

## Estrategia de migración hacia atrás

La migración hacia atrás debe eliminar o reescribir construcciones no soportadas e informar
la pérdida. El archivo objetivo debe poder cargarse en la versión KiCad solicitada incluso
si no se puede preservar semántica más nueva.

Comportamiento recomendado:

- Aplicar reglas de downgrade desde la más nueva a la más antigua, para quitar features
  posteriores antes de ejecutar reescrituras de compatibilidad más antiguas.
- Mantener warnings concretos: nombrar la feature eliminada, contar nodes afectados e
  incluir la versión de introducción.
- Para downgrades S-expression dentro de la misma familia, preservar identidad de objeto
  donde el objetivo la soporte.
- Para downgrade V6+ a V5/V4 de esquema o símbolo, usar un writer legacy dedicado y tratar
  objetos V6+ no soportados como pérdida.

### Downgrade de KiCad 7 a KiCad 6

Es un downgrade S-expression dentro de la misma familia. Debe implementarse como eliminación
dirigida y reescritura de compatibilidad.

Versiones objetivo:

- Biblioteca de símbolos: `20211014`.
- Esquema: `20211123`.
- Board / footprint: `20211014`.
- Worksheet: `20210606`.
- Design rules: `20200610`.

Reglas clave:

- Eliminar symbol class flags, symbol fonts, text boxes, text colors, unit display names y
  comportamiento KiCad 7-only property-ID.
- Eliminar schematic graphic primitives introducidas después de V6 sin equivalente V6,
  incluyendo text boxes y nuevos label fields.
- Reescribir `directive_label` a la forma compatible V6 `netclass_flag` si existe un mapeo
  fiel; de lo contrario advertir.
- Eliminar o degradar dash-dot-dot line style, text hyperlinks, field name visibility,
  do-not-autoplace options, DNP support y V7 simulation model fields.
- Mover o simplificar symbol and sheet instance data al layout esperado por KiCad 6.
- Eliminar PCB text boxes, image objects, first-class net ties, V7 teardrop keywords,
  footprint private-layer changes que V6 no puede parsear y pad/via zone-layer-connection
  additions.
- Convertir formatos V7 stroke, font, boolean, lock y hide a formas compatibles con V6.
- Eliminar worksheet font support añadido en `20220228`.

Áreas de pérdida esperadas:

- Text boxes, images, net ties, DNP flags, hyperlinks y modern simulation model data pueden
  no tener equivalentes V6 fieles.
- Algunos V7 PCB dimensions y teardrop settings deben eliminarse o aplanarse.

### Downgrade de KiCad 6 a KiCad 5

Es un downgrade entre familias para archivos de esquema y símbolo.

Writers requeridos:

- `.kicad_sch` `20211123` a legacy `.sch` V4.
- `.kicad_sym` `20211014` a legacy `.lib` 2.4 más `.dcm`.
- `.kicad_pro` JSON a legacy `.pro`.
- `.kicad_pcb` / `.kicad_mod` `20211014` a `20171130`.

Reglas clave:

- Serializar KiCad 6 schematic symbols, wires, buses, labels, junctions, sheets, fields y
  page settings en el formato de records legacy `.sch`.
- Convertir UUIDs y paths KiCad 6 en timestamps legacy o identifiers deterministas cuando
  el objetivo lo requiera.
- Dividir KiCad 6 symbol metadata en `.lib` symbol definitions y `.dcm`
  documentation records.
- Eliminar estructuras de esquema y símbolo KiCad 6-only que los archivos legacy no pueden
  representar directamente. Advertir por cada eliminación.
- Convertir settings `.kicad_pro` a `.pro` solo cuando exista equivalente legacy conocido.
  Ignorar o advertir por settings JSON-only.
- Degradar versiones board y footprint a `20171130`.
- Eliminar features V6 board/footprint que KiCad 5 no puede parsear, incluyendo V6+
  UUID-only structures, newer zone behavior, rule file assumptions y cualquier objeto
  introducido después del formato PCB V5.
- Conservar features PCB compatibles con KiCad 5: custom pads, multi-layer keepouts,
  3D model offset en mm usando el parámetro `offset` y locked footprint text.

Áreas de pérdida esperadas:

- Detalles S-expression de esquema y símbolo KiCad 6 no siempre se mapean a campos de
  records legacy.
- Project JSON settings, identidad UUID moderna y algunos detalles de linkage de biblioteca
  pueden reducirse o descartarse.
- Archivos `.kicad_dru` no tienen equivalente standalone en KiCad 5.

### Downgrade de KiCad 5 a KiCad 4

Es mayormente un downgrade dentro de la familia legacy, con PCB aún requiriendo eliminación
de features S-expression.

Formatos objetivo:

- Esquema: `.sch` V2.
- Biblioteca de símbolos: `.lib` 2.3 más `.dcm`.
- PCB / footprint: version `4`.
- Proyecto: legacy `.pro`.

Reglas clave:

- Reescribir el header de esquema de `EESchema Schematic File Version 4` a `Version 2`.
- Eliminar o reescribir records de esquema KiCad 5 que KiCad 4 no puede parsear.
- Degradar headers de biblioteca de símbolos de salida estilo 2.4 a salida estilo 2.3.
- Eliminar symbol fields o attributes que no existen en bibliotecas KiCad 4.
- Degradar archivos board y footprint de `20171130` a version `4`.
- Eliminar features PCB KiCad 5 introducidas después de KiCad 4, incluyendo differential
  pair settings per net class, long pad names, custom pad shape details, multi-layer
  keepouts, 3D model offset changes que KiCad 4 no puede interpretar y locked/unlocked
  footprint text.
- Convertir 3D model offsets a la representación KiCad 4 cuando se conozca una conversión
  de unidades reversible; de lo contrario advertir y eliminar campos offset no soportados.

Áreas de pérdida esperadas:

- Custom pads y multi-layer keepouts pueden necesitar simplificación o eliminación.
- Long pad names pueden necesitar truncado o renombrado determinista.
- Algunos metadatos net-class y footprint text-lock no pueden preservarse.

## Patrones de downgrade adyacentes posteriores

El mismo framework de downgrade debe cubrir versiones adyacentes posteriores:

| Ruta | Foco principal de implementación |
| --- | --- |
| KiCad 8 a 7 | Eliminar V8 generator cleanup fields, PCB fields, generated objects, UUID normalization, custom pad spoke templates, modern teardrops, images, rule-area changes, simulation/exclude flags y worksheet embedded images. |
| KiCad 9 a 8 | Eliminar embedded files, tables, rule areas, component classes, complex padstacks, via stacks, arbitrary user layers, tenting, multiple netclass assignments y netclass color highlighting. |
| KiCad 10 a 9 | Eliminar variants, jumper pads, barcodes, via protection, zone hatch offsets, backdrill, split via types, stopped-netcode assumptions, rounded rectangles, stacked pins, PCB points y property-formatting updates. |
| Desarrollo actual a KiCad 10 | Eliminar features post-10.0: extruded 3D body metadata, native ellipses and ellipse arcs, dielectric frequency-dependent stackup fields, net chains, copper thieving, pad `sim_electrical_type` y PCB table-cell `knockout`. |

Las migraciones hacia delante para estas rutas suelen tener menos pérdida que los downgrades,
pero siguen necesitando reescrituras de compatibilidad y defaults objetivo. Por ejemplo,
KiCad 7 a 8 debe introducir V8-normalized `generator_version` handling solo donde el formato
objetivo lo espera, y KiCad 8 a 9 no debe inventar embedded files, component classes o
padstacks salvo que ya existan en la semántica origen.

## Estrategia de pruebas

Crear un conjunto de fixtures por ruta adyacente y tipo de documento:

- Archivos de proyecto: `.pro` y `.kicad_pro`.
- Archivos de esquema: legacy `.sch` y `.kicad_sch`.
- Bibliotecas de símbolos: `.lib`, `.dcm` y `.kicad_sym`.
- Archivos PCB: `.kicad_pcb`.
- Footprints: `.kicad_mod`.
- Worksheets: legacy drawing sheet y `.kicad_wks`.
- Design rules: `.kicad_dru` solo para KiCad 6+.

Cada prueba debe verificar:

- El tipo origen y la versión origen se detectan correctamente.
- La familia, extensión y versión objetivo son correctas.
- No hay tokens prohibidos por la versión objetivo.
- Las estructuras compatibles conocidas se preservan.
- Los cambios con pérdida producen warnings.
- Reejecutar la misma migración es estable y no sigue modificando el archivo.

Para rutas V5/V6 y V6/V5, añadir golden-file tests generados por KiCad cuando sea posible.
Estas rutas cruzan una frontera de familia de archivos y tienen el mayor riesgo de deriva
semántica.

## Orden de implementación

Orden recomendado:

1. Terminar downgrades S-expression same-family de V7 a V6.
2. Añadir downgrade V5 PCB / footprint a V4 porque permanece en la familia PCB S-expression.
3. Implementar parsers legacy de esquema y símbolo para upgrade V5 a V6.
4. Implementar writers legacy de esquema y símbolo para downgrade V6 a V5.
5. Añadir downgrade legacy de esquema y símbolo V5 a V4.
6. Añadir upgrades same-family como V6 a V7.
7. Extender el mismo framework a rutas V8, V9, V10 y desarrollo actual.

Este orden entrega cobertura útil de downgrade pronto, mientras aísla el trabajo mucho más
difícil de conversión legacy de esquemas y símbolos.

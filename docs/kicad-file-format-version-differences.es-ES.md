# Diferencias de versión del formato de archivo KiCad

Este documento rastrea las diferencias de versión del formato de archivo KiCad utilizadas por el backport.
convertidor. Está organizado para que se puedan agregar nuevas versiones estables o de desarrollo.
sin cambiar el nombre del archivo.

Última actualización: 2026-06-07.

## Fuentes y método

Fuentes revisadas:

- Etiquetas oficiales de GitLab y archivos fuente de KiCad.
- Checkout local de KiCad en `E:/WORKS/MY/kicadProject/kicad`.
- Referencias y etiquetas locales: `origin/4.0`, `4.0.0`, `origin/5.0`, `origin/5.1`,
  `5.0.0`, `5.1.0`, `6.0.0`, `7.0.0`, `8.0.0`, `9.0.0`, `10.0.0`,
y `origin/10.0`.
- KiCad local `master`, usado solo para identificar la rama de desarrollo posterior a 10.0
límites.
- Implementación `kicad-backport-cplus`, especialmente:
  - `src/kicad_backport_versions.cpp`
  - `src/kicad_backport_rules.cpp`
  - `src/kicad_backport_rule_rewriters.cpp`
  - `src/kicad_backport_upgrade.cpp`
  - `src/kicad_backport_legacy.cpp`
  - `src/kicad_backport_pathmap.cpp`
  - `src/kicad_backport.cpp`
- Archivos de encabezado de versión:
  - `pcbnew/kicad_plugin.h` para formatos de PCB KiCad 4/5.
  - `pcbnew/plugins/kicad/pcb_plugin.h` para formatos de PCB KiCad 6/7.
  - `eeschema/sch_file_versions.h`
  - `pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h`
  - `include/drawing_sheet/ds_file_versions.h`
  - `pcbnew/drc/drc_rule_parser.h`
  - `eeschema/general.h` y `eeschema/sch_legacy_plugin.h` para KiCad 4/5
formatos esquemáticos heredados.

Los números de versión se toman de las macros fuente activas de KiCad:

- `SEXPR_SYMBOL_LIB_FILE_VERSION`
- `SEXPR_SCHEMATIC_FILE_VERSION`
- `SEXPR_BOARD_FILE_VERSION`
- `SEXPR_WORKSHEET_FILE_VERSION`
- `DRC_RULE_FILE_VERSION`

Notas:

- Las versiones de expresión S de la placa también cubren archivos de huella `.kicad_mod`.
- `.kicad_dru` permaneció en `20200610` desde KiCad 6.0 hasta las fuentes actuales 10.99.
Esto sólo significa que la macro de versión no cambió; la semántica de las reglas todavía puede tener
cambió.
- `.kicad_pro` es un archivo de proyecto JSON y utiliza configuración/migración de esquema en su lugar
de estas macros de versión de fecha de expresión S. Diferencias de esquema JSON del proyecto
debe ser rastreado por separado.
- Los esquemas y bibliotecas de símbolos de KiCad 4/5 son `.sch`, `.lib` y
Archivos `.dcm`, no `.kicad_sch` o `.kicad_sym`.

## Matriz de familia de archivos principal

| Versión principal de KiCad | Proyecto | Esquemático | Biblioteca de símbolos | PCB/huella | Hoja de trabajo | Reglas de diseño | Punto clave |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 4.0 | Legado `.pro` | Legado `.sch`, `EESCHEMA_VERSION=2` | `.lib` `EESchema-LIBRARY Version 2.3`, `.dcm` | `.kicad_pcb` / `.kicad_mod` Expresión S, versión `4` | Hoja de dibujo heredada | Sin `.kicad_dru` independiente | PCB ya era expresión S; las bibliotecas de esquemas y símbolos todavía eran heredadas. |
| 5.0 / 5.1 | Legado `.pro` | Legado `.sch`, `EESCHEMA_VERSION=4` | Comúnmente `.lib` `Version 2.4`, `.dcm` | `.kicad_pcb` / `.kicad_mod` Expresión S, versión `20171130` | Hoja de dibujo heredada | Sin `.kicad_dru` independiente | PCB agregó almohadillas personalizadas, reservas de múltiples capas y cambios de compensación del modelo 3D; El esquema siguió siendo un legado. |
| 6.0 | JSON `.kicad_pro`, `.kicad_prl` | `.kicad_sch` `20211123` | `.kicad_sym` `20211014` | `20211014` | `.kicad_wks` `20210606` | `.kicad_dru` `20200610` | Nuevos formatos de expresión S de esquemas y símbolos. |
| 7.0 | JSON `.kicad_pro` | `.kicad_sch` `20230121` | `.kicad_sym` `20220914` | `20221018` | `.kicad_wks` `20220228` | `20200610` | Cuadros de texto, fuentes, DNP, cambios en el modelo de simulación, vínculos de red, imágenes, palabras clave en forma de lágrima. |
| 8.0 | JSON `.kicad_pro` | `.kicad_sch` `20231120` | `.kicad_sym` `20231120` | `20240108` | `.kicad_wks` `20231118` | `20200610` | `generator_version`, limpieza de V8, campos de PCB, objetos generados, normalización de UUID. |
| 9.0 | JSON `.kicad_pro` | `.kicad_sch` `20250114` | `.kicad_sym` `20241209` | `20241229` | `.kicad_wks` `20231118` | `20200610` | Archivos incrustados, tablas, áreas de reglas, clases de componentes, padstacks, via stacks, capas de usuarios arbitrarias. |
| 10.0 | JSON `.kicad_pro` | `.kicad_sch` `20260306` | `.kicad_sym` `20251024` | `20260206` | `.kicad_wks` `20231118` | `20200610` | Variantes, puentes, códigos de barras, protección de vía, backdrill, tipos de vía dividida, dejó de escribir códigos de red. |

## Matriz de versión estable

| Etiqueta KiCad | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

## Límite heredado de KiCad 4/5

KiCad 4 y 5 no son sólo versiones antiguas de expresión S para datos esquemáticos. Ellos
utilice una familia de archivos de biblioteca de símbolos y esquemas diferente:

| Área | KiCad 4.0 | KiCad 5.0 / 5.1 |
| --- | --- | --- |
| Encabezado esquemático | `EESchema Schematic File Version 2` | `EESchema Schematic File Version 4` |
| Macro esquemática | `EESCHEMA_VERSION 2` | `EESCHEMA_VERSION 4` |
| Encabezado de la biblioteca de símbolos | Comúnmente `EESchema-LIBRARY Version 2.3` | Comúnmente `EESchema-LIBRARY Version 2.4` |
| versión PCB | `4` | `20171130` |

La versión KiCad 5 PCB/footprint apunta antes de la línea de desarrollo KiCad 6:

| Versión | Cambiar |
| ---: | --- |
| 20160815 | Configuración de pares diferenciales por clase de red |
| 20170123 | `EDA_TEXT` refactorizar; movido `hide` |
| 20170920 | Nombres de pad largos y forma de pad personalizada |
| 20170922 | Las zonas de exclusión pueden existir en varias capas |
| 20171114 | Guarde el desplazamiento del modelo 3D en mm en lugar de pulgadas |
| 20171125 | Texto de huella bloqueada/desbloqueada |
| 20171130 | Desplazamiento del modelo 3D escrito usando el parámetro `offset` |

Implicaciones del backport:

- Los objetivos esquemáticos KiCad 4/5 requieren un escritor `.sch` heredado, no solo un
`.kicad_sch` reescritura de versión.
- Los destinos de símbolos KiCad 4/5 requieren una salida `.lib` / `.dcm` heredada o una salida explícita
Advertencia con pérdida/no implementada.
- Los objetivos de la placa KiCad 4 utilizan la versión `4`; Los objetivos de la placa KiCad 5 utilizan `20171130`.
- UUID V6+, cuadros de texto, archivos incrustados, variantes, tablas, áreas de reglas, componentes
clases, padstacks, via stacks, backdrill y estructuras similares no se pueden
conservado directamente en archivos V4/V5.

### Sincronizacion de la implementacion C++ actual (2026-06-07)

La implementacion actual trata explicitamente el limite legacy de KiCad 4/5: los `.sch` legacy pueden convertirse a `.kicad_sch` V6+, los `.lib` / `.dcm` legacy a `.kicad_sym` V6+, y los `.kicad_sch` / `.kicad_sym` / `.kicad_pro` V6+ pueden escribirse de vuelta como `.sch` / `.lib` + `.dcm` / `.pro` legacy. Entre archivos legacy V4 y V5, por ahora se reescriben sobre todo las cabeceras `.sch` y `.lib` y se conservan los registros brutos.

La conversion de esquemas legacy analiza page metadata, components, fields, AR paths, wires, buses, bus entries, sheets, sheet pins, labels/text, junctions y no-connects. Si la referencia `L` o `F0` sigue siendo un placeholder como `C?`, se prefiere el `AR Ref=` anotado. Las referencias de alimentacion ocultas `#U...` se normalizan cuando es posible a `#PWR` / `#FLG`. La conversion de bibliotecas de simbolos legacy normaliza el `Reference` por defecto del simbolo al prefijo: `C?`, `R12` y `TP?` se escriben como `C`, `R` y `TP`. Las instancias colocadas en el esquema mantienen referencias concretas como `C12` y su footprint property.

El alias de destino `10.99` actual solo avanza las salidas board/footprint a `20260603`. Las bibliotecas de simbolos siguen en `20251024` y los esquemas en `20260306`. Esos destinos de desarrollo de symbol/schematic se habilitaran por separado cuando sean objetivos de conversion explicitos.

El flujo principal de `CONVERTER::normalizeFile()` es: detectar documento legacy o S-expression, resolver el alias de destino o version numerica bruta, reescribir cabeceras legacy para legacy -> V4/V5, escribir S-expression para legacy -> V6+, escribir legacy y sidecar `.dcm` para modern -> V4/V5, elegir copy / upgrade normalization / downgrade rules para modern S-expression y finalmente escribir el atomo `version` destino.

La conversion de proyectos filtra outputs generados, history/backup, Gerber, BOM y archivos temporales. Los destinos V4/V5 no copian `.kicad_prl` modernos. Los destinos V6/V7/V8 generan `.kicad_prl` con numeric visible-items. Los destinos de proyecto V6 eliminan `version` top-level de las library tables, agregan aliases de footprint `.pretty` locales, crean un `sym-lib-table` project-local cuando hace falta y pueden incrustar symbol definitions generadas en el schematic cache.
## Matriz de versiones de desarrollo actual

La rama KiCad `master` revisada ya pasó al desarrollo 11.0.
Estos hallazgos son elementos de desarrollo posteriores a la versión 10.0 y no deben etiquetarse como KiCad.
Soporte de formato estable 10.0:

| tipo de archivo | Versión de desarrollo actual | Notas |
| --- | ---: | --- |
| Tablero `.kicad_pcb` | `20260603` | Bandera eliminatoria en las celdas de la tabla |
| Huella `.kicad_mod` | `20260603` | Las huellas utilizan la versión PCB S-expression |
| Esquema `.kicad_sch` | `20260512` | cadenas de red |
| Biblioteca de símbolos `.kicad_sym` | `20260508` | Primitiva de elipse nativa |
| Hoja de trabajo `.kicad_wks` | `20231118` | Versión del generador / limpieza de KiCad 8 |
| Reglas de diseño `.kicad_dru` | `20200610` | No se encontró ninguna versión específica del desarrollo actual |

Pasos de la versión de desarrollo posterior a 10.0 encontrados hasta ahora:

| Versión | tipo de archivo | Cambiar |
| ---: | --- | --- |
| 20260410 | Board / footprint | Cuerpo extruido en 3D |
| 20260508 | Board / footprint | Primitivas nativas de elipse y arco de elipse de PCB |
| 20260508 | Esquema / símbolo | Primitivas nativas de elipse esquemática/símbolo y arco de elipse |
| 20260511 | Junta | Modelos de apilamiento dependientes de la frecuencia dieléctrica |
| 20260512 | Tablero / esquema | cadenas de red |
| 20260513 | Junta | Modo de llenado de la zona de robo de cobre |
| 20260521 | Board / footprint | Tipos eléctricos de simulación de pad |
| 20260603 | Board / footprint | Bandera eliminatoria en las celdas de la tabla |

## 6,0 a 7,0

### Biblioteca de símbolos

| Versión | Cambiar |
| ---: | --- |
| 20220101 | banderas de clase |
| 20220102 | Fuentes |
| 20220126 | cuadros de texto |
| 20220328 | El cuadro de texto `start/end` cambió a `at/size` |
| 20220331 | Colores de texto |
| 20220914 | Nombres de visualización de unidades de símbolos |
| 20220914 | Los ID de propiedad ya no se guardan |

### Esquemático

| Versión | Cambiar |
| ---: | --- |
| 20220101 | Círculos, arcos, rectas, polis, beziers |
| 20220102 | guión-punto-punto |
| 20220103 | Campos de etiqueta |
| 20220104 | Fuentes |
| 20220124 | `netclass_flag` renombrado a `directive_label` |
| 20220126 | cuadros de texto |
| 20220328 | El cuadro de texto `start/end` cambió a `at/size` |
| 20220331 | Colores de texto |
| 20220404 | Datos de instancia de símbolo esquemático predeterminados |
| 20220622 | Nuevo formato de modelo de simulación. |
| 20220820 | Corrección de datos de instancia de símbolo predeterminado |
| 20220822 | Hipervínculos de objetos de texto |
| 20220903 | Visibilidad del nombre del campo |
| 20220904 | Opción de no colocar automáticamente el campo |
| 20220914 | soporte DNP |
| 20220929 | Los ID de propiedad ya no se guardan |
| 20221002 | Los datos de la instancia volvieron a la definición de símbolo |
| 20221004 | Los datos de la instancia volvieron a la definición de símbolo |
| 20221110 | Datos de instancia de hoja movidos a la definición de hoja |
| 20221126 | Se eliminaron el valor y la huella de los datos de la instancia. |
| 20221206 | Campos del modelo de simulación V6 a V7 |
| 20230121 | `SCH_MARKER` serialización de ruta de hoja |

### PCB / Huella

| Versión | Cambiar |
| ---: | --- |
| 20211226 | Dimensión radial |
| 20211227 | Anulación del ángulo de los radios con alivio térmico |
| 20211228 | `allow_soldermask_bridges` atributo de huella |
| 20211229 | Formato de trazo |
| 20211230 | Dimensiones en huellas |
| 20211231 | Capas de huella privada |
| 20211232 | Fuentes |
| 20220131 | Cuadros de texto |
| 20220211 | Soporte de estrategia de llenado de zona V5 finalizado |
| 20220225 | TEDIT eliminado |
| 20220308 | Texto eliminado y propiedad de texto gráfico bloqueado |
| 20220331 | Trazar en todas las configuraciones de selección de capas |
| 20220417 | Precisiones de dimensiones automáticas |
| 20220427 | Edge.Cuts y Margin excluidos de las capas privadas de huella |
| 20220609 | Palabras clave en forma de lágrima |
| 20220621 | Soporte de imagen |
| 20220815 | `allow-soldermask-bridges-in-FPs` bandera |
| 20220818 | Lazos de red de primera clase |
| 20220914 | Cajas de números de pad con forma personalizada |
| 20221018 | Vía y almohadilla de conexiones de capa de zona |

### Hoja de trabajo

| Versión | Cambiar |
| ---: | --- |
| 20220228 | Soporte de fuentes |

## 7,0 a 8,0

### Biblioteca de símbolos

| Versión | Cambiar |
| ---: | --- |
| 20230620 | `ki_description` cambió al campo `Description` |
| 20231120 | `generator_version` y limpieza de V8 |

### Esquemático

| Versión | Cambiar |
| ---: | --- |
| 20230221 | Símbolos de poder modernos, valor editable = neto |
| 20230409 | `exclude_from_sim` marcado |
| 20230620 | `ki_description` cambió al campo `Description` |
| 20230808 | El campo `Sim.Enable` se movió al atributo `exclude_from_sim` |
| 20230819 | Múltiples niveles de herencia de símbolos de biblioteca |
| 20231120 | `generator_version` y limpieza de V8 |

### PCB / Huella

| Versión | Cambiar |
| ---: | --- |
| 20230410 | Atributo DNP propagado desde el esquema a `attr` |
| 20230517 | Pad y mediante parámetros de lágrima |
| 20230620 | campos de PCB |
| 20230730 | Conectividad de formas gráficas |
| 20230825 | Bandera de borde explícito de cuadro de texto |
| 20230906 | Soporte para múltiples tipos de imágenes |
| 20230913 | Plantillas de radios con forma personalizada |
| 20231007 | Objetos generativos |
| 20231014 | Normalización del formato de archivo V8 |
| 20231212 | Bloqueo de imagen de referencia/UUID, formato booleano de huella |
| 20231231 | Los generadores y grupos usan `uuid` en lugar de `id` |
| 20240108 | Los parámetros de lágrima cambiaron a booleanos explícitos |

### Hoja de trabajo

| Versión | Cambiar |
| ---: | --- |
| 20230607 | Imágenes guardadas como base64 |
| 20231118 | Limpieza de formatos de archivos `generator_version` y V8 |

## 8,0 a 9,0

### Biblioteca de símbolos

| Versión | Cambiar |
| ---: | --- |
| 20240529 | Archivos incrustados |
| 20240819 | El algoritmo hash de archivos incrustados cambió a Murmur3 |
| 20241209 | `SCH_FIELD` banderas privadas |

### Esquemático

| Versión | Cambiar |
| ---: | --- |
| 20240101 | Mesas |
| 20240417 | Áreas de reglas |
| 20240602 | Atributos de hoja |
| 20240620 | Archivos incrustados |
| 20240716 | Múltiples asignaciones de clase de red |
| 20240812 | Resaltado de color Netclass |
| 20240819 | El algoritmo hash de archivos incrustados cambió a Murmur3 |
| 20241004 | El símbolo `hide` usa valores booleanos |
| 20241209 | `SCH_FIELD` banderas privadas |
| 20250114 | Las referencias cruzadas de variables de texto utilizan rutas completas |

### PCB / Huella

| Versión | Cambiar |
| ---: | --- |
| 20240201 | Las anulaciones utilizan propiedades que aceptan valores NULL |
| 20240202 | Mesas |
| 20240225 | `solder_paste_margin` racionalización |
| 20240609 | `tenting` palabra clave |
| 20240617 | Ángulos de mesa |
| 20240703 | Tipos de capas de usuario |
| 20240706 | Archivos incrustados |
| 20240819 | El algoritmo hash de archivos incrustados cambió a Murmur3 |
| 20240928 | Clases de componentes |
| 20240929 | Pilas de blocs complejas |
| 20241006 | A través de pilas |
| 20241007 | Las pistas pueden llevar una capa y un margen de máscara de soldadura. |
| 20241009 | Evolución del formato del área de reglas de ubicación |
| 20241010 | Las formas gráficas pueden llevar una capa y un margen de máscara de soldadura. |
| 20241030 | Direcciones de flecha de dimensión, normalización `suppress_zeroes` |
| 20241129 | `keep_text_aligned` normalizado y propiedades de relleno |
| 20241228 | Los puntos de la curva de lágrima cambiaron a booleanos |
| 20241229 | Capas de usuario ampliadas a un recuento arbitrario |

### Hoja de trabajo

Sin aumento de versión de la hoja de cálculo; permanece `20231118`.

## 9,0 a 10,0

### Biblioteca de símbolos

| Versión | Cambiar |
| ---: | --- |
| 20250318 | `~` ya no significa texto vacío |
| 20250324 | Grupos de pines de puente |
| 20250829 | Rectángulos redondeados |
| 20250901 | Notación de pines apilados |
| 20250925 | Alias ​​de bus en el archivo del proyecto |
| 20251024 | Actualizaciones de formato de propiedad: `do_not_autoplace`, `show_name` |

### Esquemático

| Versión | Cambiar |
| ---: | --- |
| 20250222 | Rellenos rayados para formas |
| 20250227 | Símbolos de poder locales |
| 20250318 | `~` ya no significa texto vacío |
| 20250425 | UUID para tablas |
| 20250513 | Los grupos pueden llevar el bloque de diseño `lib_id` |
| 20250610 | Las áreas de reglas admiten DNP y otras banderas |
| 20250827 | Estilos de carrocería personalizados |
| 20250829 | Rectángulos redondeados |
| 20250901 | Notación de pines apilados |
| 20250922 | Variantes esquemáticas |
| 20251012 | Soporte de jerarquía esquemática plana |
| 20251028 | Actualizaciones de formato de propiedad: `do_not_autoplace`, `show_name` |
| 20260101 | Variantes de PCB |
| 20260306 | Semántica de la variante `in_bom` corregida |

### PCB / Huella

| Versión | Cambiar |
| ---: | --- |
| 20250210 | Eliminación del cuadro de texto |
| 20250222 | Eclosión de formas de PCB |
| 20250228 | IPC-4761 mediante funciones de protección |
| 20250302 | Desplazamientos de sombreado de zonas |
| 20250309 | Reglas de asignación dinámica de clases de componentes |
| 20250324 | Almohadillas para puentes |
| 20250401 | Ajuste de la longitud del dominio del tiempo |
| 20250513 | Los grupos pueden llevar el bloque de diseño `lib_id` |
| 20250801 | `(island)` cambió a `(island yes/no)` |
| 20250811 | Propiedad de fabricación de almohadillas de ajuste a presión |
| 20250818 | Las huellas admiten recuentos de capas personalizados |
| 20250829 | Rectángulos redondeados |
| 20250901 | puntos de PCB |
| 20250907 | UUID para tablas |
| 20250909 | Metadatos de la unidad de huella: unidades/pines |
| 20250914 | `PCB_BARCODE` objetos |
| 20250926 | Vía tipos divididos en ciego/enterrado/a través |
| 20251027 | Corrección de escalado de retrasos de pad-to-die |
| 20251028 | Dejó de escribir códigos de red; son detalles de implementación interna |
| 20251101 | Soporte de retroperforación y perforación terciaria |
| 20260101 | Variantes de PCB con anulaciones por huella |
| 20260206 | Correcciones de serialización de atributos de variantes y códigos de barras |

### Hoja de trabajo

Sin aumento de versión de la hoja de cálculo; permanece `20231118`.

## 10.0 al desarrollo actual

En comparación con los archivos de destino KiCad 10, la rama de desarrollo actual revisada
agrega estos pasos de formato más nuevos:

| Versión | tipo de archivo | Diferencia |
| ---: | --- | --- |
| 20260410 | Board / footprint | Metadatos corporales 3D extruidos en bloques de modelo de huella |
| 20260508 | Board / footprint | Primitivas nativas de elipse y arco de elipse de PCB |
| 20260508 | Esquema / símbolo | Primitivas nativas de elipse esquemática/símbolo y arco de elipse |
| 20260511 | Junta | Campos del modelo de apilamiento dependiente de la frecuencia dieléctrica |
| 20260512 | Junta | Bloque de agregación de cadena neta |
| 20260512 | Esquemático | Registros de cadena neta |
| 20260513 | Junta | Modo de llenado de la zona de robo de cobre |
| 20260521 | Board / footprint | Tipo eléctrico de simulación de pad, serializado como `sim_electrical_type` en pads |
| 20260603 | Board / footprint | Indicador `knockout` de celda de tabla |

## Resumen de destino del backport a partir de archivos de desarrollo actuales

En comparación con los objetivos admitidos más antiguos, 10.99 introduce o conserva nuevos
construcciones que deben eliminarse, simplificarse o cambiarse de nombre al realizar la copia de seguridad:

| Objetivo | Objetivo de tablero/huella | Objetivo esquemático | Objetivo del símbolo | Principales áreas de degradación del desarrollo actual |
| --- | ---: | ---: | ---: | --- |
| KiCad 10 | `20260206` | `20260306` | `20251024` | Elimine los metadatos del cuerpo extruido exclusivos para el desarrollo, las elipses nativas, los campos de frecuencia dieléctrica, las cadenas de redes, el robo de cobre, los tipos eléctricos de simulación de almohadillas y las banderas eliminatorias de las celdas de la tabla. |
| KiCad 9 | `20241229` | `20250114` | `20241209` | KiCad 10 elementos más sombreado de forma de PCB, protección de vía, compensaciones de sombreado de zona, almohadillas de puente, ID de bloques de diseño de grupo, recuentos de capas personalizadas, rectángulos redondeados, puntos de PCB, UUID de tabla, códigos de barras, tipos de vía dividida, omisión de código de red, backdrill/post-mecanizado, variantes de PCB, variantes esquemáticas/estilos de cuerpo/rectángulos redondeados/pines apilados/formato de propiedades |
| KiCad 8 | `20240108` | `20231120` | `20231120` | Elementos de KiCad 9 más tablas, archivos incrustados, clases de componentes, padstacks, via stacks, áreas de reglas, tenting, expansión de capa de usuario, atributos de hoja, múltiples asignaciones de netclass, resaltado de color de netclass |
| KiCad 7 | `20221018` | `20230121` | `20220914` | KiCad 8 elementos más campos de PCB, propagación de atributos DNP, lágrimas modernas, plantillas de radios de almohadilla personalizadas, generadores, normalización de UUID/id, cuadros de texto, imágenes, vínculos de red, formato de fuente/campo, áreas de reglas, simulación esquemática moderna/banderas de exclusión |

## Cobertura de implementación del backport de C++

La CLI `kicad-backport-cplus` implementa reescrituras de expresiones S basadas en versiones.
Resuelve alias de publicación por tipo de documento, aplica reglas de degradación y luego
escribe el campo `version` de destino. También acepta un formato de archivo numérico sin formato.
versión para probar un límite de analizador específico.

Asignaciones de alias admitidas en el código:

| Alias | Símbolo | Esquemático | Junta | Huella | Hoja de trabajo | Reglas de diseño |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |
| 10.99 | 20251024 | 20260306 | 20260603 | 20260603 | 20231118 | 20200610 |

Si el archivo fuente ya tiene exactamente la versión numérica solicitada, el
El convertidor lo copia sin cambios. Si la versión de origen es inferior a la de destino,
la implementación de C++ ahora aplica actualizaciones de compatibilidad limitadas antes
escribiendo el campo `version` solicitado:

| Amable | Normalización de actualización implementada |
| --- | --- |
| Biblioteca de símbolos | Ampliar los átomos de estilo de fuente heredados para objetivos modernos; ampliar la visibilidad de los átomos; mover la propiedad `hide` fuera de `effects`; eliminar los ID de propiedad heredados. |
| Esquemático | Cambie el nombre de `tstamp` a `uuid`; cambie el nombre de `netclass_flag` a `directive_label`; convertir el antiguo cuadro de texto `start/end` a `at/size`; ampliar la fuente heredada y los átomos de visibilidad de pines; trasladar la propiedad `hide` fuera de `effects`; eliminar los ID de propiedad heredados. |
| Board / footprint | Cambie el nombre de `tstamp` a `uuid` para objetivos modernos; expandir átomos de estilo de fuente; ampliar la huella `dnp` átomos; normalizar valores booleanos al estilo KiCad 7 `yes/no`; eliminar `tedit` obsoleto; Opcionalmente, convierta referencias netas numéricas heredadas en nombres de redes. |

Este no es un motor de actualización semántica completo; sólo normaliza la sintaxis
El convertidor ya sabe expresar.

### Cobertura de objetivo de lanzamiento implementada

Las reglas de C++ están controladas por límites, por lo que cada alias de versión activa las reglas cuyo
los cortes son más recientes que la versión de destino de esa familia de archivos. El siguiente resumen
enumera la cobertura práctica para los objetivos estables que no son V6.

#### Objetivo KiCad 10

Los objetivos de KiCad 10 eliminan en su mayoría construcciones posteriores a 10.0/desarrollo actual:

| Amable | Manejo implementado |
| --- | --- |
| Biblioteca de símbolos | Elimine las primitivas nativas de elipse y arco de elipse introducidas después del objetivo del símbolo 10.0. |
| Esquemático | Elimine los campos `locked` posteriores a 10.0, las primitivas de elipse nativas y `net_chain`/`net_chains`. |
| Board / footprint | Elimine o degrade los bloques de modelo extruidos/mecanografiados posteriores a 10.0, las primitivas de elipse nativas, los campos de acumulación de frecuencia dieléctrica, las cadenas de red, el modo de relleno de robo de cobre, el pad `sim_electrical_type` y la celda de tabla `knockout`. |
| Archivos laterales del proyecto | No se genera ninguna reescritura de compatibilidad de tabla de biblioteca o `.kicad_prl` heredado para el sufijo V10. |

#### Objetivo KiCad 9

Los objetivos de KiCad 9 eliminan KiCad 10 y la sintaxis de desarrollo actual manteniendo
características que son válidas en las versiones de archivos KiCad 9:

| Amable | Manejo implementado |
| --- | --- |
| Biblioteca de símbolos | Elimine grupos de pines de puente, rectángulos redondeados, elipses nativas, símbolos `in_pos_files`, `duplicate_pin_numbers_are_jumpers`, `power` indicadores de clase, propiedad `show_name` / `do_not_autoplace` y fuente `face`. |
| Esquemático | Elimine rectángulos redondeados, variantes esquemáticas, elipses nativas, cadenas de red, `locked`, `embedded_fonts` posteriores al objetivo, estilos de cuerpo personalizados, indicadores de simulación/ensamblaje de hojas, símbolo `in_pos_files`, indicadores de clase de potencia/puente, fuente `face`, campos de formato de propiedad y nodos raíz `group`. |
| Board / footprint | Eliminar o degradar IPC-4761 mediante protección, campos de puentes, fuentes de ubicación de clases de componentes, rellenos de sombreado de PCB, recuentos de capas personalizados, rectángulos redondeados, objetos puntuales de PCB, códigos de barras, campos de retroperforación/postmecanizado, variantes de PCB, características de desarrollo actual y fuente `face`; reconstruir códigos de red de tableros numéricos heredados. Tenting se degrada de listas frontales/posteriores booleanas a átomos heredados para este rango objetivo. |
| Archivos laterales del proyecto | No se genera ninguna reescritura de `.kicad_prl` heredado para V9. |

#### Objetivo KiCad 8

Los objetivos de KiCad 8 eliminan la sintaxis actual de KiCad 9/10 y también normalizan varios
Los formularios de desarrollo tardío de KiCad-8 vuelven a las versiones de archivos 8.0.0:

| Amable | Manejo implementado |
| --- | --- |
| Biblioteca de símbolos | Elimine los archivos incrustados/campos privados de V9+ y la sintaxis de puente, rectángulo redondeado y elipse de V10+; eliminar `embedded_fonts`, fuente `face`, campos de formato de símbolo/propiedad; agregar ID de propiedad heredadas y mover la visibilidad de la propiedad a `effects`; convierta el estilo de fuente y los valores booleanos de visibilidad de pin a una sintaxis atómica más antigua. |
| Esquemático | Elimine las tablas V9+, áreas de reglas, archivos incrustados/campos privados y la sintaxis de rectángulo redondeado, variante, estilo de cuerpo y elipse/cadena de red V10+; eliminar banderas de simulación/ensamblaje de texto y hoja, campos de formato de símbolo/propiedad, fuente `face`; agregar ID de propiedad heredadas y mover la visibilidad de la propiedad a `effects`; convertir valores booleanos de visibilidad de fuente y pin a una sintaxis atómica más antigua; eliminar los nodos raíz `group`. |
| Board / footprint | Elimine tablas V9+, tenting, archivos/fuentes incrustados, clases de componentes, padstacks complejos, pilas de vía, áreas de reglas, protección de vía, calificadores arbitrarios de capa de usuario, recuentos de capas personalizados, rectángulos redondeados, puntos de PCB, códigos de barras, taladrado posterior/postmecanizado, variantes y características de desarrollo actual. También elimine los campos de capa/margen de máscara de soldadura de pista/gráfico, ángulo de celda de tabla, cachés de representación de texto, cuadro de texto/celda de tabla/eliminación de capa, modelo `hide`, fuente `face` y agregue códigos de red numéricos heredados. `solder_paste_margin_ratio` pasa a llamarse `solder_paste_ratio`. |
| Archivos laterales del proyecto | Genere configuraciones de visualización de ID numérico heredadas `.kicad_prl` para placas V8. |

#### Objetivo KiCad 7

Los objetivos de KiCad 7 eliminan la sintaxis actual de KiCad 8/9/10 y aplican un analizador adicional
reescrituras de compatibilidad en torno a campos de PCB, UUID y datos de huellas:

| Amable | Manejo implementado |
| --- | --- |
| Biblioteca de símbolos | Eliminar V8+ `generator_version`, fuentes/archivos incrustados, campos privados V9, sintaxis de puente/redondeado/elipse V10, símbolo `exclude_from_sim`, archivos de posición y campos de formato de propiedad, puentes/indicadores de clase de potencia y fuente `face`; agregar ID de propiedades heredadas; mover la visibilidad de la propiedad a `effects`; convierta los valores booleanos de fuente y visibilidad a sintaxis atómica. |
| Esquemático | Elimine V8+ `generator_version` y `fields_autoplaced`, tablas V9+/áreas de reglas/campos incrustados/privados, sintaxis redondeada/variante/estilo de cuerpo V10+, campos de exclusión de simulación posterior al objetivo, indicadores de simulación/ensamblaje de hojas, campos de formato de símbolo/propiedad, fuente `face` y nodos raíz `group`. Los átomos de UUID no están entrecomillados para los analizadores KiCad 6/7, y la visibilidad/ID de las propiedades se degradan a la ubicación heredada. |
| Board / footprint | Elimine objetos generados en V8+, lágrimas, tablas, archivos/fuentes incrustados, clases de componentes, pilas de pad/vía, áreas de reglas, protección de vía y sintaxis de destino más nueva. Convertir calificadores de tipo de capa de usuario a `user`; elimine campos de máscara de soldadura de gráficos/pistas, ángulos de mesa, cachés de renderizado, indicadores de eliminación, modelo `hide`, conectividad de red gráfica, campos bloqueados de grupo, campos de conexión a través de capas, campos de puente de huella/conexión de red/unidad, fuente `face` y átomos de huella incompatibles con el legado. Convierta las propiedades de la huella de PCB nuevamente a `fp_text`, cambie el nombre de `uuid`/`id` nuevamente a `tstamp`/`id` formas heredadas, cambie el nombre de campos térmicos y de soldadura en pasta, convierta trazos a `width` heredado, convierta dimensiones en gráficos visibles, degrade valores booleanos/átomos de presencia y reconstruya códigos de red numéricos. |
| Archivos laterales del proyecto | Genere configuraciones de visualización de ID numérico heredadas `.kicad_prl` para placas V7. |

### Detección de documentos y manejo de proyectos

La implementación de C++ detecta el tipo de documento KiCad principalmente desde la raíz
Cabeza de expresión S:

| cabeza de raíz | Amable |
| --- | --- |
| `kicad_symbol_lib` | Biblioteca de símbolos |
| `kicad_sch` | Esquemático |
| `kicad_pcb` | Junta |
| `footprint` | Huella |
| `kicad_dru` | Reglas de diseño |
| `kicad_wks`, `drawing_sheet` | Hoja de trabajo |

Si falta el encabezado raíz o se desconoce, recurre a la extensión de archivo:
`.kicad_sym`, `.kicad_sch`, `.kicad_pcb`, `.kicad_mod`, `.kicad_dru` y
`.kicad_wks`. Legacy `.sch`, `.lib`, `.dcm`, and `.pro` are also detected as
tipos de KiCad heredados, pero la conversión directa de esas familias de archivos heredados no es
implementado en la fase actual.

Al convertir un directorio de proyecto o `.kicad_pro`, copia solo editable
Entradas de proyectos KiCad y archivos de modelos 3D locales comunes. Salidas generadas,
Carpetas de historial/copia de seguridad, Gerbers, resultados de fabricación, listas de materiales y archivos temporales.
se omiten. Para los objetivos de placa KiCad 6, 7 y 8, también crea un legado
`.kicad_prl` configuración de visualización de la placa local con `visible_items` numérico, completo
`visible_layers` y la metaversión anterior de configuración local para objetos convertidos
permanecen visibles en GUI más antiguas. Para el proyecto KiCad 6 se dirige adicionalmente
elimina los nodos `version` de nivel superior de `sym-lib-table` / `fp-lib-table` y
Reconstruye tablas de instancias de jerarquía esquemática de nivel raíz en hojas secundarias.

### Reglas de la biblioteca de símbolos

Las puertas genéricas del analizador eliminan estos nodos introducidos cuando el formato del archivo de destino
es anterior a la versión de introducción:

| Introducido | Cabezas eliminadas | Razón |
| ---: | --- | --- |
| 20220126 | `text_box`, `textbox` | Cuadros de texto de símbolos |
| 20240529 | `embedded_files`, `embedded_file` | Archivos incrustados |
| 20241209 | `private` | Marcas privadas de SCH_FIELD |
| 20250324 | `pin_group`, `pin_groups` | Grupos de pines de puente |
| 20250829 | `rounded_rectangle`, `roundrect` | Rectángulos redondeados |
| 20260508 | `ellipse`, `ellipse_arc` | Primitivas de elipse nativas |

Reescrituras de compatibilidad:

| límite objetivo | Volver a escribir |
| ---: | --- |
| `< 20231120` | Eliminar campos raíz `generator_version` |
| `< 20241209` | Eliminar `embedded_fonts`; agregar ID de propiedades heredadas; mover las banderas de propiedad `hide` a `effects` |
| `< 20230409` | Eliminar los indicadores de exclusión de simulación de la biblioteca de símbolos `symbol/exclude_from_sim` |
| `< 20240108` | Convierta las listas de fuentes `(bold yes/no)` y `(italic yes/no)` en átomos de presencia heredados |
| `<= 20241209` | Eliminar campos de fuente `face` |
| `< 20241004` | Convierta listas booleanas `hide` en átomos heredados; aplanar `pin_names` / `pin_numbers` ocultar listas |
| `<= 20211014` | Agregue ID de propiedad estándar de KiCad 6: `Reference=0`, `Value=1`, `Footprint=2`, `Datasheet=3`, `ki_keywords=4`, `ki_description=5`, `ki_fp_filters=6` |
| `< 20251024` | Eliminar el símbolo `in_pos_files`; eliminar la propiedad `show_name` y `do_not_autoplace` |
| `< 20250324` | Eliminar `duplicate_pin_numbers_are_jumpers` |
| `< 20250227` | Eliminar banderas de clase del símbolo `power` |

### Reglas esquemáticas

Puertas genéricas del analizador:

| Introducido | Cabezas eliminadas | Razón |
| ---: | --- | --- |
| 20220126 | `text_box`, `textbox` | Cuadros de texto esquemáticos |
| 20220622 | `simulation_model`, `sim_model` | Nuevo formato de modelo de simulación. |
| 20240101 | `table` | Tablas esquemáticas |
| 20240417 | `rule_area` | Áreas de reglas esquemáticas |
| 20240620 | `embedded_files`, `embedded_file` | Archivos incrustados |
| 20241209 | `private` | Marcas privadas de SCH_FIELD |
| 20250829 | `rounded_rectangle`, `roundrect` | Rectángulos redondeados |
| 20250922 | `variants`, `variant` | Variantes esquemáticas |
| 20260508 | `ellipse`, `ellipse_arc` | Primitivas de elipse nativas |
| 20260512 | `net_chain`, `net_chains` | Cadenas netas esquemáticas |

Reescrituras de compatibilidad:

| límite objetivo | Volver a escribir |
| ---: | --- |
| `< 20231120` | Eliminar `generator_version`; eliminar `fields_autoplaced` de símbolos y hojas |
| `< 20260326` | Eliminar los campos esquemáticos `locked` introducidos después del objetivo |
| `< 20260306` | Eliminar `embedded_fonts`; eliminar hoja `exclude_from_sim`, `in_bom`, `on_board`, `dnp`; eliminar los nodos `group` del esquema raíz |
| `< 20250827` | Eliminar `body_styles` y `body_style` |
| `< 20250114` | Eliminar texto/cuadro de texto `exclude_from_sim` |
| `<= 20230121` | Eliminar todos los `exclude_from_sim` restantes |
| `< 20220822` | Eliminar los campos de texto, etiqueta y etiqueta directiva `hyperlink` |
| `< 20220914` | Eliminar indicadores de símbolo colocado `dnp` |
| `< 20220124` | Cambie el nombre de los nodos raíz `directive_label` a `netclass_flag` |
| `< 20251024` | Eliminar símbolo `in_pos_files` |
| `< 20250324` | Eliminar `duplicate_pin_numbers_are_jumpers` |
| `< 20250227` | Eliminar banderas de clase del símbolo `power` |
| `< 20241004` | Convierta listas booleanas `hide` en átomos heredados; aplanar visibilidad de pin ocultar listas |
| `<= 20211123` | Eliminar definiciones de símbolo de biblioteca `pin/alternate` |
| `< 20240108` | Convierta listas booleanas en negrita/cursiva a átomos heredados |
| `<= 20250114` | Eliminar campos de fuente `face` |
| `<= 20230121` | Quite las comillas de los átomos `uuid` para los analizadores KiCad 6/7 |
| `<= 20211123` | Genere `sheet_instances` y `symbol_instances` de nivel raíz de KiCad 6 cuando el esquema raíz de origen ya tenga datos de instancia; las hojas secundarias no reciben tablas de instancia raíz |
| `<= 20211123` | Agregue ID de propiedades esquemáticas estándar de KiCad 6 y normalice los nombres/ID de propiedades de la hoja a `Sheet name=0` y `Sheet file=1` |
| `<= 20211123` | Elimine los bloques `instances` internos de símbolos después de que se haya generado la tabla de instancia raíz de KiCad 6 |
| `< 20241209` | Agregar ID de propiedades heredadas; mover las banderas de propiedad `hide` a `effects` |
| `< 20251028` | Eliminar propiedad `show_name` y `do_not_autoplace` |

### Reglas de tablero y huella

Puertas genéricas del analizador:

| Introducido | Cabezas eliminadas | Razón |
| ---: | --- | --- |
| 20220131 | `gr_text_box`, `fp_text_box`, `text_box`, `textbox` | cuadros de texto de PCB |
| 20220621 | `image` | Objetos de imagen de PCB |
| 20220818 | `net_tie`, `net_ties` | Almacenamiento de red de primera clase |
| 20231007 | `generated` | Objetos generativos |
| 20240108 | `teardrop`, `teardrops` | Parámetros de lágrima |
| 20240202 | `table` | mesas de PCB |
| 20240609 | `tenting` | Palabra clave de tienda de campaña |
| 20240706 | `embedded_files`, `embedded_file`, `embedded_fonts` | Archivos incrustados |
| 20240928 | `component_class`, `component_classes` | Clases de componentes |
| 20240929 | `padstack` | Pilas de blocs complejas |
| 20241006 | `via_stack`, `viastack` | A través de pilas |
| 20241009 | `rule_area` | Áreas de colocación/reglas |
| 20250228 | `via_protection`, `covering`, `plugging`, `filling`, `capping` | IPC-4761 vía protección |
| 20250818 | `custom_layer_count`, `custom_layer_counts` | Recuentos de capas de huellas personalizadas |
| 20250829 | `rounded_rectangle`, `roundrect` | Rectángulos redondeados |
| 20250901 | `point` | Objetos puntuales de PCB |
| 20250914 | `barcode`, `pcb_barcode`, `gr_barcode`, `fp_barcode` | Objetos de código de barras PCB |
| 20251101 | `backdrill`, `tertiary_drill`, `front_post_machining`, `back_post_machining` | Campos de perforación backdrill y terciario |
| 20260101 | `variants`, `variant` | Variantes de PCB |
| 20260410 | `extruded` | Modelos de cuerpo 3D de huella extruida |
| 20260508 | `gr_ellipse`, `gr_ellipse_arc`, `fp_ellipse`, `fp_ellipse_arc` | Primitivas de elipse de PCB nativas |
| 20260511 | `spec_frequency`, `dielectric_model` | Campos de apilamiento dependientes de la frecuencia dieléctrica |
| 20260512 | `net_chains`, `net_chain` | Cadenas de red de PCB |
| 20260513 | `thieving` | Modo de llenado de la zona de robo de cobre |

Notas de cobertura de desarrollo actual de KiCad local `10.99.0-1273-gd90e32b6a0`:

| Introducido | Manejo | Notas |
| ---: | --- | --- |
| 20260521 | Implementado | El elemento secundario `sim_electrical_type` se elimina para objetivos anteriores a `20260521`. |
| 20260603 | Implementado | El elemento secundario de celda de tabla `knockout` se elimina contextualmente para objetivos anteriores a `20260603`; `knockout` no se utiliza como puerta de token global porque otros tipos de objetos también lo utilizan. |

Reescrituras de compatibilidad:

| límite objetivo | Volver a escribir |
| ---: | --- |
| `< 20260410` | Elimine bloques de modelo 3D mecanografiados/extruidos eliminando los nodos `model` que contienen `type` |
| `< 20260513` | Reemplace el modo de relleno de la zona de robo de cobre con relleno de polígono |
| `>= 20220225` | Eliminar campos obsoletos de huella `tedit` |
| `>= 20200628` | Eliminar la configuración obsoleta del tablero `visible_elements` |
| `< 20260603` | Eliminar campos de celda de tabla de PCB `knockout` |
| `< 20240703` | Convertir calificadores de tipo de capa de usuario `front`, `back`, `auxiliary` a `user` |
| `< 20241010` | Eliminar campos gráficos `solder_mask_margin` |
| `< 20241030` | Convertir campos booleanos de dimensión en átomos heredados; eliminar dimensión `arrow_direction` |
| `< 20250210` | Eliminar texto de PCB `render_cache`; eliminar cuadro de texto `knockout`; eliminar `knockout` átomos de las listas de capas; agregue `filled_areas_thickness no` a los rellenos de zona en caché donde sea necesario |
| `< 20241009` | Eliminar campos de zona `placement` |
| `<= 20221018` | Eliminar zona `attr`; eliminar pad/zona `thermal_bridge_angle`; cambiar el nombre del área/zona `thermal_bridge_width` a heredado `thermal_width` |
| `< 20240108` | Eliminar `setup/allow_soldermask_bridges_in_footprints`; eliminar grupo `locked`; eliminar a través de campos de conexión de capas como `keep_end_layers`, `start_end_only` y `zone_layer_connections` |
| `< 20241007` | Eliminar los campos de seguimiento `solder_mask_margin` y `solder_mask_layer` |
| `< 20240617` | Retire la celda de la tabla de PCB `angle` |
| `< 20260521` | Retire la almohadilla `sim_electrical_type` |
| `< 20250228` | Convierta listas booleanas frontales/posteriores en átomos heredados; eliminar los campos de protección IPC-4761 |
| `< 20231212` | Convierta las listas booleanas `locked` y `hide` en átomos de presencia; eliminar `unlocked`; eliminar modelo `hide` |
| `< 20231014` | Eliminar `generator_version` |
| `< 20230924` | Convierta los valores booleanos `pcbplotparams` `yes/no` a `true/false`; convertir relleno de forma `no` a `none` |
| `< 20230730` | Eliminar la conectividad de la forma gráfica `net` |
| `< 20240108` | Convierta listas booleanas en negrita/cursiva a átomos heredados |
| `< 20230620` | Convierta las propiedades de huella `Reference` y `Value` nuevamente a `fp_text`; convertir `Description` a `ki_description`; asignar `sheetname`/`sheetfile` a propiedades |
| `< 20231231` | Cambie el nombre de los campos `uuid` con alcance a `tstamp`; cambiar el nombre del grupo/generado `uuid` de nuevo a `id` |
| `< 20250324` | Eliminar los campos del panel de puente de huella: `duplicate_pad_numbers_are_jumpers` y `jumper_pad_groups` |
| `<= 20221018` | Eliminar los atributos de huella `dnp`, `net_tie_pad_groups`, `units` y `allow_missing_courtyard`; quitar la almohadilla/vía `remove_unused_layers`; convertir dimensiones en gráficos visibles; eliminar `locked` incompatible con el legado; bajar de categoría gratis a través de campos; convertir bloques `stroke` gráficos de PCB en campos `width` heredados |
| `< 20250309` | Eliminar `component_class` de las reglas de ubicación |
| `< 20250222` | Convierta rellenos de forma de rayado/rayado inverso/rayado cruzado de PCB en relleno sólido |
| `<= 20241229` | Eliminar campos de fuente PCB `face` |
| `< 20251101` | Quitar pad/a través de campos de posmecanizado |
| `< 20251028` | Reconstruir códigos de red de tableros numéricos heredados y declaraciones de red de nivel raíz |

Correcciones específicas del analizador KiCad 6 observadas en pruebas a nivel de proyecto:

| Área | Solución implementada |
| --- | --- |
| configuración de PCB | Elimine `setup/allow_soldermask_bridges_in_footprints` para objetivos de tablero anteriores al 8. |
| Huellas de PCB | Elimine `net_tie_pad_groups`, `units`, grupos de puentes y átomos de atributos `allow_missing_courtyard` para los objetivos de la placa KiCad 6/7. |
| Zonas y pads de PCB | Elimine la zona `attr`, elimine `thermal_bridge_angle` y cambie el nombre de `thermal_bridge_width` a `thermal_width` para los objetivos de placa KiCad 6/7. |
| Texto y tablas de PCB | Elimine el texto `render_cache`, el cuadro de texto `knockout`, la celda de la tabla `knockout` y la lista de capas `knockout` donde los analizadores más antiguos los rechacen. |
| Bibliotecas de símbolos | Elimine el símbolo `exclude_from_sim` para objetivos anteriores a `20230409` y agregue ID de propiedad estándar de KiCad 6. |
| esquemas | Elimine el pin `alternate`, genere tablas de instancia raíz de KiCad 6, normalice las rutas de instancia de la hoja raíz, normalice los nombres/ID de propiedades de la hoja y elimine el `instances` interno del símbolo. Los bloques UUID de pin de símbolo colocados se conservan intencionalmente porque KiCad 6 los usa, por ejemplo, para la asociación. |
| Archivos laterales del proyecto | Genere la configuración de visualización de ID numérico `.kicad_prl` para V6/V7/V8 y elimine los nodos `version` de nivel superior de la tabla de biblioteca para V6. |

### Hoja de trabajo y reglas de diseño

El manejo de hojas de trabajo actualmente tiene una puerta de analizador implementada:

| límite objetivo | Volver a escribir |
| ---: | --- |
| `< 20220228` | Eliminar bloques de la hoja de trabajo `font` |

Se detectan reglas de diseño y tienen alias de versión de destino, pero no se degradan
Las reescrituras se implementan actualmente porque la macro de versión del formato de archivo permanece
`20200610` en las versiones de KiCad rastreadas.

### Semántica de advertencias e informes

Cada eliminación implementada o reescritura de compatibilidad que cambia el árbol agrega un
advertencia. Las puertas de características genéricas informan el número de nodos eliminados y el
versión de introducción. Los informes incluyen ruta, tipo detectado, versión fuente,
versión de destino, bandera modificada y advertencias.

## Requisitos del convertidor

### Leer ruta

- Conservar el archivo fuente `version`; no interpretar solo como la corriente
Formato KiCad.
- Alias ​​de compatibilidad de soporte para archivos más antiguos:
  - `page` a `paper`
  - Barra superior heredada de `~...~` a `~{...}`
  - Antiguo formato de cuadro de texto `start/end` al nuevo `at/size`
  - Antiguo `id` a `uuid`
  - Antiguos formatos booleanos/token de presencia a booleanos explícitos
- Detecta formatos futuros y devuelve un error claro o una estrategia de degradación definida.

### Escribir ruta

- `--target-version` debe hacer más que cambiar el número de versión de nivel superior. Él
debe podar o reescribir la semántica de acuerdo con el objetivo solicitado.
- Cada versión de destino necesita puertas de funciones:
  - KiCad 6 no debe escribir campos del modelo de simulación V7, DNP o cuadros de texto posteriores a V6
estructuras.
  - KiCad 7 no debe escribir estructuras que sólo se estabilizaron después de V8
`generator_version` limpieza.
  - KiCad 8 no debe escribir archivos integrados V9, clases de componentes ni archivos complejos.
pilas de blocs.
  - KiCad 9 no debe escribir variantes V10, códigos de barras, backdrill, división por tipo ni
construcciones similares.
  - KiCad 10 no debe escribir metadatos del cuerpo extruido de desarrollo actual, nativo
elipses, campos de frecuencia dieléctrica, cadenas de red, robo de cobre, almohadilla
tipos eléctricos de simulación o banderas eliminatorias de celdas de mesa.
- Las degradaciones con pérdida deberían producir advertencias o metadatos complementarios en lugar de silencio
supresión.

### Ruta de prueba

- Cree accesorios mínimos para KiCad 6, 7, 8, 9 y 10:
  - Biblioteca de símbolos
  - Esquemático
  - Junta
  - Huella
  - Hoja de trabajo
  - Reglas de diseño
- Cada conversión entre versiones debe verificar:
  - La versión fuente se lee correctamente.
  - El número de versión de destino está escrito correctamente
  - Los tokens no permitidos se eliminan o degradan
  - Se conserva la semántica clave.
  - Las advertencias cubren conversiones con pérdidas

## Notas de mantenimiento

Al agregar diferencias de versiones futuras:

1. Primero agregue o actualice la matriz de versiones.
2. Agregue una nueva sección de intervalo como `10.0 to 11.0` o
   `10.99 / current to 11.99 / current`.
3. Mantenga los hallazgos de la rama de desarrollo separados de las etiquetas estables publicadas hasta que
La versión KiCad correspondiente está etiquetada.
4. Actualizar el resumen de destino del backport cuando se introduce una nueva versión fuente
construcciones que afectan los objetivos de degradación existentes.
5. Realice un seguimiento de las migraciones de esquemas JSON de `.kicad_pro` en un documento separado.

# Diferencias de formato y comportamiento de conversión de KiCad Backport

Este documento describe las diferencias de formato KiCad que la implementación actual de `kicad-backport` trata realmente: familias de archivos, anclas de versión, despacho de conversión, reglas de reescritura y rutas con pérdida. La referencia normativa de formatos KiCad es la documentación para desarrolladores de KiCad.

- https://dev-docs.kicad.org/en/file-formats/index.html

## Lectura rápida para desarrolladores de KiCad

| Tema | Comportamiento del conversor |
| --- | --- |
| Detección de formato | Los archivos modernos se identifican primero por el root token S-expression: `kicad_sch`, `kicad_symbol_lib`, `kicad_pcb`, `footprint`, `kicad_wks` / `drawing_sheet`. La extensión es fallback. |
| Detección de versión | Los S-expression modernos leen el campo top-level `(version ...)`. `.kicad_pro` se informa como `kicad-project-json`. Los legacy `.sch/.lib/.dcm/.pro` usan alias legacy. |
| Frontera KiCad 5/6 | KiCad 6 es la frontera de familia para esquemas, bibliotecas de símbolos y proyectos. KiCad 4/5 `.sch/.lib/.pro` y KiCad 6+ `.kicad_sch/.kicad_sym/.kicad_pro` no comparten sintaxis. |
| PCB y huellas | Boards y footprints KiCad 4-10 se tratan como S-expressions. Las diferencias importantes son version anchors, conjuntos de nodos y sintaxis de campos. |
| `.kicad_pro` | El JSON de proyecto moderno no se reescribe por versión mayor objetivo. Para KiCad 6+ normalmente se copia en bruto; para KiCad 5/4 se genera un `.pro` mínimo. |
| `.kicad_wks` | Las worksheets se detectan y pueden reescribir su version. Solo hay una regla worksheet downgrade pequeña y no existe writer legacy KiCad 4/5. |
| `.kicad_dru` | El código puede detectarlo y usa el anchor fijo `20200610`, pero no forma parte de la matriz principal de formatos visibles al usuario. |

## Modelo de implementación

| Etapa | Implementación | Significado |
| --- | --- | --- |
| Lectura | `loadDocumentImpl()` lee texto, separa `.kicad_pro` y legacy por extensión y parsea el resto como S-expression. | Evita tratar JSON o texto legacy como S-expression. |
| Tipo | `DetectKind()` prefiere root token y usa extensión como fallback. | Un S-expression con root correcto puede manejarse aunque el nombre sea atípico. |
| Objetivo | `ResolveTargetVersion()` mapea cada alias KiCad a una versión por tipo de archivo. | Una versión KiCad no usa una única versión de formato para todos los archivos. |
| Extensión de salida | `withTargetFamilyExtension()` cambia `.sch/.lib/.pro` y `.kicad_sch/.kicad_sym/.kicad_pro` en la frontera KiCad 5/6. | La conversión 5/6 no es un simple cambio de `(version ...)`. |
| Misma versión | Si source y target S-expression tienen la misma versión, se copia o queda sin cambios. | Evita churn de formato innecesario. |
| Upgrade | Si la fuente es más antigua que el objetivo, `ApplyUpgradeRules()` normaliza sintaxis de forma conservadora. | No inventa nueva intención de diseño. |
| Downgrade | Si la fuente es más nueva que el objetivo, `ApplyDowngradeRules()` elimina, renombra, aplana o aproxima. | Los parsers KiCad antiguos no ven nodos desconocidos. |

## Version anchors objetivo

| Objetivo | Symbol library | Schematic | Board | Footprint | Worksheet | Notas |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `4.0` | legacy `.lib` 2.3 | legacy `.sch` V2 | `4` | `4` | indefinido | Schematic/symbol usan writers legacy. |
| `5.0` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | indefinido | Mismo anchor PCB/footprint que `5.1`. |
| `5.1` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | indefinido | Igual a `5.0`. |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | Inicio de familias modernas schematic/symbol. |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | Extensiones S-expression modernas. |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | Frontera de `generator_version`, UUID/id y PCB fields. |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | Embedded data, tables, rule areas y objetos PCB complejos. |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | Mayor alias objetivo regular soportado por el código. |
| `10.99` | `20251024` | `20260306` | `20260603` | `20260603` | `20231118` | Alias de desarrollo; solo board/footprint avanzan respecto a `10.0`. |

## Conversión por familia

| Archivo | Comportamiento | Límite |
| --- | --- | --- |
| `.kicad_pro` | Copia JSON bruta para KiCad 6+, `.pro` mínimo para KiCad 5/4. | No reescribe JSON por versión mayor objetivo. |
| legacy `.pro` | Genera `.kicad_pro` JSON mínimo para KiCad 6+. | Conserva solo settings legacy y nombres de bibliotecas reconocidos. |
| `.kicad_sch` | Writer legacy `.sch` para KiCad 5/4; reglas S-expression para KiCad 6+. | Propiedades, instancias y objetos modernos complejos son con pérdida en legacy. |
| legacy `.sch` | Convierte a `.kicad_sch` para KiCad 6+; reescribe cabecera para KiCad 5/4. | El dibujo legacy no-wire no se mapea por completo. |
| `.kicad_sym` | Escribe `.lib` y `.dcm` para KiCad 5/4. | Propiedades, gráficos y símbolos anidados modernos se aproximan. |
| legacy `.lib/.dcm` | Genera `.kicad_sym` para KiCad 6+. | `.dcm` solo se convierte en esqueleto de símbolos; metadata documental incompleta. |
| `.kicad_pcb/.kicad_mod` | Permanece S-expression; reescribe version, nodes y fields. | Campos no soportados de geometry/electrical/manufacturing/cache se eliminan o aproximan. |
| `.kicad_wks` | Version rewrite y regla worksheet limitada para KiCad 6+. | No hay writer worksheet legacy KiCad 4/5. |

## Principios de downgrade

| Caso | Decisión de implementación | Ejemplos |
| --- | --- | --- |
| El parser objetivo no lee un nodo | Eliminar nodo/campo y emitir warning. | `embedded_files`, `variants`, `barcodes`, `net_chains`, native ellipse. |
| Hay representación antigua | Renombrar, aplanar o usar campo legacy. | `directive_label -> netclass_flag`, `stroke -> width`, `uuid/tstamp/id`. |
| Geometría más débil | Convertir a primitivas simples. | PCB rectangles a lines, track arcs a segments, custom pads a rectangular pads. |
| Layout antiguo de propiedades | Mover propiedades, añadir o quitar IDs. | Ubicación de property hide, standard property id. |
| La fuente no contiene nueva semántica | No crear objetos de nuevas funciones. | No genera padstacks, variants, component classes ni barcodes. |

## Conversión de directorio de proyecto

| Tratamiento | Implementación |
| --- | --- |
| Entrada | Un directorio o `.kicad_pro` se trata como project tree. |
| Copia | Documentos KiCad, documentos legacy, library tables, `.kicad_prl`, modelos 3D. |
| Omite | VCS, history, backup, archive, gerber/fabrication/output/plot/BOM/assembly/vendor output. |
| Extensiones | KiCad 5/4 mapea `.kicad_sch/.kicad_sym/.kicad_pro` a `.sch/.lib/.pro`; KiCad 6+ hace lo inverso. |
| `.dcm` | En KiCad 6+, si existe `.lib` pareado, `.dcm` no se convierte por separado. |
| Library tables | `sym-lib-table` / `fp-lib-table` se normalizan para la familia objetivo. |
| Soporte schematic | En KiCad 6+, símbolos locales se embeben en `lib_symbols` y se reconstruyen hierarchy instances. |


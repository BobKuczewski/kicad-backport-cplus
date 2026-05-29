# Diferencias de versiones del formato de archivo KiCad

Este documento es la versión en español del [documento de referencia en inglés](kicad-file-format-version-differences.md). Resume las versiones de formato usadas por KiCad Backport C++, las diferencias principales y las reglas de conversión hacia atrás ya implementadas. Los tokens de KiCad, macros de código, extensiones y números de versión se mantienen en inglés para facilitar la búsqueda en el código.

Última actualización: 2026-05-29.

## Fuentes

- Tags oficiales de KiCad y cabeceras de versión.
- Checkout local de KiCad `master`: `10.99.0-1153-g8c1d374f29`.
- Implementación `kicad-backport-cplus`: `kicad_backport_versions.cpp`, `kicad_backport_rules.cpp`, `kicad_backport_rule_rewriters.cpp`, `kicad_backport.cpp`.

Las versiones proceden de `SEXPR_SYMBOL_LIB_FILE_VERSION`, `SEXPR_SCHEMATIC_FILE_VERSION`, `SEXPR_BOARD_FILE_VERSION`, `SEXPR_WORKSHEET_FILE_VERSION` y `DRC_RULE_FILE_VERSION`.

## Matriz de versiones estables

| KiCad tag | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

La versión S-expression de board también cubre `.kicad_mod`. `.kicad_dru` permanece en `20200610` dentro del rango analizado; eso no implica que la semántica de las reglas no haya cambiado. `.kicad_pro` es JSON y se gestiona mediante migraciones de esquema.

## 10.99 / current

| Tipo de archivo | Versión 10.99/current | Notas |
| --- | ---: | --- |
| Board `.kicad_pcb` | `20260513` | Copper thieving zone fill mode |
| Footprint `.kicad_mod` | `20260513` | Usa la versión S-expression de PCB |
| Schematic `.kicad_sch` | `20260512` | Net chains |
| Symbol library `.kicad_sym` | `20260508` | Native ellipse primitive |
| Worksheet `.kicad_wks` | `20231118` | Sin bump tras la limpieza de KiCad 8 |
| Design rules `.kicad_dru` | `20200610` | No se encontró bump específico de 10.99 |

Las principales novedades después de KiCad 10 son extruded 3D body metadata, native ellipses, dielectric frequency fields, net chains y copper thieving.

## Versiones objetivo en la implementación C++

| Alias | Symbol | Schematic | Board | Footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |

El convertidor no actualiza archivos antiguos hacia versiones más nuevas. Si la versión fuente es menor que el objetivo, copia el archivo sin modificar y conserva la versión original en el informe.

## Reglas principales de conversión hacia atrás

Symbol library / schematic:

- Elimina nodos que los parsers antiguos no aceptan, como `text_box`, embedded files, private flags, rounded rectangles, native ellipses, schematic variants y net chains.
- Añade legacy property IDs y mueve property `hide` de vuelta a `effects`.
- Convierte listas booleanas `hide` y font `bold` / `italic` a presence atoms.
- Elimina según el objetivo `generator_version`, `embedded_fonts`, font `face`, `show_name`, `do_not_autoplace`, power class flags y jumper pin flags.

Board / footprint:

- Elimina o convierte text boxes, images, net ties, generated objects, teardrops, tables, tenting, embedded files, component classes, padstacks, via stacks, rule areas, via protection, custom layer counts, rounded rectangles, points, barcodes, backdrill, variants, extruded models, native ellipses, dielectric frequency fields, net chains y copper thieving.
- Convierte copper thieving fill mode a polygon fill.
- Convierte propiedades footprint `Reference` / `Value` de vuelta a legacy `fp_text`.
- Renombra `uuid` aplicables a `tstamp` y group/generated `uuid` a `id`.
- Convierte PCB dimensions incompatibles con KiCad 7 en anotaciones de texto visibles.
- Reconstruye numeric netcodes y root-level net declarations para boards antiguos.

Worksheet elimina `font` blocks para objetivos anteriores a `20220228`. Design rules se detecta y se resuelve a una versión objetivo, pero todavía no tiene reglas de rewrite dedicadas.

## Informes y mantenimiento

Cada eliminación o rewrite de compatibilidad que modifica el árbol produce un warning. Los informes incluyen path, kind, source version, target version, changed y warnings. Para añadir nuevas versiones de KiCad, primero se debe actualizar la matriz y después documentar por separado las diferencias de stable tag y development branch.

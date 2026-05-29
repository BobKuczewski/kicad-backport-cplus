# KiCad-Dateiformat-Versionsunterschiede

Dies ist die deutsche Fassung des [englischen Referenzdokuments](kicad-file-format-version-differences.md). Sie beschreibt die Dateiformatversionen, die KiCad Backport C++ verwendet, die wichtigsten Unterschiede und die bereits implementierten Downgrade-Regeln. KiCad-Tokens, Quellmakros, Dateiendungen und Versionsnummern bleiben im Original, damit sie im Code auffindbar bleiben.

Zuletzt aktualisiert: 2026-05-29.

## Quellen

- Offizielle KiCad-Tags und Versions-Header.
- Lokaler KiCad-`master`: `10.99.0-1153-g8c1d374f29`.
- Implementierung in `kicad-backport-cplus`: `kicad_backport_versions.cpp`, `kicad_backport_rules.cpp`, `kicad_backport_rule_rewriters.cpp`, `kicad_backport.cpp`.

Die Versionsnummern stammen aus `SEXPR_SYMBOL_LIB_FILE_VERSION`, `SEXPR_SCHEMATIC_FILE_VERSION`, `SEXPR_BOARD_FILE_VERSION`, `SEXPR_WORKSHEET_FILE_VERSION` und `DRC_RULE_FILE_VERSION`.

## Matrix stabiler Versionen

| KiCad tag | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

Die Board-S-expression-Version gilt auch für `.kicad_mod`. `.kicad_dru` bleibt in den betrachteten Versionen bei `20200610`; das bedeutet nicht, dass sich die Regel-Semantik nie geändert hat. `.kicad_pro` ist JSON und wird über Schema-Migrationen behandelt.

## 10.99 / current

| Dateityp | 10.99/current version | Hinweise |
| --- | ---: | --- |
| Board `.kicad_pcb` | `20260513` | Copper thieving zone fill mode |
| Footprint `.kicad_mod` | `20260513` | Nutzt die PCB-S-expression-Version |
| Schematic `.kicad_sch` | `20260512` | Net chains |
| Symbol library `.kicad_sym` | `20260508` | Native ellipse primitive |
| Worksheet `.kicad_wks` | `20231118` | Kein Bump nach KiCad-8-Cleanup |
| Design rules `.kicad_dru` | `20200610` | Kein 10.99-spezifischer Bump gefunden |

Wichtige Neuerungen nach KiCad 10 sind extruded 3D body metadata, native ellipses, dielectric frequency fields, net chains und copper thieving.

## Zielversionen der C++-Implementierung

| Alias | Symbol | Schematic | Board | Footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |

Der Konverter aktualisiert alte Dateien nicht nach oben. Wenn die Source-Version kleiner als das Ziel ist, wird die Datei unverändert kopiert und die ursprüngliche Version im Report behalten.

## Wichtige Downgrade-Regeln

Symbol library / schematic:

- Entfernt Knoten, die ältere Parser nicht akzeptieren, darunter `text_box`, embedded files, private flags, rounded rectangles, native ellipses, schematic variants und net chains.
- Ergänzt legacy property IDs und verschiebt property `hide` zurück nach `effects`.
- Wandelt boolesche `hide`-Listen und font `bold` / `italic` in presence atoms um.
- Entfernt je nach Ziel `generator_version`, `embedded_fonts`, font `face`, `show_name`, `do_not_autoplace`, power class flags und jumper pin flags.

Board / footprint:

- Entfernt oder konvertiert text boxes, images, net ties, generated objects, teardrops, tables, tenting, embedded files, component classes, padstacks, via stacks, rule areas, via protection, custom layer counts, rounded rectangles, points, barcodes, backdrill, variants, extruded models, native ellipses, dielectric frequency fields, net chains und copper thieving.
- Wandelt copper thieving fill mode in polygon fill um.
- Wandelt footprint `Reference` / `Value` properties in legacy `fp_text` zurück.
- Benennt passende `uuid`-Felder in `tstamp` und group/generated `uuid` in `id` um.
- Wandelt KiCad-7-inkompatible PCB dimensions in sichtbare Textannotationen um.
- Baut numeric netcodes und root-level net declarations für alte Boards neu auf.

Worksheet entfernt `font` blocks für Ziele vor `20220228`. Design rules werden erkannt und auf Zielversionen abgebildet, haben aber derzeit keine eigenen Rewrite-Regeln.

## Reports und Pflege

Jede Entfernung oder kompatible Umwandlung, die den Baum verändert, erzeugt ein warning. Reports enthalten path, kind, source version, target version, changed und warnings. Neue KiCad-Versionen sollten zuerst in der Matrix ergänzt und dann getrennt nach stable tag und development branch dokumentiert werden.

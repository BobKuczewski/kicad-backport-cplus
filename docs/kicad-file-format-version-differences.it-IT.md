# Differenze tra versioni del formato file KiCad

Questo documento è la versione italiana del [documento di riferimento in inglese](kicad-file-format-version-differences.md). Riassume le versioni di formato usate da KiCad Backport C++, le differenze principali e le regole di downgrade già implementate. Token KiCad, macro sorgente, estensioni e numeri di versione restano in inglese per facilitare la ricerca nel codice.

Ultimo aggiornamento: 2026-05-29.

## Fonti

- Tag ufficiali KiCad e header di versione.
- Checkout locale KiCad `master`: `10.99.0-1153-g8c1d374f29`.
- Implementazione `kicad-backport-cplus`: `kicad_backport_versions.cpp`, `kicad_backport_rules.cpp`, `kicad_backport_rule_rewriters.cpp`, `kicad_backport.cpp`.

Le versioni provengono da `SEXPR_SYMBOL_LIB_FILE_VERSION`, `SEXPR_SCHEMATIC_FILE_VERSION`, `SEXPR_BOARD_FILE_VERSION`, `SEXPR_WORKSHEET_FILE_VERSION` e `DRC_RULE_FILE_VERSION`.

## Matrice delle versioni stabili

| KiCad tag | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

La versione S-expression dei board copre anche `.kicad_mod`. `.kicad_dru` resta a `20200610` nell'intervallo tracciato; questo non garantisce che la semantica delle regole non sia cambiata. `.kicad_pro` è JSON e usa migrazioni di schema separate.

## 10.99 / current

| Tipo file | Versione 10.99/current | Note |
| --- | ---: | --- |
| Board `.kicad_pcb` | `20260513` | Copper thieving zone fill mode |
| Footprint `.kicad_mod` | `20260513` | Usa la versione S-expression PCB |
| Schematic `.kicad_sch` | `20260512` | Net chains |
| Symbol library `.kicad_sym` | `20260508` | Native ellipse primitive |
| Worksheet `.kicad_wks` | `20231118` | Nessun bump dopo il cleanup KiCad 8 |
| Design rules `.kicad_dru` | `20200610` | Nessun bump specifico 10.99 trovato |

Le aggiunte principali dopo KiCad 10 sono extruded 3D body metadata, native ellipses, dielectric frequency fields, net chains e copper thieving.

## Versioni target nell'implementazione C++

| Alias | Symbol | Schematic | Board | Footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |

Il convertitore non aggiorna file vecchi verso versioni più recenti. Se la versione sorgente è inferiore al target, copia il file senza modificarlo e mantiene la versione sorgente nel report.

## Regole principali di downgrade

Symbol library / schematic:

- Rimuove nodi non accettati dai parser più vecchi, come `text_box`, embedded files, private flags, rounded rectangles, native ellipses, schematic variants e net chains.
- Aggiunge legacy property IDs e sposta property `hide` dentro `effects`.
- Converte liste booleane `hide` e font `bold` / `italic` in presence atoms.
- Rimuove in base al target `generator_version`, `embedded_fonts`, font `face`, `show_name`, `do_not_autoplace`, power class flags e jumper pin flags.

Board / footprint:

- Rimuove o converte text boxes, images, net ties, generated objects, teardrops, tables, tenting, embedded files, component classes, padstacks, via stacks, rule areas, via protection, custom layer counts, rounded rectangles, points, barcodes, backdrill, variants, extruded models, native ellipses, dielectric frequency fields, net chains e copper thieving.
- Converte copper thieving fill mode in polygon fill.
- Converte le proprietà footprint `Reference` / `Value` in legacy `fp_text`.
- Rinomina gli `uuid` applicabili in `tstamp` e group/generated `uuid` in `id`.
- Converte PCB dimensions incompatibili con KiCad 7 in annotazioni di testo visibili.
- Ricostruisce numeric netcodes e root-level net declarations per board vecchi.

Worksheet rimuove `font` blocks per target precedenti a `20220228`. Design rules viene rilevato e risolto a una versione target, ma non ha ancora rewrite dedicati.

## Report e manutenzione

Ogni rimozione o rewrite compatibile che modifica l'albero produce un warning. I report includono path, kind, source version, target version, changed e warnings. Per aggiungere nuove versioni KiCad, aggiornare prima la matrice e poi documentare separatamente le differenze tra stable tag e development branch.

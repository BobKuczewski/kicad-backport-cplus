# Differenze tra le versioni del formato file di KiCad

Questo testo è sincronizzato strutturalmente con il riferimento inglese; termini tecnici KiCad, token e nomi delle modifiche upstream restano intenzionalmente invariati.

Questo documento traccia le differenze tra le versioni del formato file KiCad usate dal
convertitore di backport. La struttura consente di aggiungere nuove versioni stabili o di
sviluppo senza rinominare il file.

Ultimo aggiornamento: 2026-06-04.

## Fonti e metodo

Fonti esaminate:

- Tag GitLab ufficiali di KiCad e file sorgente.
- Checkout locale di KiCad in `E:/WORKS/MY/kicadProject/kicad`.
- Local refs and tags: `origin/4.0`, `4.0.0`, `origin/5.0`, `origin/5.1`,
  `5.0.0`, `5.1.0`, `6.0.0`, `7.0.0`, `8.0.0`, `9.0.0`, `10.0.0`,
  and `origin/10.0`.
- `master` locale di KiCad, usato solo per identificare i confini del ramo di sviluppo
  successivo a 10.0.
- Implementazione `kicad-backport-cplus`, in particolare:
  - `src/kicad_backport_versions.cpp`
  - `src/kicad_backport_rules.cpp`
  - `src/kicad_backport_rule_rewriters.cpp`
  - `src/kicad_backport.cpp`
- File header di versione:
  - `pcbnew/kicad_plugin.h` for KiCad 4/5 PCB formats.
  - `pcbnew/plugins/kicad/pcb_plugin.h` for KiCad 6/7 PCB formats.
  - `eeschema/sch_file_versions.h`
  - `pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h`
  - `include/drawing_sheet/ds_file_versions.h`
  - `pcbnew/drc/drc_rule_parser.h`
  - `eeschema/general.h` and `eeschema/sch_legacy_plugin.h` for KiCad 4/5
    legacy schematic formats.

I numeri di versione derivano dalle macro attive del sorgente KiCad:

- `SEXPR_SYMBOL_LIB_FILE_VERSION`
- `SEXPR_SCHEMATIC_FILE_VERSION`
- `SEXPR_BOARD_FILE_VERSION`
- `SEXPR_WORKSHEET_FILE_VERSION`
- `DRC_RULE_FILE_VERSION`

Note:

- Le versioni S-expression della board coprono anche i file footprint `.kicad_mod`.
- `.kicad_dru` stayed at `20200610` from KiCad 6.0 through current 10.99 sources.
  This only means the version macro did not change; rule semantics may still have
  changed.
- `.kicad_pro` is a JSON project file and uses settings/schema migration instead
  of these S-expression date version macros. Project JSON schema differences
  should be tracked separately.
- Gli schemi e le librerie simboli KiCad 4/5 sono file legacy `.sch`, `.lib` e
  `.dcm`, non `.kicad_sch` o `.kicad_sym`.

## Matrice principale delle famiglie di file

| KiCad major version | Project | Schematic | Symbol library | PCB / footprint | Worksheet | Design rules | Key point |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 4.0 | Legacy `.pro` | Legacy `.sch`, `EESCHEMA_VERSION=2` | `.lib` `EESchema-LIBRARY Version 2.3`, `.dcm` | `.kicad_pcb` / `.kicad_mod` S-expression, version `4` | Legacy drawing sheet | No standalone `.kicad_dru` | PCB was already S-expression; schematic and symbol libraries were still legacy. |
| 5.0 / 5.1 | Legacy `.pro` | Legacy `.sch`, `EESCHEMA_VERSION=4` | Commonly `.lib` `Version 2.4`, `.dcm` | `.kicad_pcb` / `.kicad_mod` S-expression, version `20171130` | Legacy drawing sheet | No standalone `.kicad_dru` | PCB added custom pads, multi-layer keepouts, and 3D model offset changes; schematic remained legacy. |
| 6.0 | JSON `.kicad_pro`, `.kicad_prl` | `.kicad_sch` `20211123` | `.kicad_sym` `20211014` | `20211014` | `.kicad_wks` `20210606` | `.kicad_dru` `20200610` | New schematic and symbol S-expression formats. |
| 7.0 | JSON `.kicad_pro` | `.kicad_sch` `20230121` | `.kicad_sym` `20220914` | `20221018` | `.kicad_wks` `20220228` | `20200610` | Text boxes, fonts, DNP, simulation model changes, net ties, images, teardrop keywords. |
| 8.0 | JSON `.kicad_pro` | `.kicad_sch` `20231120` | `.kicad_sym` `20231120` | `20240108` | `.kicad_wks` `20231118` | `20200610` | `generator_version`, V8 cleanup, PCB fields, generated objects, UUID normalization. |
| 9.0 | JSON `.kicad_pro` | `.kicad_sch` `20250114` | `.kicad_sym` `20241209` | `20241229` | `.kicad_wks` `20231118` | `20200610` | Embedded files, tables, rule areas, component classes, padstacks, via stacks, arbitrary user layers. |
| 10.0 | JSON `.kicad_pro` | `.kicad_sch` `20260306` | `.kicad_sym` `20251024` | `20260206` | `.kicad_wks` `20231118` | `20200610` | Variants, jumper pads, barcodes, via protection, backdrill, split via types, stopped writing netcodes. |

## Matrice delle versioni stabili

| KiCad tag | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

## Confine legacy KiCad 4/5

KiCad 4 e 5 non sono solo versioni S-expression più vecchie per i dati dello schema. Usano
una famiglia di file diversa per schemi e librerie simboli:

| Area | KiCad 4.0 | KiCad 5.0 / 5.1 |
| --- | --- | --- |
| Schematic header | `EESchema Schematic File Version 2` | `EESchema Schematic File Version 4` |
| Schematic macro | `EESCHEMA_VERSION 2` | `EESCHEMA_VERSION 4` |
| Symbol library header | Commonly `EESchema-LIBRARY Version 2.3` | Commonly `EESchema-LIBRARY Version 2.4` |
| PCB version | `4` | `20171130` |

Punti di versione PCB/footprint di KiCad 5 prima della linea di sviluppo KiCad 6:

| Versione | Modifica |
| ---: | --- |
| 20160815 | Differential pair settings per net class |
| 20170123 | `EDA_TEXT` refactor; moved `hide` |
| 20170920 | Long pad names and custom pad shape |
| 20170922 | Keepout zones can exist on multiple layers |
| 20171114 | Save 3D model offset in mm instead of inches |
| 20171125 | Locked/unlocked footprint text |
| 20171130 | 3D model offset written using the `offset` parameter |

Implicazioni per il backport:

- KiCad 4/5 schematic targets require a legacy `.sch` writer, not just a
  `.kicad_sch` version rewrite.
- KiCad 4/5 symbol targets require legacy `.lib` / `.dcm` output or an explicit
  lossy/unimplemented warning.
- KiCad 4 board targets use version `4`; KiCad 5 board targets use `20171130`.
- V6+ UUIDs, text boxes, embedded files, variants, tables, rule areas, component
  classes, padstacks, via stacks, backdrill, and similar structures cannot be
  preserved directly in V4/V5 files.

## Matrice della versione di sviluppo corrente

Il branch KiCad `master` esaminato è già passato allo sviluppo 11.0.
Questi elementi sono sviluppi successivi a 10.0 e non devono essere indicati come supporto del formato
stabile KiCad 10.0:

| Tipo di file | Versione di sviluppo corrente | Note |
| --- | ---: | --- |
| Board `.kicad_pcb` | `20260603` | Knockout flag on table cells |
| Footprint `.kicad_mod` | `20260603` | Footprints use the PCB S-expression version |
| Schematic `.kicad_sch` | `20260512` | Net chains |
| Symbol library `.kicad_sym` | `20260508` | Native ellipse primitive |
| Worksheet `.kicad_wks` | `20231118` | Generator version / KiCad 8 cleanup |
| Design rules `.kicad_dru` | `20200610` | No current-development-specific version bump found |

Passaggi di versione di sviluppo dopo 10.0 trovati finora:

| Versione | Tipo di file | Modifica |
| ---: | --- | --- |
| 20260410 | Board / footprint | Extruded 3D body |
| 20260508 | Board / footprint | Native PCB ellipse and ellipse-arc primitives |
| 20260508 | Schematic / symbol | Native schematic/symbol ellipse and ellipse-arc primitives |
| 20260511 | Board | Dielectric frequency-dependent stackup models |
| 20260512 | Board / schematic | Net chains |
| 20260513 | Board | Copper thieving zone fill mode |
| 20260521 | Board / footprint | Pad simulation electrical types |
| 20260603 | Board / footprint | Knockout flag on table cells |

## Da 6.0 a 7.0

### Libreria simboli

| Versione | Modifica |
| ---: | --- |
| 20220101 | Class flags |
| 20220102 | Fonts |
| 20220126 | Text boxes |
| 20220328 | Text box `start/end` changed to `at/size` |
| 20220331 | Text colors |
| 20220914 | Symbol unit display names |
| 20220914 | Property IDs are no longer saved |

### Schema

| Versione | Modifica |
| ---: | --- |
| 20220101 | Circles, arcs, rects, polys, beziers |
| 20220102 | Dash-dot-dot |
| 20220103 | Label fields |
| 20220104 | Fonts |
| 20220124 | `netclass_flag` renamed to `directive_label` |
| 20220126 | Text boxes |
| 20220328 | Text box `start/end` changed to `at/size` |
| 20220331 | Text colors |
| 20220404 | Default schematic symbol instance data |
| 20220622 | New simulation model format |
| 20220820 | Default symbol instance data fix |
| 20220822 | Text object hyperlinks |
| 20220903 | Field name visibility |
| 20220904 | Do not autoplace field option |
| 20220914 | DNP support |
| 20220929 | Property IDs are no longer saved |
| 20221002 | Instance data moved back to symbol definition |
| 20221004 | Instance data moved back to symbol definition |
| 20221110 | Sheet instance data moved to sheet definition |
| 20221126 | Removed value and footprint from instance data |
| 20221206 | Simulation model fields V6 to V7 |
| 20230121 | `SCH_MARKER` sheet path serialization |

### PCB / Footprint

| Versione | Modifica |
| ---: | --- |
| 20211226 | Radial dimension |
| 20211227 | Thermal relief spoke angle overrides |
| 20211228 | `allow_soldermask_bridges` footprint attribute |
| 20211229 | Stroke formatting |
| 20211230 | Dimensions in footprints |
| 20211231 | Private footprint layers |
| 20211232 | Fonts |
| 20220131 | Textboxes |
| 20220211 | Ended V5 zone fill strategy support |
| 20220225 | Removed TEDIT |
| 20220308 | Knockout text and locked graphic text property |
| 20220331 | Plot on all layers selection setting |
| 20220417 | Automatic dimension precisions |
| 20220427 | Excluded Edge.Cuts and Margin from footprint private layers |
| 20220609 | Teardrop keywords |
| 20220621 | Image support |
| 20220815 | `allow-soldermask-bridges-in-FPs` flag |
| 20220818 | First-class net ties |
| 20220914 | Custom-shape pad number boxes |
| 20221018 | Via and pad zone-layer-connections |

### Foglio di disegno

| Versione | Modifica |
| ---: | --- |
| 20220228 | Font support |

## Da 7.0 a 8.0

### Libreria simboli

| Versione | Modifica |
| ---: | --- |
| 20230620 | `ki_description` changed to `Description` field |
| 20231120 | `generator_version` and V8 cleanup |

### Schema

| Versione | Modifica |
| ---: | --- |
| 20230221 | Modern power symbols, editable value = net |
| 20230409 | `exclude_from_sim` markup |
| 20230620 | `ki_description` changed to `Description` field |
| 20230808 | `Sim.Enable` field moved to `exclude_from_sim` attribute |
| 20230819 | Multiple levels of library symbol inheritance |
| 20231120 | `generator_version` and V8 cleanup |

### PCB / Footprint

| Versione | Modifica |
| ---: | --- |
| 20230410 | DNP attribute propagated from schematic to `attr` |
| 20230517 | Pad and via teardrop parameters |
| 20230620 | PCB fields |
| 20230730 | Graphic shapes connectivity |
| 20230825 | Textbox explicit border flag |
| 20230906 | Multiple image type support |
| 20230913 | Custom-shaped-pad spoke templates |
| 20231007 | Generative objects |
| 20231014 | V8 file format normalization |
| 20231212 | Reference image locking / UUIDs, footprint boolean format |
| 20231231 | Generators and groups use `uuid` instead of `id` |
| 20240108 | Teardrop parameters changed to explicit booleans |

### Foglio di disegno

| Versione | Modifica |
| ---: | --- |
| 20230607 | Images saved as base64 |
| 20231118 | `generator_version` and V8 file format cleanup |

## Da 8.0 a 9.0

### Libreria simboli

| Versione | Modifica |
| ---: | --- |
| 20240529 | Embedded files |
| 20240819 | Embedded file hash algorithm changed to Murmur3 |
| 20241209 | `SCH_FIELD` private flags |

### Schema

| Versione | Modifica |
| ---: | --- |
| 20240101 | Tables |
| 20240417 | Rule areas |
| 20240602 | Sheet attributes |
| 20240620 | Embedded files |
| 20240716 | Multiple netclass assignments |
| 20240812 | Netclass color highlighting |
| 20240819 | Embedded file hash algorithm changed to Murmur3 |
| 20241004 | Symbol `hide` uses booleans |
| 20241209 | `SCH_FIELD` private flags |
| 20250114 | Text variable cross references use full paths |

### PCB / Footprint

| Versione | Modifica |
| ---: | --- |
| 20240201 | Overrides use nullable properties |
| 20240202 | Tables |
| 20240225 | `solder_paste_margin` rationalization |
| 20240609 | `tenting` keyword |
| 20240617 | Table angles |
| 20240703 | User layer types |
| 20240706 | Embedded files |
| 20240819 | Embedded file hash algorithm changed to Murmur3 |
| 20240928 | Component classes |
| 20240929 | Complex padstacks |
| 20241006 | Via stacks |
| 20241007 | Tracks can carry soldermask layer and margin |
| 20241009 | Placement rule area format evolution |
| 20241010 | Graphic shapes can carry soldermask layer and margin |
| 20241030 | Dimension arrow directions, `suppress_zeroes` normalization |
| 20241129 | Normalized `keep_text_aligned` and fill properties |
| 20241228 | Teardrop curve points changed to boolean |
| 20241229 | User layers expanded to arbitrary count |

### Foglio di disegno

No worksheet version bump; remains `20231118`.

## Da 9.0 a 10.0

### Libreria simboli

| Versione | Modifica |
| ---: | --- |
| 20250318 | `~` no longer means empty text |
| 20250324 | Jumper pin groups |
| 20250829 | Rounded rectangles |
| 20250901 | Stacked pin notation |
| 20250925 | Bus aliases in project file |
| 20251024 | Property formatting updates: `do_not_autoplace`, `show_name` |

### Schema

| Versione | Modifica |
| ---: | --- |
| 20250222 | Hatched fills for shapes |
| 20250227 | Local power symbols |
| 20250318 | `~` no longer means empty text |
| 20250425 | UUIDs for tables |
| 20250513 | Groups can carry design-block `lib_id` |
| 20250610 | Rule areas support DNP and other flags |
| 20250827 | Custom body styles |
| 20250829 | Rounded rectangles |
| 20250901 | Stacked pin notation |
| 20250922 | Schematic variants |
| 20251012 | Flat schematic hierarchy support |
| 20251028 | Property formatting updates: `do_not_autoplace`, `show_name` |
| 20260101 | PCB variants |
| 20260306 | Variant `in_bom` semantics corrected |

### PCB / Footprint

| Versione | Modifica |
| ---: | --- |
| 20250210 | Textbox knockout |
| 20250222 | PCB shapes hatching |
| 20250228 | IPC-4761 via protection features |
| 20250302 | Zone hatching offsets |
| 20250309 | Component class dynamic assignment rules |
| 20250324 | Jumper pads |
| 20250401 | Time domain length tuning |
| 20250513 | Groups can carry design-block `lib_id` |
| 20250801 | `(island)` changed to `(island yes/no)` |
| 20250811 | Press-fit pad fabrication property |
| 20250818 | Footprints support custom layer counts |
| 20250829 | Rounded rectangles |
| 20250901 | PCB points |
| 20250907 | UUIDs for tables |
| 20250909 | Footprint unit metadata: units / pins |
| 20250914 | `PCB_BARCODE` objects |
| 20250926 | Via types split into blind / buried / through |
| 20251027 | Pad-to-die delays scaling fix |
| 20251028 | Stopped writing netcodes; they are internal implementation details |
| 20251101 | Backdrill and tertiary drill support |
| 20260101 | PCB variants with per-footprint overrides |
| 20260206 | Barcode and variant attribute serialization fixes |

### Foglio di disegno

No worksheet version bump; remains `20231118`.

## Da 10.0 allo sviluppo corrente

Rispetto ai file target KiCad 10, il ramo di sviluppo corrente esaminato
aggiunge questi passaggi di formato più recenti:

| Version | File type | Difference |
| ---: | --- | --- |
| 20260410 | Board / footprint | Extruded 3D body metadata in footprint model blocks |
| 20260508 | Board / footprint | Native PCB ellipse and ellipse-arc primitives |
| 20260508 | Schematic / symbol | Native schematic/symbol ellipse and ellipse-arc primitives |
| 20260511 | Board | Dielectric frequency-dependent stackup model fields |
| 20260512 | Board | Net chain aggregation block |
| 20260512 | Schematic | Net chain records |
| 20260513 | Board | Copper thieving zone fill mode |
| 20260521 | Board / footprint | Pad simulation electrical type, serialized as `sim_electrical_type` on pads |
| 20260603 | Board / footprint | Table-cell `knockout` flag |

## Riepilogo dei target di backport dai file di sviluppo correnti

Compared with older supported targets, 10.99 introduces or retains newer
constructs that must be removed, simplified, or renamed when backporting:

| Target | Board / footprint target | Schematic target | Symbol target | Main downgrade areas from current development |
| --- | ---: | ---: | ---: | --- |
| KiCad 10 | `20260206` | `20260306` | `20251024` | Remove development-only extruded body metadata, native ellipses, dielectric frequency fields, net chains, copper thieving, pad simulation electrical types, and table-cell knockout flags |
| KiCad 9 | `20241229` | `20250114` | `20241209` | KiCad 10 items plus PCB shape hatching, via protection, zone hatch offsets, jumper pads, group design-block IDs, custom layer counts, rounded rectangles, PCB points, table UUIDs, barcodes, split via types, netcode omission, backdrill/post-machining, PCB variants, schematic variants/body styles/rounded rectangles/stacked pins/property formatting |
| KiCad 8 | `20240108` | `20231120` | `20231120` | KiCad 9 items plus tables, embedded files, component classes, padstacks, via stacks, rule areas, tenting, user layer expansion, sheet attributes, multiple netclass assignments, netclass color highlighting |
| KiCad 7 | `20221018` | `20230121` | `20220914` | KiCad 8 items plus PCB fields, DNP attribute propagation, modern teardrops, custom pad spoke templates, generators, UUID/id normalization, text boxes, images, net ties, font/field formatting, rule areas, modern schematic simulation/exclude flags |

## Copertura dell'implementazione C++ del backport

La CLI `kicad-backport-cplus` implementa riscritture S-expression guidate dalla versione.
Risolve gli alias di release per tipo di documento, applica le regole di downgrade e poi scrive
il campo `version` target. Accetta anche una versione numerica grezza del formato file
per testare uno specifico cutoff del parser.

Supported alias mappings in code:

| Alias | Symbol | Schematic | Board | Footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |

Il convertitore non aggiorna i file vecchi verso versioni più nuove. Se il file sorgente ha una versione
numerica inferiore alla versione target richiesta, copia il file senza modifiche
e mantiene la versione sorgente nel report.

### Rilevamento documenti e gestione progetti

L'implementazione C++ rileva il tipo di documento KiCad principalmente dall'head radice
della S-expression:

| Head radice | Tipo |
| --- | --- |
| `kicad_symbol_lib` | Symbol library |
| `kicad_sch` | Schematic |
| `kicad_pcb` | Board |
| `footprint` | Footprint |
| `kicad_dru` | Design rules |
| `kicad_wks`, `drawing_sheet` | Worksheet |

Se l'head radice manca o è sconosciuto, il rilevamento usa come fallback l'estensione del file:
`.kicad_sym`, `.kicad_sch`, `.kicad_pcb`, `.kicad_mod`, `.kicad_dru` e
`.kicad_wks`.

When converting a project directory or `.kicad_pro`, it copies only editable
KiCad project inputs and common local 3D model files. Generated outputs,
history/backup folders, Gerbers, fabrication outputs, BOMs, and temporary files
are skipped. For KiCad 7 and KiCad 8 board targets it also creates legacy
`.kicad_prl` local board display settings so converted through-hole pads remain
visible.

### Regole della libreria simboli

I gate generici del parser rimuovono questi nodi introdotti quando il formato target
è più vecchio della versione di introduzione:

| Introdotto | Heads rimossi | Motivo |
| ---: | --- | --- |
| 20220126 | `text_box`, `textbox` | Symbol text boxes |
| 20240529 | `embedded_files`, `embedded_file` | Embedded files |
| 20241209 | `private` | Private SCH_FIELD flags |
| 20250324 | `pin_group`, `pin_groups` | Jumper pin groups |
| 20250829 | `rounded_rectangle`, `roundrect` | Rounded rectangles |
| 20260508 | `ellipse`, `ellipse_arc` | Native ellipse primitives |

Riscritture di compatibilità:

| Cutoff target | Riscrittura |
| ---: | --- |
| `< 20231120` | Remove root `generator_version` fields |
| `< 20241209` | Remove `embedded_fonts`; add legacy property IDs; move property `hide` flags into `effects` |
| `< 20240108` | Convert font `(bold yes/no)` and `(italic yes/no)` lists to legacy presence atoms |
| `<= 20241209` | Remove font `face` fields |
| `< 20241004` | Convert boolean `hide` lists to legacy atoms; flatten `pin_names` / `pin_numbers` hide lists |
| `< 20251024` | Remove symbol `in_pos_files`; remove property `show_name` and `do_not_autoplace` |
| `< 20250324` | Remove `duplicate_pin_numbers_are_jumpers` |
| `< 20250227` | Remove symbol `power` class flags |

### Regole dello schema

Gate generici del parser:

| Introdotto | Heads rimossi | Motivo |
| ---: | --- | --- |
| 20220126 | `text_box`, `textbox` | Schematic text boxes |
| 20220622 | `simulation_model`, `sim_model` | New simulation model format |
| 20240101 | `table` | Schematic tables |
| 20240417 | `rule_area` | Schematic rule areas |
| 20240620 | `embedded_files`, `embedded_file` | Embedded files |
| 20241209 | `private` | Private SCH_FIELD flags |
| 20250829 | `rounded_rectangle`, `roundrect` | Rounded rectangles |
| 20250922 | `variants`, `variant` | Schematic variants |
| 20260508 | `ellipse`, `ellipse_arc` | Native ellipse primitives |
| 20260512 | `net_chain`, `net_chains` | Schematic net chains |

Riscritture di compatibilità:

| Cutoff target | Riscrittura |
| ---: | --- |
| `< 20231120` | Remove `generator_version`; remove `fields_autoplaced` from symbols and sheets |
| `< 20260326` | Remove schematic `locked` fields introduced after the target |
| `< 20260306` | Remove `embedded_fonts`; remove sheet `exclude_from_sim`, `in_bom`, `on_board`, `dnp`; remove root schematic `group` nodes |
| `< 20250827` | Remove `body_styles` and `body_style` |
| `< 20250114` | Remove text/textbox `exclude_from_sim` |
| `<= 20230121` | Remove all remaining `exclude_from_sim` |
| `< 20251024` | Remove symbol `in_pos_files` |
| `< 20250324` | Remove `duplicate_pin_numbers_are_jumpers` |
| `< 20250227` | Remove symbol `power` class flags |
| `< 20241004` | Convert boolean `hide` lists to legacy atoms; flatten pin visibility hide lists |
| `< 20240108` | Convert font bold/italic boolean lists to legacy atoms |
| `<= 20250114` | Remove font `face` fields |
| `< 20241209` | Add legacy property IDs; move property `hide` flags into `effects` |
| `< 20251028` | Remove property `show_name` and `do_not_autoplace` |

### Regole di board e footprint

Gate generici del parser:

| Introdotto | Heads rimossi | Motivo |
| ---: | --- | --- |
| 20220131 | `gr_text_box`, `fp_text_box`, `text_box`, `textbox` | PCB textboxes |
| 20220621 | `image` | PCB image objects |
| 20220818 | `net_tie`, `net_ties` | First-class net-tie storage |
| 20231007 | `generated` | Generative objects |
| 20240108 | `teardrop`, `teardrops` | Teardrop parameters |
| 20240202 | `table` | PCB tables |
| 20240609 | `tenting` | Tenting keyword |
| 20240706 | `embedded_files`, `embedded_file`, `embedded_fonts` | Embedded files |
| 20240928 | `component_class`, `component_classes` | Component classes |
| 20240929 | `padstack` | Complex padstacks |
| 20241006 | `via_stack`, `viastack` | Via stacks |
| 20241009 | `rule_area` | Placement/rule areas |
| 20250228 | `via_protection`, `covering`, `plugging`, `filling`, `capping` | IPC-4761 via protection |
| 20250818 | `custom_layer_count`, `custom_layer_counts` | Custom footprint layer counts |
| 20250829 | `rounded_rectangle`, `roundrect` | Rounded rectangles |
| 20250901 | `point` | PCB point objects |
| 20250914 | `barcode`, `pcb_barcode`, `gr_barcode`, `fp_barcode` | PCB barcode objects |
| 20251101 | `backdrill`, `tertiary_drill`, `front_post_machining`, `back_post_machining` | Backdrill and tertiary drill fields |
| 20260101 | `variants`, `variant` | PCB variants |
| 20260410 | `extruded` | Extruded footprint 3D body models |
| 20260508 | `gr_ellipse`, `gr_ellipse_arc`, `fp_ellipse`, `fp_ellipse_arc` | Native PCB ellipse primitives |
| 20260511 | `spec_frequency`, `dielectric_model` | Dielectric frequency-dependent stackup fields |
| 20260512 | `net_chains`, `net_chain` | PCB net chains |
| 20260513 | `thieving` | Copper thieving zone fill mode |

Current development coverage gaps found in local KiCad `10.99.0-1273-gd90e32b6a0`:

| Introdotto | Gestione downgrade mancante | Note |
| ---: | --- | --- |
| 20260521 | Pad `sim_electrical_type` | Serialized as `(sim_electrical_type source)` or `(sim_electrical_type sink)` on pads; not yet present in `kicad-backport-cplus` feature gates. |
| 20260603 | Table-cell `knockout` flag | Must be handled contextually for PCB table cells; `knockout` is not safe as a global token gate because other object types also use it. |

Riscritture di compatibilità:

| Cutoff target | Riscrittura |
| ---: | --- |
| `< 20260410` | Remove typed/extruded 3D model blocks by removing `model` nodes that contain `type` |
| `< 20260513` | Replace copper thieving zone fill mode with polygon fill |
| `>= 20220225` | Remove obsolete footprint `tedit` fields |
| `>= 20200628` | Remove obsolete board `visible_elements` settings |
| `< 20240703` | Convert user-layer type qualifiers `front`, `back`, `auxiliary` to `user` |
| `< 20241010` | Remove graphic `solder_mask_margin` fields |
| `< 20241030` | Convert dimension boolean fields to legacy atoms; remove dimension `arrow_direction` |
| `< 20241009` | Remove zone `placement` fields |
| `< 20241007` | Remove track `solder_mask_margin` and `solder_mask_layer` fields |
| `< 20240617` | Remove PCB table cell `angle` |
| `< 20250228` | Convert tenting front/back boolean lists to legacy atoms; remove IPC-4761 protection fields |
| `< 20231212` | Convert `locked` and `hide` boolean lists to presence atoms; remove `unlocked`; remove model `hide` |
| `< 20231014` | Remove `generator_version` |
| `< 20230924` | Convert `pcbplotparams` `yes/no` booleans to `true/false`; convert shape fill `no` to `none` |
| `< 20230730` | Remove graphic shape `net` connectivity |
| `< 20240108` | Remove group `locked`; convert font bold/italic boolean lists to legacy atoms |
| `< 20230620` | Convert footprint `Reference` and `Value` properties back to `fp_text`; convert `Description` to `ki_description`; map `sheetname`/`sheetfile` to properties |
| `< 20231231` | Rename scoped `uuid` fields back to `tstamp`; rename group/generated `uuid` back to `id` |
| `< 20250324` | Remove footprint jumper pad fields |
| `<= 20221018` | Remove footprint `dnp` attributes; remove pad/via `remove_unused_layers`; convert dimensions to visible text annotations; remove legacy-incompatible `locked`; downgrade free via fields |
| `< 20250309` | Remove `component_class` from placement rules |
| `< 20250222` | Convert PCB hatch/reverse-hatch/cross-hatch shape fills to solid fill |
| `< 20250210` | Remove PCB text box `knockout`; add `filled_areas_thickness no` to cached zone fills where needed |
| `<= 20241229` | Remove PCB font `face` fields |
| `< 20251101` | Remove pad/via post-machining fields |
| `< 20251028` | Rebuild legacy numeric board netcodes and root-level net declarations |

### Foglio di disegno e regole di progettazione

La gestione dei fogli di disegno ha attualmente un gate del parser implementato:

| Cutoff target | Riscrittura |
| ---: | --- |
| `< 20220228` | Remove worksheet `font` blocks |

Le regole di progettazione vengono rilevate e hanno alias di versione target, ma non sono implementate riscritture di downgrade
perché la macro della versione del formato file resta
`20200610` in tutte le versioni KiCad tracciate.

### Semantica di avvisi e report

Ogni rimozione implementata o riscrittura di compatibilità che modifica l'albero aggiunge un
avviso. I feature gate generici riportano il numero di nodi rimossi e la
versione di introduzione. I report includono percorso, tipo rilevato, versione sorgente,
versione target, flag di modifica e avvisi.

## Requisiti del convertitore

### Percorso di lettura

- Conservare la `version` del file sorgente; non interpretarlo solo come formato KiCad corrente.
- Supportare alias di compatibilità per i file più vecchi:
  - `page` to `paper`
  - Legacy overbar `~...~` to `~{...}`
  - Old `start/end` text box format to new `at/size`
  - Old `id` to `uuid`
  - Old boolean / presence-token formats to explicit booleans
- Rilevare formati futuri e restituire un errore chiaro o una strategia di downgrade definita.

### Percorso di scrittura

- `--target-version` must do more than change the top-level version number. It
  must prune or rewrite semantics according to the requested target.
- Each target version needs feature gates:
  - KiCad 6 must not write V7 simulation model fields, DNP, or post-V6 text box
    structures.
  - KiCad 7 must not write structures that only became stable after V8
    `generator_version` cleanup.
  - KiCad 8 must not write V9 embedded files, component classes, or complex
    padstacks.
  - KiCad 9 must not write V10 variants, barcode, backdrill, split via type, and
    similar constructs.
  - KiCad 10 must not write current-development extruded body metadata, native
    ellipses, dielectric frequency fields, net chains, copper thieving, pad
    simulation electrical types, or table-cell knockout flags.
- I downgrade con perdita devono produrre avvisi o metadati sidecar invece di eliminare
  in silenzio.

### Percorso di test

- Build minimal fixtures for KiCad 6, 7, 8, 9, and 10:
  - Symbol library
  - Schematic
  - Board
  - Footprint
  - Worksheet
  - Design rules
- Each cross-version conversion should verify:
  - Source version is read correctly
  - Target version number is written correctly
  - Disallowed tokens are removed or downgraded
  - Key semantics are preserved
  - Warnings cover lossy conversions

## Note di manutenzione

Quando si aggiungono future differenze di versione:

1. Add or update the version matrix first.
2. Add a new interval section such as `10.0 to 11.0` or
   `10.99 / current to 11.99 / current`.
3. Keep development-branch findings separate from released stable tags until the
   corresponding KiCad release is tagged.
4. Update the backport target summary when a new source version introduces
   constructs that affect existing downgrade targets.
5. Track `.kicad_pro` JSON schema migrations in a separate document.

# KiCad File Format Version Differences

This document tracks KiCad file format version differences used by the backport
converter. It is organized so newer stable or development versions can be added
without renaming the file.

Last updated: 2026-06-05.

## Sources and Method

Sources reviewed:

- KiCad official GitLab tags and source files.
- Local KiCad checkout at `E:/WORKS/MY/kicadProject/kicad`.
- Local refs and tags: `origin/4.0`, `4.0.0`, `origin/5.0`, `origin/5.1`,
  `5.0.0`, `5.1.0`, `6.0.0`, `7.0.0`, `8.0.0`, `9.0.0`, `10.0.0`,
  and `origin/10.0`.
- Local KiCad `master`, used only to identify post-10.0 development branch
  boundaries.
- `kicad-backport-cplus` implementation, especially:
  - `src/kicad_backport_versions.cpp`
  - `src/kicad_backport_rules.cpp`
  - `src/kicad_backport_rule_rewriters.cpp`
  - `src/kicad_backport_upgrade.cpp`
  - `src/kicad_backport.cpp`
- Version header files:
  - `pcbnew/kicad_plugin.h` for KiCad 4/5 PCB formats.
  - `pcbnew/plugins/kicad/pcb_plugin.h` for KiCad 6/7 PCB formats.
  - `eeschema/sch_file_versions.h`
  - `pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h`
  - `include/drawing_sheet/ds_file_versions.h`
  - `pcbnew/drc/drc_rule_parser.h`
  - `eeschema/general.h` and `eeschema/sch_legacy_plugin.h` for KiCad 4/5
    legacy schematic formats.

Version numbers are taken from the active KiCad source macros:

- `SEXPR_SYMBOL_LIB_FILE_VERSION`
- `SEXPR_SCHEMATIC_FILE_VERSION`
- `SEXPR_BOARD_FILE_VERSION`
- `SEXPR_WORKSHEET_FILE_VERSION`
- `DRC_RULE_FILE_VERSION`

Notes:

- Board S-expression versions also cover footprint `.kicad_mod` files.
- `.kicad_dru` stayed at `20200610` from KiCad 6.0 through current 10.99 sources.
  This only means the version macro did not change; rule semantics may still have
  changed.
- `.kicad_pro` is a JSON project file and uses settings/schema migration instead
  of these S-expression date version macros. Project JSON schema differences
  should be tracked separately.
- KiCad 4/5 schematics and symbol libraries are legacy `.sch`, `.lib`, and
  `.dcm` files, not `.kicad_sch` or `.kicad_sym`.

## Major File-Family Matrix

| KiCad major version | Project | Schematic | Symbol library | PCB / footprint | Worksheet | Design rules | Key point |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 4.0 | Legacy `.pro` | Legacy `.sch`, `EESCHEMA_VERSION=2` | `.lib` `EESchema-LIBRARY Version 2.3`, `.dcm` | `.kicad_pcb` / `.kicad_mod` S-expression, version `4` | Legacy drawing sheet | No standalone `.kicad_dru` | PCB was already S-expression; schematic and symbol libraries were still legacy. |
| 5.0 / 5.1 | Legacy `.pro` | Legacy `.sch`, `EESCHEMA_VERSION=4` | Commonly `.lib` `Version 2.4`, `.dcm` | `.kicad_pcb` / `.kicad_mod` S-expression, version `20171130` | Legacy drawing sheet | No standalone `.kicad_dru` | PCB added custom pads, multi-layer keepouts, and 3D model offset changes; schematic remained legacy. |
| 6.0 | JSON `.kicad_pro`, `.kicad_prl` | `.kicad_sch` `20211123` | `.kicad_sym` `20211014` | `20211014` | `.kicad_wks` `20210606` | `.kicad_dru` `20200610` | New schematic and symbol S-expression formats. |
| 7.0 | JSON `.kicad_pro` | `.kicad_sch` `20230121` | `.kicad_sym` `20220914` | `20221018` | `.kicad_wks` `20220228` | `20200610` | Text boxes, fonts, DNP, simulation model changes, net ties, images, teardrop keywords. |
| 8.0 | JSON `.kicad_pro` | `.kicad_sch` `20231120` | `.kicad_sym` `20231120` | `20240108` | `.kicad_wks` `20231118` | `20200610` | `generator_version`, V8 cleanup, PCB fields, generated objects, UUID normalization. |
| 9.0 | JSON `.kicad_pro` | `.kicad_sch` `20250114` | `.kicad_sym` `20241209` | `20241229` | `.kicad_wks` `20231118` | `20200610` | Embedded files, tables, rule areas, component classes, padstacks, via stacks, arbitrary user layers. |
| 10.0 | JSON `.kicad_pro` | `.kicad_sch` `20260306` | `.kicad_sym` `20251024` | `20260206` | `.kicad_wks` `20231118` | `20200610` | Variants, jumper pads, barcodes, via protection, backdrill, split via types, stopped writing netcodes. |

## Stable Version Matrix

| KiCad tag | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

## KiCad 4/5 Legacy Boundary

KiCad 4 and 5 are not just older S-expression versions for schematic data. They
use a different schematic and symbol-library file family:

| Area | KiCad 4.0 | KiCad 5.0 / 5.1 |
| --- | --- | --- |
| Schematic header | `EESchema Schematic File Version 2` | `EESchema Schematic File Version 4` |
| Schematic macro | `EESCHEMA_VERSION 2` | `EESCHEMA_VERSION 4` |
| Symbol library header | Commonly `EESchema-LIBRARY Version 2.3` | Commonly `EESchema-LIBRARY Version 2.4` |
| PCB version | `4` | `20171130` |

KiCad 5 PCB/footprint version points before the KiCad 6 development line:

| Version | Change |
| ---: | --- |
| 20160815 | Differential pair settings per net class |
| 20170123 | `EDA_TEXT` refactor; moved `hide` |
| 20170920 | Long pad names and custom pad shape |
| 20170922 | Keepout zones can exist on multiple layers |
| 20171114 | Save 3D model offset in mm instead of inches |
| 20171125 | Locked/unlocked footprint text |
| 20171130 | 3D model offset written using the `offset` parameter |

Backport implications:

- KiCad 4/5 schematic targets require a legacy `.sch` writer, not just a
  `.kicad_sch` version rewrite.
- KiCad 4/5 symbol targets require legacy `.lib` / `.dcm` output or an explicit
  lossy/unimplemented warning.
- KiCad 4 board targets use version `4`; KiCad 5 board targets use `20171130`.
- V6+ UUIDs, text boxes, embedded files, variants, tables, rule areas, component
  classes, padstacks, via stacks, backdrill, and similar structures cannot be
  preserved directly in V4/V5 files.

## Current Development Version Matrix

The reviewed KiCad `master` branch has already moved into 11.0 development.
These findings are post-10.0 development items and must not be labeled as KiCad
10.0 stable format support:

| File type | Current development version | Notes |
| --- | ---: | --- |
| Board `.kicad_pcb` | `20260603` | Knockout flag on table cells |
| Footprint `.kicad_mod` | `20260603` | Footprints use the PCB S-expression version |
| Schematic `.kicad_sch` | `20260512` | Net chains |
| Symbol library `.kicad_sym` | `20260508` | Native ellipse primitive |
| Worksheet `.kicad_wks` | `20231118` | Generator version / KiCad 8 cleanup |
| Design rules `.kicad_dru` | `20200610` | No current-development-specific version bump found |

Post-10.0 development version steps found so far:

| Version | File type | Change |
| ---: | --- | --- |
| 20260410 | Board / footprint | Extruded 3D body |
| 20260508 | Board / footprint | Native PCB ellipse and ellipse-arc primitives |
| 20260508 | Schematic / symbol | Native schematic/symbol ellipse and ellipse-arc primitives |
| 20260511 | Board | Dielectric frequency-dependent stackup models |
| 20260512 | Board / schematic | Net chains |
| 20260513 | Board | Copper thieving zone fill mode |
| 20260521 | Board / footprint | Pad simulation electrical types |
| 20260603 | Board / footprint | Knockout flag on table cells |

## 6.0 to 7.0

### Symbol Library

| Version | Change |
| ---: | --- |
| 20220101 | Class flags |
| 20220102 | Fonts |
| 20220126 | Text boxes |
| 20220328 | Text box `start/end` changed to `at/size` |
| 20220331 | Text colors |
| 20220914 | Symbol unit display names |
| 20220914 | Property IDs are no longer saved |

### Schematic

| Version | Change |
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

| Version | Change |
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

### Worksheet

| Version | Change |
| ---: | --- |
| 20220228 | Font support |

## 7.0 to 8.0

### Symbol Library

| Version | Change |
| ---: | --- |
| 20230620 | `ki_description` changed to `Description` field |
| 20231120 | `generator_version` and V8 cleanup |

### Schematic

| Version | Change |
| ---: | --- |
| 20230221 | Modern power symbols, editable value = net |
| 20230409 | `exclude_from_sim` markup |
| 20230620 | `ki_description` changed to `Description` field |
| 20230808 | `Sim.Enable` field moved to `exclude_from_sim` attribute |
| 20230819 | Multiple levels of library symbol inheritance |
| 20231120 | `generator_version` and V8 cleanup |

### PCB / Footprint

| Version | Change |
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

### Worksheet

| Version | Change |
| ---: | --- |
| 20230607 | Images saved as base64 |
| 20231118 | `generator_version` and V8 file format cleanup |

## 8.0 to 9.0

### Symbol Library

| Version | Change |
| ---: | --- |
| 20240529 | Embedded files |
| 20240819 | Embedded file hash algorithm changed to Murmur3 |
| 20241209 | `SCH_FIELD` private flags |

### Schematic

| Version | Change |
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

| Version | Change |
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

### Worksheet

No worksheet version bump; remains `20231118`.

## 9.0 to 10.0

### Symbol Library

| Version | Change |
| ---: | --- |
| 20250318 | `~` no longer means empty text |
| 20250324 | Jumper pin groups |
| 20250829 | Rounded rectangles |
| 20250901 | Stacked pin notation |
| 20250925 | Bus aliases in project file |
| 20251024 | Property formatting updates: `do_not_autoplace`, `show_name` |

### Schematic

| Version | Change |
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

| Version | Change |
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

### Worksheet

No worksheet version bump; remains `20231118`.

## 10.0 to Current Development

Compared with KiCad 10 target files, the reviewed current development branch
adds these newer format steps:

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

## Backport Target Summary from Current Development Files

Compared with older supported targets, 10.99 introduces or retains newer
constructs that must be removed, simplified, or renamed when backporting:

| Target | Board / footprint target | Schematic target | Symbol target | Main downgrade areas from current development |
| --- | ---: | ---: | ---: | --- |
| KiCad 10 | `20260206` | `20260306` | `20251024` | Remove development-only extruded body metadata, native ellipses, dielectric frequency fields, net chains, copper thieving, pad simulation electrical types, and table-cell knockout flags |
| KiCad 9 | `20241229` | `20250114` | `20241209` | KiCad 10 items plus PCB shape hatching, via protection, zone hatch offsets, jumper pads, group design-block IDs, custom layer counts, rounded rectangles, PCB points, table UUIDs, barcodes, split via types, netcode omission, backdrill/post-machining, PCB variants, schematic variants/body styles/rounded rectangles/stacked pins/property formatting |
| KiCad 8 | `20240108` | `20231120` | `20231120` | KiCad 9 items plus tables, embedded files, component classes, padstacks, via stacks, rule areas, tenting, user layer expansion, sheet attributes, multiple netclass assignments, netclass color highlighting |
| KiCad 7 | `20221018` | `20230121` | `20220914` | KiCad 8 items plus PCB fields, DNP attribute propagation, modern teardrops, custom pad spoke templates, generators, UUID/id normalization, text boxes, images, net ties, font/field formatting, rule areas, modern schematic simulation/exclude flags |

## C++ Backport Implementation Coverage

The `kicad-backport-cplus` CLI implements version-driven S-expression rewrites.
It resolves release aliases per document kind, applies downgrade rules, then
writes the target `version` field. It also accepts a raw numeric file format
version to test a specific parser cutoff.

Supported alias mappings in code:

| Alias | Symbol | Schematic | Board | Footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |

If the source file already has exactly the requested numeric version, the
converter copies it unchanged. If the source version is lower than the target,
the C++ implementation now applies limited compatibility upgrades before
writing the requested `version` field:

| Kind | Implemented upgrade normalization |
| --- | --- |
| Symbol library | Expand legacy font-style atoms for modern targets; expand pin visibility atoms; move property `hide` out of `effects`; remove legacy property IDs. |
| Schematic | Rename `tstamp` to `uuid`; rename `netclass_flag` to `directive_label`; convert old text-box `start/end` to `at/size`; expand legacy font and pin-visibility atoms; move property `hide` out of `effects`; remove legacy property IDs. |
| Board / footprint | Rename `tstamp` to `uuid` for modern targets; expand font-style atoms; expand footprint `dnp` atoms; normalize booleans to KiCad 7-style `yes/no`; remove obsolete `tedit`; optionally convert legacy numeric net references to net names. |

This is not a full semantic upgrade engine; it only normalizes syntax the
converter already knows how to express.

### Implemented Release-Target Coverage

The C++ rules are cutoff-driven, so each release alias activates the rules whose
cutoffs are newer than that file family's target version. The following summary
lists the practical coverage for the non-V6 stable targets.

#### KiCad 10 Target

KiCad 10 targets mostly remove post-10.0/current-development constructs:

| Kind | Implemented handling |
| --- | --- |
| Symbol library | Remove native ellipse and ellipse-arc primitives introduced after the 10.0 symbol target. |
| Schematic | Remove post-10.0 `locked` fields, native ellipse primitives, and `net_chain` / `net_chains`. |
| Board / footprint | Remove or downgrade post-10.0 typed/extruded model blocks, native ellipse primitives, dielectric frequency stackup fields, net chains, copper thieving fill mode, pad `sim_electrical_type`, and table-cell `knockout`. |
| Project side files | No legacy `.kicad_prl` or library-table compatibility rewrite is generated for the V10 suffix. |

#### KiCad 9 Target

KiCad 9 targets remove KiCad 10 and current-development syntax while retaining
features that are valid at the KiCad 9 file versions:

| Kind | Implemented handling |
| --- | --- |
| Symbol library | Remove jumper pin groups, rounded rectangles, native ellipses, symbol `in_pos_files`, `duplicate_pin_numbers_are_jumpers`, `power` class flags, property `show_name` / `do_not_autoplace`, and font `face`. |
| Schematic | Remove rounded rectangles, schematic variants, native ellipses, net chains, post-target `locked`, `embedded_fonts`, custom body styles, sheet assembly/simulation flags, symbol `in_pos_files`, jumper/power class flags, font `face`, property formatting fields, and root `group` nodes. |
| Board / footprint | Remove or downgrade IPC-4761 via protection, jumper pad fields, component-class placement sources, PCB hatch fills, custom layer counts, rounded rectangles, PCB point objects, barcodes, backdrill/post-machining fields, PCB variants, current-development features, and font `face`; rebuild legacy numeric board netcodes. Tenting is downgraded from boolean front/back lists to legacy atoms for this target range. |
| Project side files | No legacy `.kicad_prl` rewrite is generated for V9. |

#### KiCad 8 Target

KiCad 8 targets remove KiCad 9/10/current syntax and also normalize several
late-KiCad-8 development forms back to the 8.0.0 file versions:

| Kind | Implemented handling |
| --- | --- |
| Symbol library | Remove V9+ embedded files/private fields and V10+ jumper, rounded-rectangle, and ellipse syntax; remove `embedded_fonts`, font `face`, symbol/property formatting fields; add legacy property IDs and move property visibility into `effects`; convert font style and pin-visibility booleans to older atom syntax. |
| Schematic | Remove V9+ tables, rule areas, embedded files/private fields and V10+ rounded-rectangle, variant, body-style, and ellipse/net-chain syntax; remove text and sheet simulation/assembly flags, symbol/property formatting fields, font `face`; add legacy property IDs and move property visibility into `effects`; convert font and pin visibility booleans to older atom syntax; remove root `group` nodes. |
| Board / footprint | Remove V9+ tables, tenting, embedded files/fonts, component classes, complex padstacks, via stacks, rule areas, via protection, arbitrary user-layer qualifiers, custom layer counts, rounded rectangles, PCB points, barcodes, backdrill/post-machining, variants, and current-development features. Also remove graphic/track soldermask margin/layer fields, table-cell angle, text render caches, textbox/table-cell/layer knockout, model `hide`, font `face`, and add legacy numeric netcodes. `solder_paste_margin_ratio` is renamed to `solder_paste_ratio`. |
| Project side files | Generate legacy numeric-ID `.kicad_prl` display settings for V8 boards. |

#### KiCad 7 Target

KiCad 7 targets remove KiCad 8/9/10/current syntax and apply additional parser
compatibility rewrites around PCB fields, UUIDs, and footprint data:

| Kind | Implemented handling |
| --- | --- |
| Symbol library | Remove V8+ `generator_version`, embedded fonts/files, V9 private fields, V10 jumper/rounded/ellipse syntax, symbol `exclude_from_sim`, position-file and property formatting fields, jumper/power class flags, and font `face`; add legacy property IDs; move property visibility into `effects`; convert font and visibility booleans to atom syntax. |
| Schematic | Remove V8+ `generator_version` and `fields_autoplaced`, V9+ tables/rule areas/embedded/private fields, V10+ rounded/variant/body-style syntax, post-target simulation exclusion fields, sheet assembly/simulation flags, symbol/property formatting fields, font `face`, and root `group` nodes. UUID atoms are unquoted for KiCad 6/7 parsers, and property visibility/IDs are downgraded to legacy placement. |
| Board / footprint | Remove V8+ generated objects, teardrops, tables, embedded files/fonts, component classes, pad/via stacks, rule areas, via protection, and newer target syntax. Convert user-layer type qualifiers to `user`; remove graphic/track soldermask fields, table angles, render caches, knockout flags, model `hide`, graphic net connectivity, group locked fields, via layer-connection fields, footprint jumper/net-tie/unit fields, font `face`, and legacy-incompatible footprint attr atoms. Convert PCB footprint properties back to `fp_text`, rename `uuid`/`id` back to `tstamp`/`id` legacy forms, rename solder-paste and thermal fields, convert strokes to legacy `width`, convert dimensions to visible graphics, downgrade booleans/presence atoms, and rebuild numeric netcodes. |
| Project side files | Generate legacy numeric-ID `.kicad_prl` display settings for V7 boards. |

### Document Detection and Project Handling

The C++ implementation detects KiCad document kind primarily from the root
S-expression head:

| Root head | Kind |
| --- | --- |
| `kicad_symbol_lib` | Symbol library |
| `kicad_sch` | Schematic |
| `kicad_pcb` | Board |
| `footprint` | Footprint |
| `kicad_dru` | Design rules |
| `kicad_wks`, `drawing_sheet` | Worksheet |

If the root head is missing or unknown, it falls back to file extension:
`.kicad_sym`, `.kicad_sch`, `.kicad_pcb`, `.kicad_mod`, `.kicad_dru`, and
`.kicad_wks`. Legacy `.sch`, `.lib`, `.dcm`, and `.pro` are also detected as
legacy KiCad kinds, but direct conversion from those legacy file families is not
implemented in the current phase.

When converting a project directory or `.kicad_pro`, it copies only editable
KiCad project inputs and common local 3D model files. Generated outputs,
history/backup folders, Gerbers, fabrication outputs, BOMs, and temporary files
are skipped. For KiCad 6, 7, and 8 board targets it also creates legacy
`.kicad_prl` local board display settings with numeric `visible_items`, full
`visible_layers`, and the older local-settings meta version so converted objects
remain visible in older GUIs. For KiCad 6 project targets it additionally
removes top-level `version` nodes from `sym-lib-table` / `fp-lib-table` and
rebuilds root-level schematic hierarchy instance tables across child sheets.

### Symbol Library Rules

Generic parser gates remove these introduced nodes when the target file format
is older than the introduction version:

| Introduced | Removed heads | Reason |
| ---: | --- | --- |
| 20220126 | `text_box`, `textbox` | Symbol text boxes |
| 20240529 | `embedded_files`, `embedded_file` | Embedded files |
| 20241209 | `private` | Private SCH_FIELD flags |
| 20250324 | `pin_group`, `pin_groups` | Jumper pin groups |
| 20250829 | `rounded_rectangle`, `roundrect` | Rounded rectangles |
| 20260508 | `ellipse`, `ellipse_arc` | Native ellipse primitives |

Compatibility rewrites:

| Target cutoff | Rewrite |
| ---: | --- |
| `< 20231120` | Remove root `generator_version` fields |
| `< 20241209` | Remove `embedded_fonts`; add legacy property IDs; move property `hide` flags into `effects` |
| `< 20230409` | Remove symbol-library `symbol/exclude_from_sim` simulation exclusion flags |
| `< 20240108` | Convert font `(bold yes/no)` and `(italic yes/no)` lists to legacy presence atoms |
| `<= 20241209` | Remove font `face` fields |
| `< 20241004` | Convert boolean `hide` lists to legacy atoms; flatten `pin_names` / `pin_numbers` hide lists |
| `<= 20211014` | Add KiCad 6 standard property IDs: `Reference=0`, `Value=1`, `Footprint=2`, `Datasheet=3`, `ki_keywords=4`, `ki_description=5`, `ki_fp_filters=6` |
| `< 20251024` | Remove symbol `in_pos_files`; remove property `show_name` and `do_not_autoplace` |
| `< 20250324` | Remove `duplicate_pin_numbers_are_jumpers` |
| `< 20250227` | Remove symbol `power` class flags |

### Schematic Rules

Generic parser gates:

| Introduced | Removed heads | Reason |
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

Compatibility rewrites:

| Target cutoff | Rewrite |
| ---: | --- |
| `< 20231120` | Remove `generator_version`; remove `fields_autoplaced` from symbols and sheets |
| `< 20260326` | Remove schematic `locked` fields introduced after the target |
| `< 20260306` | Remove `embedded_fonts`; remove sheet `exclude_from_sim`, `in_bom`, `on_board`, `dnp`; remove root schematic `group` nodes |
| `< 20250827` | Remove `body_styles` and `body_style` |
| `< 20250114` | Remove text/textbox `exclude_from_sim` |
| `<= 20230121` | Remove all remaining `exclude_from_sim` |
| `< 20220822` | Remove text, label, and directive-label `hyperlink` fields |
| `< 20220914` | Remove placed-symbol `dnp` flags |
| `< 20220124` | Rename root `directive_label` nodes back to `netclass_flag` |
| `< 20251024` | Remove symbol `in_pos_files` |
| `< 20250324` | Remove `duplicate_pin_numbers_are_jumpers` |
| `< 20250227` | Remove symbol `power` class flags |
| `< 20241004` | Convert boolean `hide` lists to legacy atoms; flatten pin visibility hide lists |
| `<= 20211123` | Remove library-symbol `pin/alternate` definitions |
| `< 20240108` | Convert font bold/italic boolean lists to legacy atoms |
| `<= 20250114` | Remove font `face` fields |
| `<= 20230121` | Unquote `uuid` atoms for KiCad 6/7 parsers |
| `<= 20211123` | Generate KiCad 6 root-level `sheet_instances` and `symbol_instances` when the source root schematic already has instance data; child sheets are not given root instance tables |
| `<= 20211123` | Add KiCad 6 standard schematic property IDs and normalize sheet property names/IDs to `Sheet name=0` and `Sheet file=1` |
| `<= 20211123` | Remove symbol-internal `instances` blocks after the KiCad 6 root instance table has been generated |
| `< 20241209` | Add legacy property IDs; move property `hide` flags into `effects` |
| `< 20251028` | Remove property `show_name` and `do_not_autoplace` |

### Board and Footprint Rules

Generic parser gates:

| Introduced | Removed heads | Reason |
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

Current development coverage notes from local KiCad `10.99.0-1273-gd90e32b6a0`:

| Introduced | Handling | Notes |
| ---: | --- | --- |
| 20260521 | Implemented | Pad child `sim_electrical_type` is removed for targets older than `20260521`. |
| 20260603 | Implemented | Table-cell child `knockout` is removed contextually for targets older than `20260603`; `knockout` is not used as a global token gate because other object types also use it. |

Compatibility rewrites:

| Target cutoff | Rewrite |
| ---: | --- |
| `< 20260410` | Remove typed/extruded 3D model blocks by removing `model` nodes that contain `type` |
| `< 20260513` | Replace copper thieving zone fill mode with polygon fill |
| `>= 20220225` | Remove obsolete footprint `tedit` fields |
| `>= 20200628` | Remove obsolete board `visible_elements` settings |
| `< 20260603` | Remove PCB table-cell `knockout` fields |
| `< 20240703` | Convert user-layer type qualifiers `front`, `back`, `auxiliary` to `user` |
| `< 20241010` | Remove graphic `solder_mask_margin` fields |
| `< 20241030` | Convert dimension boolean fields to legacy atoms; remove dimension `arrow_direction` |
| `< 20250210` | Remove PCB text `render_cache`; remove textbox `knockout`; remove `knockout` atoms from layer lists; add `filled_areas_thickness no` to cached zone fills where needed |
| `< 20241009` | Remove zone `placement` fields |
| `<= 20221018` | Remove zone `attr`; remove pad/zone `thermal_bridge_angle`; rename pad/zone `thermal_bridge_width` to legacy `thermal_width` |
| `< 20240108` | Remove `setup/allow_soldermask_bridges_in_footprints`; remove group `locked`; remove via layer-connection fields such as `keep_end_layers`, `start_end_only`, and `zone_layer_connections` |
| `< 20241007` | Remove track `solder_mask_margin` and `solder_mask_layer` fields |
| `< 20240617` | Remove PCB table cell `angle` |
| `< 20260521` | Remove pad `sim_electrical_type` |
| `< 20250228` | Convert tenting front/back boolean lists to legacy atoms; remove IPC-4761 protection fields |
| `< 20231212` | Convert `locked` and `hide` boolean lists to presence atoms; remove `unlocked`; remove model `hide` |
| `< 20231014` | Remove `generator_version` |
| `< 20230924` | Convert `pcbplotparams` `yes/no` booleans to `true/false`; convert shape fill `no` to `none` |
| `< 20230730` | Remove graphic shape `net` connectivity |
| `< 20240108` | Convert font bold/italic boolean lists to legacy atoms |
| `< 20230620` | Convert footprint `Reference` and `Value` properties back to `fp_text`; convert `Description` to `ki_description`; map `sheetname`/`sheetfile` to properties |
| `< 20231231` | Rename scoped `uuid` fields back to `tstamp`; rename group/generated `uuid` back to `id` |
| `< 20250324` | Remove footprint jumper pad fields: `duplicate_pad_numbers_are_jumpers` and `jumper_pad_groups` |
| `<= 20221018` | Remove footprint `dnp` attributes, `net_tie_pad_groups`, `units`, and `allow_missing_courtyard`; remove pad/via `remove_unused_layers`; convert dimensions to visible graphics; remove legacy-incompatible `locked`; downgrade free via fields; convert PCB graphic `stroke` blocks to legacy `width` fields |
| `< 20250309` | Remove `component_class` from placement rules |
| `< 20250222` | Convert PCB hatch/reverse-hatch/cross-hatch shape fills to solid fill |
| `<= 20241229` | Remove PCB font `face` fields |
| `< 20251101` | Remove pad/via post-machining fields |
| `< 20251028` | Rebuild legacy numeric board netcodes and root-level net declarations |

KiCad 6 parser-specific fixes observed in project-level tests:

| Area | Implemented fix |
| --- | --- |
| PCB setup | Remove `setup/allow_soldermask_bridges_in_footprints` for pre-8 board targets. |
| PCB footprints | Remove `net_tie_pad_groups`, `units`, jumper pad groups, and `allow_missing_courtyard` attr atoms for KiCad 6/7 board targets. |
| PCB zones and pads | Remove zone `attr`, remove `thermal_bridge_angle`, and rename `thermal_bridge_width` to `thermal_width` for KiCad 6/7 board targets. |
| PCB text and tables | Remove text `render_cache`, textbox `knockout`, table-cell `knockout`, and layer-list `knockout` where older parsers reject them. |
| Symbol libraries | Remove symbol `exclude_from_sim` for targets older than `20230409` and add KiCad 6 standard property IDs. |
| Schematics | Remove pin `alternate`, generate KiCad 6 root instance tables, normalize root-sheet instance paths, normalize sheet property names/IDs, and remove symbol-internal `instances`. Placed symbol pin UUID blocks are intentionally retained because KiCad 6 uses them for instance association. |
| Project side files | Generate numeric-ID `.kicad_prl` display settings for V6/V7/V8 and remove library-table top-level `version` nodes for V6. |

### Worksheet and Design Rules

Worksheet handling currently has one implemented parser gate:

| Target cutoff | Rewrite |
| ---: | --- |
| `< 20220228` | Remove worksheet `font` blocks |

Design rules are detected and have target version aliases, but no downgrade
rewrites are currently implemented because the file format version macro remains
`20200610` across the tracked KiCad versions.

### Warning and Report Semantics

Every implemented removal or compatibility rewrite that changes the tree adds a
warning. Generic feature gates report the number of removed nodes and the
introduction version. Reports include path, detected kind, source version,
target version, changed flag, and warnings.

## Converter Requirements

### Read Path

- Preserve the source file `version`; do not interpret only as the current
  KiCad format.
- Support compatibility aliases for older files:
  - `page` to `paper`
  - Legacy overbar `~...~` to `~{...}`
  - Old `start/end` text box format to new `at/size`
  - Old `id` to `uuid`
  - Old boolean / presence-token formats to explicit booleans
- Detect future formats and return a clear error or a defined downgrade strategy.

### Write Path

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
- Lossy downgrades should produce warnings or sidecar metadata instead of silent
  deletion.

### Test Path

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

## Maintenance Notes

When adding future version differences:

1. Add or update the version matrix first.
2. Add a new interval section such as `10.0 to 11.0` or
   `10.99 / current to 11.99 / current`.
3. Keep development-branch findings separate from released stable tags until the
   corresponding KiCad release is tagged.
4. Update the backport target summary when a new source version introduces
   constructs that affect existing downgrade targets.
5. Track `.kicad_pro` JSON schema migrations in a separate document.

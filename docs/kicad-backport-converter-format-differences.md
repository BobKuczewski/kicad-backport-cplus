# KiCad Backport Converter Format Differences and Conversion Behavior

This document describes the KiCad file-format differences that are actually handled by the current `kicad-backport` implementation: file families, version anchors, conversion dispatch, rewrite rules, and known lossy paths. The canonical KiCad file-format reference is the KiCad developer documentation:

- https://dev-docs.kicad.org/en/file-formats/index.html

## Quick Read for KiCad Developers

| Topic | Converter behavior |
| --- | --- |
| Format detection | Modern files are identified primarily by S-expression root tokens: `kicad_sch`, `kicad_symbol_lib`, `kicad_pcb`, `footprint`, and `kicad_wks` / `drawing_sheet`. The filename extension is a fallback. |
| Version detection | Modern S-expression files read the top-level `(version ...)` field. `.kicad_pro` is reported as `kicad-project-json`. Legacy `.sch/.lib/.dcm/.pro` files are reported with legacy aliases. |
| KiCad 5/6 boundary | KiCad 6 is the file-family boundary for schematics, symbol libraries, and project files. KiCad 4/5 `.sch/.lib/.pro` and KiCad 6+ `.kicad_sch/.kicad_sym/.kicad_pro` are different syntax families. |
| PCB and footprints | KiCad 4-10 board and footprint files are handled as S-expressions. The relevant differences are version anchors, node sets, and field syntax. |
| `.kicad_pro` | Modern project JSON is not generally rewritten per target KiCad major version. For KiCad 6/7/8 targets, project-level embedded worksheet URI references are cleared because those versions do not resolve `kicad-embed://...kicad_wks` page-layout paths. For KiCad 5/4 targets it is converted to a legacy `.pro`. |
| `.kicad_wks` | Worksheets are detected and can have their version field rewritten. The current code has only a small worksheet downgrade rule and no KiCad 4/5 legacy worksheet writer. |
| `.kicad_dru` | Design-rule files are detected and copied only when the target supports the same `.kicad_dru` anchor. Targets without `.kicad_dru` support skip the file with a warning. |
| Conversion granularity | Same-family modern files are rewritten at the S-expression AST level. Legacy/modern crossings use dedicated readers and writers. Project-directory conversion also repairs library tables, embedded symbols, hierarchy instances, project-local settings, and legacy schematic cache libraries. |

## Implementation Overview

| Area | Current implementation |
| --- | --- |
| Target aliases | Supports `4.0`, `5.0`, `5.1`, `6.0`, `7.0`, `8.0`, `9.0`, `10.0`, and `10.99`. Forms such as `v9`, `kicad-9`, and `9` are accepted. |
| Numeric target versions | A numeric `--target-version` is treated as a raw S-expression version. If the source version is lower than that numeric target, the implementation keeps the source version to avoid an unsupported arbitrary upgrade. |
| Kind detection | S-expression roots are preferred over extensions. `.kicad_pro` and legacy files are handled as text paths, not parsed as S-expressions. |
| Same-version files | If source and target S-expression versions are equal, the file is copied or left unchanged; rewrite rules are not run. |
| Same-family upgrade | If the source S-expression version is lower than the target, `ApplyUpgradeRules()` performs conservative syntax normalization. |
| Same-family downgrade | If the source S-expression version is higher than the target, `ApplyDowngradeRules()` removes, renames, flattens, or approximates nodes that older KiCad parsers cannot read. |
| Legacy crossing | `.sch`, `.lib`, `.dcm`, and `.pro` convert to modern families for KiCad 6+ targets. Modern schematics, symbol libraries, and projects convert back to legacy families for KiCad 5/4 targets. |
| Project-directory conversion | Editable project inputs are copied, generated/manufacturing/backup directories are skipped, document files are converted individually, and project-local support files are normalized. |

`.kicad_dru` is detected as `design-rules` and has the fixed anchor `20200610`. Project conversion copies it for targets that resolve to the same design-rule anchor and reports a warning when the requested target family does not support `.kicad_dru`.

## Conversion Pipeline

| Stage | Code behavior | Intent | Resulting documentation meaning |
| --- | --- | --- | --- |
| Read | `loadDocumentImpl()` reads text, then routes `.kicad_pro` and legacy files by extension; other files are parsed as S-expressions. | Avoid parsing JSON or legacy text as S-expressions. | Project JSON and legacy formats must be documented as separate paths. |
| Detect kind | `DetectKind()` prefers root tokens and falls back to extensions. | Allow correctly rooted S-expression files even with unusual filenames. | Modern file kind is root-driven; extension mainly controls fallback and output naming. |
| Resolve target | `ResolveTargetVersion()` maps each KiCad alias to a per-kind format version. | KiCad releases do not use one shared format version across all file types. | Version tables must be per file family. |
| Choose output extension | `withTargetFamilyExtension()` switches `.sch/.lib/.pro` and `.kicad_sch/.kicad_sym/.kicad_pro` at the KiCad 5/6 boundary. | Reflect the real syntax-family boundary. | KiCad 5/6 conversion is not a simple `(version ...)` edit. |
| Fast copy | Equal S-expression versions are copied or left unchanged. Modern `.kicad_pro` to KiCad 6+ is copied, then project-level compatibility post-processing may clear unsupported embedded worksheet page-layout references for KiCad 6/7/8. | Avoid unnecessary formatting churn while still fixing known project-load failures. | Most unchanged files are not normalized; project JSON has a narrow compatibility cleanup path. |
| Legacy target | For target major `<=5`, modern schematic/symbol/project files use legacy writers; board/footprint files stay S-expression. | KiCad 4/5 schematic, symbol, and project formats are legacy text formats. | `.kicad_sch -> .sch`, `.kicad_sym -> .lib/.dcm`, and `.kicad_pro -> .pro` are object-level writes. |
| Modern target | For target major `>5`, legacy inputs use legacy parsers and modern writers. | Promote KiCad 4/5 files into KiCad 6+ file families. | Legacy-to-modern conversion is mapped, not text replacement. |
| Upgrade rules | Source S-expression version lower than target runs `ApplyUpgradeRules()`. | Upgrade only syntax that can be derived from existing source data. | New KiCad design intent is not invented. |
| Downgrade rules | Source S-expression version higher than target runs `ApplyDowngradeRules()`. | Keep older parsers away from unknown nodes and fields. | Downgrades can be lossy and should emit warnings. |
| Version writeback | `ensureVersion()` inserts or updates top-level `(version ...)`. | Make the emitted file declare the resolved target version. | The final version is the converter-resolved anchor, not necessarily the source value. |
| Formatting | Modern S-expressions are written through `SEXPR::Format()`. JSON and legacy text are copied or emitted by their writers. | Format only the syntax family that the S-expression formatter understands. | Modern files may have formatting churn; JSON/legacy behavior depends on their path. |
| Report | Each file gets a `FILE_REPORT` with source/target kind, source/target version, change status, and warnings. | Make lossy and compatibility rewrites visible to CLI and JSON reports. | Documented loss cases correspond to emitted warning semantics. |

## Strategy Layers

| Layer | Handles | Strategy | Typical code entry points |
| --- | --- | --- | --- |
| File-family layer | Legacy vs modern extensions, root tokens, project JSON | Decide copy vs cross-family writer vs S-expression rules. | `loadDocumentImpl()`, `DetectKind()`, `withTargetFamilyExtension()`, `normalizeFile()` |
| Version-anchor layer | Aliases from `4.0` through `10.99`, plus numeric versions | Map aliases to file-kind-specific format versions. | `ResolveTargetVersion()`, `TargetMajorVersion()`, `DisplayVersionAlias()` |
| Legacy mapping layer | `.sch/.lib/.dcm/.pro` and modern equivalents | Parse what can be represented, write the target text structure, warn on loss. | `ConvertLegacyToSexprText()`, `ConvertSexprToLegacyText()`, `RewriteLegacyTextForTarget()` |
| S-expression rule layer | `.kicad_sch/.kicad_sym/.kicad_pcb/.kicad_mod/.kicad_wks` | Remove, rename, expand, flatten, or approximate nodes by target version thresholds. | `ApplyUpgradeRules()`, `ApplyDowngradeRules()` |
| Project repair layer | Project directories, library tables, local settings, hierarchy instances, embedded symbols, legacy cache libraries, project page-layout references | After file conversion, add the support data needed for the converted project to open. | `copyProjectTree()`, `ensureProjectLocalSymbolLibraryTable()`, `embedProjectLocalSchematicSymbols()`, `rebuildKiCad6ProjectHierarchyInstances()`, `ensureLegacySchematicCacheLibrary()`, `ensureLegacyProjectPageLayoutRefs()` |

## Lossy Conversion Rules

| Condition | Implementation choice | Reason | Examples |
| --- | --- | --- | --- |
| Target cannot parse a node | Remove the node or field and emit a warning. | Older KiCad parsers fail or behave unpredictably on unknown syntax. | `embedded_files`, `variants`, `barcodes`, `net_chains`, native ellipse nodes. |
| Target has an approximate representation | Rename, flatten, or rewrite to legacy fields. | Preserve visible/electrical meaning where possible. | `directive_label -> netclass_flag`, `stroke -> width`, `paper/page`, `uuid/tstamp/id`. |
| Target has weaker geometry | Convert to older geometry or simpler objects. | Old formats have no equivalent primitive. | PCB rectangles to lines, track arcs to segments, custom/roundrect pads to rectangular pads. |
| Target uses older property layout | Move properties, add IDs, remove new fields, or normalize old property names. | KiCad 6/7/8 differ in property IDs, `hide`, sheet property names, and font boolean syntax. | Property hide moves between `effects` and property-level `hide`; standard property IDs are added or removed; KiCad 6/7 sheet properties are normalized. |
| Target parser accepts fewer fill or drawing forms | Replace unsupported fills or remove unsupported schematic drawing primitives. | KiCad 6 schematic parsers reject symbol fill colors and root-level schematic drawing primitives emitted by newer KiCad versions. | `(fill (type color) ...)` is downgraded to `background`; root-level `rectangle`, `circle`, `arc`, `polyline`, and `bezier` nodes are removed for KiCad 6 schematics. |
| Source has no new semantics | Do not synthesize new objects. | The converter cannot infer user design intent. | Upgrades do not create padstacks, variants, component classes, or barcodes. |
| Legacy cannot represent a modern object | Emit the expressible subset and warn. | KiCad 4/5 `.sch/.lib` capabilities are much smaller than modern S-expressions. | Modern symbol graphics, properties, instances, and complex schematic objects are lossy when written to legacy formats. |

## File Families and Extensions

| File family | Modern extension | Legacy extension | Internal kind | Conversion behavior |
| --- | --- | --- | --- | --- |
| Project | `.kicad_pro` | `.pro` | `project` / `legacy-project` | `.kicad_pro` is handled as raw JSON text; legacy `.pro` can become minimal KiCad 6+ JSON; modern JSON can become a KiCad 4/5 `.pro` with `[general]`, `[eeschema]`, and `[eeschema/libraries]` sections. |
| Schematic | `.kicad_sch` | `.sch` | `schematic` / `legacy-schematic` | KiCad 6+ is S-expression; KiCad 4/5 is legacy Eeschema text. Crossings use dedicated parsers and writers. |
| Symbol library | `.kicad_sym` | `.lib`, `.dcm` | `symbol-library` / `legacy-symbol-library` / `legacy-symbol-documentation` | `.lib` and `.dcm` can each produce `.kicad_sym`; modern `.kicad_sym` to legacy writes `.lib` plus a `.dcm` sidecar. |
| Board | `.kicad_pcb` | none | `board` | KiCad 4-10 are S-expression files and are converted with version and rule rewrites. |
| Footprint | `.kicad_mod` | none | `footprint` | Shares the board rule set and remains S-expression across supported targets. |
| Worksheet | `.kicad_wks` | no implemented legacy writer | `worksheet` | Version rewrite is supported; only a small worksheet downgrade rule exists. |
| Design rules | `.kicad_dru` | none | `design-rules` | Copied only when the target supports the same fixed design-rule anchor; skipped with a warning for targets without `.kicad_dru` support. |
| Library tables | `sym-lib-table`, `fp-lib-table` | same names | `library-table` report item | Normalized during project conversion: version field, library type/URI, and local footprint aliases. |
| Local runtime state | `.kicad_prl` | omitted for KiCad 5/4 | not a primary document | Generated or normalized for KiCad 6/7/8 targets; skipped for KiCad 5/4 targets. |

## Target Version Anchors

| KiCad target | Symbol library `.kicad_sym` | Schematic `.kicad_sch` | Board `.kicad_pcb` | Footprint `.kicad_mod` | Worksheet `.kicad_wks` | Notes |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `4.0` | legacy `.lib` 2.3 | legacy `.sch` V2 | `4` | `4` | undefined | Only board/footprint have S-expression targets; schematic and symbol use legacy writers. |
| `5.0` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | undefined | KiCad 5.0 and 5.1 share board/footprint anchors in this implementation. |
| `5.1` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | undefined | Same anchors as `5.0`. |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | Modern schematic and symbol file-family starting point. |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | Modern S-expression extensions. |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `generator_version`, UUID/id, and PCB field compatibility boundary. |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | Embedded data, tables, rule areas, and complex PCB-object boundaries. |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | Highest regular target alias currently supported by the code. |
| `10.99` | `20251024` | `20260306` | `20260603` | `20260603` | `20231118` | Development alias; only board/footprint advance beyond `10.0`. |

## Version-Line Differences

| Version line | Project | Schematic | Symbol library | Board / footprint | Worksheet | Implementation notes |
| --- | --- | --- | --- | --- | --- | --- |
| KiCad 4 | `.pro` | `.sch` V2 | `.lib` 2.3 + `.dcm` 2.0 | `4` | undefined | Legacy schematic/symbol rewrites only change headers; board/footprint downgrades remove KiCad 4-incompatible fields such as differential-pair constraints, keepouts, and custom pads. |
| KiCad 5/5.1 | `.pro` | `.sch` V4 | `.lib` 2.4 + `.dcm` 2.0 | `20171130` | undefined | Modern schematic/symbol/project files downgrade to legacy writers; board/footprint remain KiCad 5 S-expression. |
| KiCad 6 | `.kicad_pro` JSON | `20211123` | `20211014` | `20211014` | `20210606` | Modern family start. KiCad 6 downgrades add parser compatibility fixes for schematic properties and instances. |
| KiCad 7 | `.kicad_pro` JSON | `20230121` | `20220914` | `20221018` | `20220228` | Boundaries include text boxes, DNP, simulation-related fields, stroke syntax, images, net ties, and teardrops. |
| KiCad 8 | `.kicad_pro` JSON | `20231120` | `20231120` | `20240108` | `20231118` | Boundaries include `generator_version`, PCB footprint fields, and UUID/id normalization. |
| KiCad 9 | `.kicad_pro` JSON | `20250114` | `20241209` | `20241229` | `20231118` | Boundaries include embedded files/fonts, tables, rule areas, component classes, pad/via stacks, and font face fields. |
| KiCad 10 | `.kicad_pro` JSON | `20260306` | `20251024` | `20260206` | `20231118` | Boundaries include variants, jumper/power/position flags, body styles, groups/locked fields, via protection, backdrill, barcodes, net chains, and native ellipses. |
| KiCad 10.99 | `.kicad_pro` JSON | `20260306` | `20251024` | `20260603` | `20231118` | Only board/footprint advance to `20260603`; development-only additions such as table-cell `knockout` are handled by implemented rules only. |

## Conversion Dispatch

| Input type | Target | Output/processing | Target version reported | Loss or limitation |
| --- | --- | --- | --- | --- |
| legacy `.sch` | `4.0`/`5.0`/`5.1` | Keep `.sch`; rewrite `EESchema Schematic File Version 2/4` header. | `legacy-sch-v2` or `legacy-sch-v4` | Records are preserved; version-specific contents are not deeply simplified. |
| legacy `.lib` | `4.0`/`5.0`/`5.1` | Keep `.lib`; rewrite `EESchema-LIBRARY Version 2.3/2.4` header. | `legacy-lib-2.3` or `legacy-lib-2.4` | Records are preserved; no deep feature downgrade. |
| legacy `.dcm` | `4.0`/`5.0`/`5.1` | Rewrite original text. | `legacy-dcm-2.0` | No content-level migration. |
| legacy `.pro` | `4.0`/`5.0`/`5.1` | Rewrite original text. | `legacy-pro` | No content-level migration. |
| legacy `.sch` | `6.0+` | Convert to `.kicad_sch`. | Target schematic anchor. | Non-wire drawing items are not fully mapped. |
| legacy `.lib` | `6.0+` | Convert to `.kicad_sym`. | Target symbol-library anchor. | Drawing primitives and pins are converted with loss warnings. |
| legacy `.dcm` | `6.0+` | Convert component names into a `.kicad_sym` skeleton. | Target symbol-library anchor. | Descriptions/keywords/doc links are not fully mapped. |
| legacy `.pro` | `6.0+` | Convert to minimal `.kicad_pro` JSON. | `kicad-project-json` | Only recognized legacy settings and library names are preserved. |
| `.kicad_pro` | `6.0+` | Copy raw JSON; for KiCad 6/7/8 project conversion, clear embedded worksheet page-layout URI values. | `kicad-project-json` | General JSON content is not rewritten per target version; only unsupported `kicad-embed://...kicad_wks` project page-layout references are cleared for KiCad 6/7/8. |
| `.kicad_pro` | `4.0`/`5.0`/`5.1` | Convert to `.pro` with legacy project sections. | `legacy-pro` | Restores `legacy.project_settings` and legacy symbol library names where available; otherwise defaults are used. |
| `.kicad_sch` | `4.0`/`5.0`/`5.1` | Convert to `.sch`; project conversion also adds a schematic cache library reference. | `legacy-sch-v2` or `legacy-sch-v4` | Modern objects, properties, instances, and graphics are reduced to legacy records; multiline text is emitted as single-line escaped `\n` text for old Eeschema parsers. |
| `.kicad_sym` | `4.0`/`5.0`/`5.1` | Convert to `.lib` and write `.dcm`; project conversion also copies the generated `.lib` to `<project>-cache.lib`. | `legacy-lib-2.3` or `legacy-lib-2.4` | Modern properties, graphics, and nested symbol relationships are approximated; legacy `DEF` reference fields use reference prefixes rather than instance references. |
| `.kicad_pcb` / `.kicad_mod` | Defined targets | Keep S-expression family; rewrite version and nodes/fields. | Board/footprint anchor. | Unsupported geometry/electrical/manufacturing/assembly/cache fields are removed or approximated. |
| `.kicad_wks` | `6.0+` | Keep `.kicad_wks`; rewrite version and apply worksheet rules. | Worksheet anchor. | Only `font` removal for `<20220228` is currently implemented. |
| `.kicad_dru` | supported design-rule targets | Copy if the fixed source anchor matches the resolved target anchor. | `6.0/7.0/8.0/9.0/10.0/10.99 (1)` display alias. | No design-rule format conversion is implemented; unsupported targets skip the file with a warning. |

## Schematic Rules

| Boundary | Implemented behavior | Effect |
| --- | --- | --- |
| legacy `.sch` -> KiCad 6+ | Parses `$Descr`, `$Comp`, fields, wires/buses, bus entries, labels, text, sheets, sheet pins, junctions, and no-connects. | Emits `.kicad_sch` with modern nodes, UUID/path data, properties, and instances; incomplete legacy drawing mapping is warned. |
| KiCad 6+ -> legacy `.sch` | Writes schematic header, `LIBS:`, `$Descr`, components, fields, wires/buses, entries, labels/text, sheets, sheet pins, junctions, and no-connects. Multiline text is written as escaped `\n` on one legacy text line. | Modern structure is compressed into legacy records while avoiding bare continuation lines that KiCad 4 treats as invalid objects. |
| Upgrade `>=20240108` | Expand font `bold`/`italic` atoms into boolean lists. | KiCad 8+ font syntax. |
| Upgrade `>=20241004` | Expand `pin_names` / `pin_numbers` `hide` atoms. | New pin visibility syntax. |
| Upgrade `>=20241209` | Move property hide out of `effects`. | KiCad 9+ property layout. |
| Downgrade `<20260326` | Remove descendant `locked` fields. | Removes KiCad 10-era locked fields. |
| Downgrade `<20260306` | Remove root `uuid`, `embedded_fonts`, sheet assembly/simulation fields, and top-level `group`. | KiCad 10 schematic rollback and pre-10 parser compatibility. |
| Downgrade `<20250827` | Remove `body_styles` / `body_style`. | Custom body styles are discarded. |
| Downgrade `<20250114` | Remove text/textbox `exclude_from_sim`. | KiCad 9/10 text simulation field rollback. |
| Downgrade `<20241209` | Add legacy property IDs and move hide into `effects`. | KiCad 8-and-earlier property layout. |
| Downgrade `<20241004` | Flatten pin visibility boolean lists and legacy `hide` bools. | Older symbol visibility syntax. |
| Downgrade `<20231120` | Remove `generator_version` and symbol/sheet `fields_autoplaced`. | KiCad 8 metadata rollback. |
| Downgrade `<=20230121` | Remove `exclude_from_sim`; normalize `uuid` atom quoting; normalize KiCad 6/7 sheet property names and IDs; remove placed symbol pin UUID blocks. | KiCad 7 and older parser compatibility. |
| Downgrade `<20220914` | Remove symbol `dnp`. | Pre-DNP schematic compatibility. |
| Downgrade `<20220822` | Remove text and label `hyperlink`. | Older text/label syntax. |
| Downgrade `<20220124` | Rename `directive_label` to `netclass_flag`. | Early KiCad 6 compatibility. |
| Downgrade `<=20211123` | Remove pin `hide` and `alternate`; generate KiCad 6 symbol instances; add standard property IDs; remove root-level schematic drawing primitives; downgrade schematic fill colors; remove `instances`. | KiCad 6 parser compatibility fixes. |
| Feature gates | Remove target-too-new nodes such as text boxes, simulation model nodes, tables, rule areas, embedded files, private fields, rounded rectangles, variants, ellipses, and net chains. | Unknown syntax is removed with warnings. |

## Symbol Library Rules

| Boundary | Implemented behavior | Effect |
| --- | --- | --- |
| legacy `.lib` -> KiCad 6+ | Parses `DEF`, `ALIAS`, fields, `DRAW`, and `X` pins. | Emits `.kicad_sym`; complex drawing/pin cases are warned as lossy or incomplete. |
| legacy `.dcm` -> KiCad 6+ | Parses `$CMP` names. | Emits a `.kicad_sym` skeleton; documentation metadata is not fully mapped. |
| KiCad 6+ -> legacy `.lib/.dcm` | Writes library header, `DEF`, `ALIAS`, standard/custom `F` fields, `DRAW`, `X` pins, and `.dcm` `D/K/F` records. `DEF` reference fields are normalized to prefixes such as `U`, `BT`, or `#PWR`, not instance references such as `U1`. | Modern symbol data is compressed to legacy records and remains resolvable by old Eeschema library loaders. |
| Upgrade `>=20240108` | Expand font style atoms. | KiCad 8+ font syntax. |
| Upgrade `>=20241004` | Expand pin visibility atoms. | New visibility syntax. |
| Upgrade `>=20241209` | Move property hide out of `effects`. | KiCad 9+ property layout. |
| All upgrades | Remove legacy standard property IDs. | Avoid carrying old standard property IDs into newer targets. |
| Downgrade `<20231120` | Remove `generator_version`. | KiCad 8 metadata rollback. |
| Downgrade `<20241209` | Remove `embedded_fonts`; add legacy property IDs; move hide into `effects`. | KiCad 8-and-earlier property layout. |
| Downgrade `<20230409` | Remove symbol `exclude_from_sim`. | Simulation-exclusion rollback. |
| Downgrade `<20240108` | Downgrade font boolean lists to atoms. | Older font syntax. |
| Downgrade `<=20211014` | Remove pin `hide`; add KiCad 6 standard property IDs; downgrade symbol-library fill colors. | KiCad 6 initial symbol-library compatibility. |
| Downgrade `<20251024` | Remove symbol `in_pos_files`; remove property `show_name` and `do_not_autoplace`. | KiCad 10 property/positioning rollback. |
| Downgrade `<20250324` | Remove `duplicate_pin_numbers_are_jumpers`. | Jumper pin-number rollback. |
| Downgrade `<20250227` | Remove symbol `power`. | Power class rollback. |
| Feature gates | Remove target-too-new text boxes, embedded files, private fields, pin groups, rounded rectangles, and ellipse nodes. | Unknown syntax is removed with warnings. |

## Board and Footprint Rules

| Boundary | Implemented behavior | Effect |
| --- | --- | --- |
| General upgrade | Remove legacy `host`; rename `page` to `paper`; convert legacy arc angles to midpoint arcs; remove legacy line `angle`; tag cached zone fills as polygon fills and add layers. | Converts KiCad 4/5-style PCB syntax toward modern S-expression syntax. |
| Upgrade `>=20231231` | Rename `tstamp` to `uuid`. | KiCad 8+ object identity syntax. |
| Upgrade `>=20230410` | Preserve footprint `attr dnp` atoms while normalizing other supported PCB boolean fields. | Current KiCad parsers still accept `dnp` as an `attr` atom; expanding it to `(dnp yes/no)` is avoided. |
| Upgrade `>=20251028` | Convert numeric board net references to net names. | KiCad 10+ net-reference syntax. |
| Downgrade `<20260603` | Remove `table_cell` `knockout`. | `10.99` development-format rollback. |
| Downgrade `<20260521` | Remove pad `sim_electrical_type`. | Pad simulation field rollback. |
| Downgrade `<20260513` | Replace copper thieving fill mode with polygon fill. | Approximate newer zone fill mode. |
| Downgrade `<20260512` | Remove `net_chains` / `net_chain`. | Net-chain rollback. |
| Downgrade `<20260511` | Remove `spec_frequency` and `dielectric_model`. | Stackup frequency-field rollback. |
| Downgrade `<20260508` | Remove native PCB/footprint ellipse nodes. | Native ellipse rollback. |
| Downgrade `<20260410` | Remove typed/extruded 3D model blocks. | 3D model rollback. |
| Downgrade `<20251101` | Remove backdrill, tertiary drill, and post-machining fields. | Backdrill/post-machining rollback. |
| Downgrade `<20251028` | Add legacy netcodes to board net references. | Older net-reference compatibility. |
| Downgrade `<20250914` | Remove barcode nodes. | Barcode rollback. |
| Downgrade `<20250909` | Remove footprint/module `units`. | Footprint unit pin grouping rollback. |
| Downgrade `<20250901` | Remove `point`. | PCB point-object rollback. |
| Downgrade `<20250829` | Remove `rounded_rectangle` / `roundrect`. | Rounded-rectangle rollback. |
| Downgrade `<20250818` | Remove custom footprint layer-count fields. | Custom layer-count rollback. |
| Downgrade `<20250324` | Remove footprint jumper pad fields. | Jumper pad rollback. |
| Downgrade `<20250228` | Remove via protection fields; for targets `>=20240609`, downgrade tenting bool lists to atoms. | IPC-4761 via protection rollback. |
| Downgrade `<20250210` | Remove render caches, text-box/layer `knockout`, and tag cached zone fills as polygon fills. | Text cache and knockout rollback. |
| Downgrade `<=20241229` | Remove font `face`. | KiCad 9-and-earlier PCB font compatibility. |
| Downgrade `<20241228` | Rename `teardrops.curved_edges` to `curve_points` and map booleans to numeric values. | Teardrop field rollback. |
| Downgrade `<20241030` | Downgrade dimension boolean fields and remove dimension style `arrow_direction`. | Dimension syntax rollback. |
| Downgrade `<20241010` | Remove graphic `solder_mask_margin`. | Graphic mask-margin rollback. |
| Downgrade `<20241009` | Remove zone `placement`. | Rule/placement area rollback. |
| Downgrade `<20241007` | Remove track/arc solder-mask layer and margin fields. | Track mask-field rollback. |
| Downgrade `<20240703` | Remove user-layer type qualifiers. | Older user-layer syntax. |
| Downgrade `<20240617` | Remove table-cell `angle`. | PCB table-cell rotation rollback. |
| Downgrade `<20240108` | Downgrade font bools; remove soldermask bridge setup, group `locked`, and via layer-connection fields. | KiCad 7-and-earlier compatibility. |
| Downgrade `<20231231` | Unquote `uuid`/`tstamp`/`id`; rename footprint UUIDs back to `tstamp`; rename group/generated UUIDs back to `id`. | KiCad 8-before identity syntax. |
| Downgrade `<20231212` | Downgrade locked/hide boolean syntax; remove unlocked fields and 3D model `hide`. | KiCad 7/8 boundary rollback. |
| Downgrade `<20231014` | Remove board/footprint `generator_version`. | KiCad 8-before metadata rollback. |
| Downgrade `<20230924` | Downgrade `pcbplotparams` booleans and shape fill `no` to `none`. | Plot/fill syntax rollback. |
| Downgrade `<20230730` | Remove graphic net connectivity; also remove `net` from rectangles in KiCad 6/7 range. | Graphic connectivity rollback. |
| Downgrade `<20230620` | Downgrade PCB footprint fields to legacy storage. | Footprint field storage rollback. |
| Downgrade `<=20221018` | Remove several zone/footprint/pad/via fields; rename thermal width; downgrade stroke blocks and dimensions; remove locked/free-incompatible fields. | KiCad 7-and-earlier PCB parser compatibility. |
| Downgrade `<=20171130` | Remove KiCad 5-incompatible zone, setup, via, pad, model, footprint, header/layer, ID, arc, rectangle, and track-arc constructs; split multilayer zones; rename footprints to modules. | KiCad 5 PCB/footprint compatibility with significant loss. |
| Downgrade `<20170922` | Remove multilayer keepout. | KiCad 4 compatibility. |
| Downgrade `<20170920` | Simplify custom/roundrect pads to rectangular pads. | KiCad 4 pad compatibility. |
| Downgrade `<20171114` | Downgrade 3D model offsets to KiCad 4 `at` fields. | KiCad 4 3D model syntax. |
| Downgrade `<20160815` | Remove netclass differential-pair constraints. | Early KiCad 4 PCB compatibility. |

## Worksheet Rules

| Target/boundary | Implemented behavior | Effect |
| --- | --- | --- |
| KiCad 6+ | `.kicad_wks` / `drawing_sheet` roots are detected as `worksheet`; target versions come from the version table. | Version rewrite and S-expression formatting are possible. |
| Downgrade `<20220228` | Remove `font` nodes. | Current dedicated worksheet downgrade behavior. |
| KiCad 4/5 | No `WORKSHEET` legacy target version and no legacy worksheet writer. | Standalone worksheet conversion to KiCad 4/5 is not implemented. |

## Project-Directory Conversion

| Phase | Implementation behavior | Format impact |
| --- | --- | --- |
| Input selection | A directory or `.kicad_pro` input is treated as a project conversion; `.kicad_pro` uses its parent directory as the source. | Project conversion is a tree conversion, not a single JSON conversion. |
| File filter | Copies KiCad documents, legacy documents, library tables, `.kicad_prl`, and 3D model files. | Keeps files needed to reopen and edit the converted project. |
| Directory filter | Skips VCS, history, backup, archive, gerber/fabrication/output/plot/BOM/assembly/vendor-output directories. | Generated outputs and backups are not copied. |
| Target extensions | KiCad 5/4 targets map modern schematic/symbol/project files to `.sch/.lib/.pro`; KiCad 6+ targets map legacy files to `.kicad_sch/.kicad_sym/.kicad_pro`. | File-family boundary is reflected in output filenames. |
| `.dcm` with paired `.lib` | For KiCad 6+ targets, a `.dcm` is not separately converted if a paired `.lib` exists. | Prevents duplicate `.kicad_sym` generation. |
| `.kicad_dru` | If the target has no `.kicad_dru` support, the design-rule file is skipped with a warning. If the resolved target anchor matches the source anchor, it is copied. | Avoids copying unsupported design-rule files into legacy targets while keeping same-format design rules for supported targets. |
| `.kicad_prl` | Skipped for KiCad 5/4; generated or normalized for KiCad 6/7/8. | Provides numeric `visible_items` and compatible `visible_layers` for older modern KiCad versions. |
| Project worksheet references | For KiCad 6/7/8 targets, embedded worksheet page-layout references in `.kicad_pro` are cleared. | Prevents KiCad 6/7/8 from trying to load unsupported `kicad-embed://...kicad_wks` project worksheet paths. |
| Symbol library table | For KiCad 6+ targets, writes project-local `sym-lib-table` entries for generated `.kicad_sym` files; KiCad 7+ gets `(version 7)`, KiCad 6 has no table version. | Converted projects can resolve local symbol libraries. |
| Embedded schematic symbols | For KiCad 6+ targets, referenced symbols from project-local generated libraries are embedded into schematic `lib_symbols`. | Improves standalone schematic loading after conversion. |
| Hierarchy instances | For KiCad 6+ targets, hierarchy instances are rebuilt. | Fixes schematic hierarchy paths/pages for KiCad 6-style parsers. |
| Legacy library tables | For KiCad 6 and earlier, normalizes `sym-lib-table` / `fp-lib-table`; KiCad 5/4 symbol table entries are changed to `Legacy` and `.lib` URIs; `fp-lib-table` gets local `Library.pretty` aliases. | Library tables match the target family. |
| Legacy schematic cache library | For KiCad 5/4 targets, copies `Library.lib` to `<project>-cache.lib` and adds `LIBS:<project>-cache` to every generated `.sch`. | Improves symbol resolution in KiCad 4/5, including direct schematic opening and older cache-library workflows. |

## Adjacent-Version Summary

| Direction | File-family change | Implementation focus | Typical loss |
| --- | --- | --- | --- |
| KiCad 4 -> KiCad 5 | Schematic/symbol stay legacy; board/footprint `4 -> 20171130`. | Legacy `.sch/.lib` headers only; board/footprint upgrade rules. | No new KiCad 5 intent is synthesized. |
| KiCad 5 -> KiCad 4 | Schematic/symbol stay legacy; board/footprint `20171130 -> 4`. | Legacy headers and project/library support files are normalized; board/footprint remove KiCad 4-incompatible fields and simplify custom pads. | Differential pairs, keepouts, custom pads, newer 3D offsets. |
| KiCad 5 -> KiCad 6 | `.sch/.lib/.dcm/.pro` become `.kicad_sch/.kicad_sym/.kicad_pro`; board/footprint target `20211014`. | Legacy parsers to modern writers; UUIDs, instances, properties, and minimal project JSON. | Legacy drawing gaps, complex symbol graphics, project UI/cache settings. |
| KiCad 6 -> KiCad 5 | Modern schematic/symbol/project become legacy; board/footprint target `20171130`. | Legacy writers; project `.pro` sections, schematic cache libraries, and PCB header/layer/module/netcode/arc/rect rollback. | Modern properties, instances, complex graphics, advanced PCB geometry/layers. |
| KiCad 6 -> KiCad 7 | File family unchanged. | Conservative old-syntax normalization. | No DNP/simulation/image/net-tie/teardrop intent is generated. |
| KiCad 7 -> KiCad 6 | File family unchanged. | Remove KiCad 7-only fields and add KiCad 6 parser compatibility structures, including fill-color and root drawing primitive cleanup. | DNP, simulation fields, hyperlinks, net ties, images, stroke/dimension data, unsupported schematic drawing primitives. |
| KiCad 7 -> KiCad 8 | File family unchanged. | Upgrade identity, font, and property-hide syntax where source data exists. | No generated objects or PCB fields are invented. |
| KiCad 8 -> KiCad 7 | File family unchanged. | Remove `generator_version`, PCB fields, newer UUID/id forms, and KiCad 8-only fields. | Generated objects, footprint fields, metadata. |
| KiCad 8 -> KiCad 9 | File family unchanged. | Conservative old-syntax normalization. | No embedded files, tables, component classes, padstacks, or via stacks are invented. |
| KiCad 9 -> KiCad 8 | File family unchanged. | Remove or rewrite embedded data, tables, rule/placement areas, component classes, pad/via stacks, font faces, and user-layer type qualifiers. | Embedded data, complex manufacturing/constraint/layer-type structures. |
| KiCad 9 -> KiCad 10 | File family unchanged. | Upgrade old syntax; netcodes may become net names. | No variants, jumper data, backdrill, barcode, or via protection intent is inferred. |
| KiCad 10 -> KiCad 9 | File family unchanged. | Remove KiCad 10-only schematic/symbol/PCB fields; restore legacy netcodes; remove advanced manufacturing/assembly objects. | Variants, net chains, native ellipses, barcodes, backdrill, via protection, body styles. |
| KiCad 10 -> KiCad 10.99 | Only board/footprint version advances to `20260603`. | Symbol and schematic anchors remain equal to `10.0`; board/footprint allow `20260603`. | Not a KiCad 11 or future stable-format promise. |
| KiCad 10.99 -> KiCad 10 | Board/footprint `20260603 -> 20260206`; other families are effectively same anchors. | Remove 10.99-only board/footprint fields such as table-cell `knockout`. | Development-format fields are removed. |

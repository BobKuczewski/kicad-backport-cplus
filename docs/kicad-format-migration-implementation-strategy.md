# KiCad Format Migration Implementation Strategy

This document turns the format differences in
`kicad-file-format-version-differences.md` into implementation guidance for
adjacent KiCad major-version migrations.

The main boundary is KiCad 5 to KiCad 6. KiCad 4 and 5 use legacy schematic and
symbol-library files (`.sch`, `.lib`, `.dcm`), while KiCad 6 and newer use
S-expression schematic and symbol files (`.kicad_sch`, `.kicad_sym`). PCB and
footprint files are S-expression files across these versions, but their feature
sets and version numbers still require explicit rewrite rules.

Last updated: 2026-06-04.

## Scope

This is an implementation roadmap, not a release-support promise. It describes
how to implement:

- Forward migration such as KiCad 6 to 7 and KiCad 5 to 6.
- Backward migration such as KiCad 7 to 6, KiCad 6 to 5, and KiCad 5 to 4.
- Adjacent-version extension points for later versions such as 7 to 8, 8 to 9,
  9 to 10, and 10 to the current development branch.

The current C++ converter is primarily a downgrade engine. If the source format
version is older than the requested target, it currently copies the file
unchanged instead of upgrading it. Upgrade support should therefore be added as a
separate migration path, not by weakening the downgrade rules.

## Target Version Map

| KiCad target | Symbol library | Schematic | Board / footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: |
| 4.0 | Legacy `.lib` 2.3 | Legacy `.sch` V2 | `4` | Legacy drawing sheet | None |
| 5.0 / 5.1 | Legacy `.lib` 2.4 | Legacy `.sch` V4 | `20171130` | Legacy drawing sheet | None |
| 6.0 | `20211014` | `20211123` | `20211014` | `20210606` | `20200610` |
| 7.0 | `20220914` | `20230121` | `20221018` | `20220228` | `20200610` |
| 8.0 | `20231120` | `20231120` | `20240108` | `20231118` | `20200610` |
| 9.0 | `20241209` | `20250114` | `20241229` | `20231118` | `20200610` |
| 10.0 | `20251024` | `20260306` | `20260206` | `20231118` | `20200610` |

## Migration Engine Shape

Implement migrations as a pipeline with file-family adapters:

1. Detect document kind by root S-expression head or legacy extension/header.
2. Parse into a mutable document tree or typed intermediate model.
3. Select a migration route from source family/version to target
   family/version.
4. Apply ordered migration steps. Each step should be guarded by document kind,
   source version, and target version.
5. Write the target file family with the target version and target extension.
6. Emit warnings for every lossy removal, fallback default, or unimplemented
   semantic conversion.

Do not implement migration as a top-level version-number rewrite. Each version
boundary introduces or removes tokens, layout structures, object attributes, or
even whole file families.

## Cross-Family Rules

### KiCad 4/5 Legacy Family

KiCad 4 and 5 require legacy writers for schematic and symbol-library outputs:

- Schematic: `.sch`, with `EESchema Schematic File Version 2` for KiCad 4 and
  `Version 4` for KiCad 5.
- Symbol library: `.lib` plus optional `.dcm`, commonly `EESchema-LIBRARY
  Version 2.3` for KiCad 4 and `Version 2.4` for KiCad 5.
- Project: legacy `.pro`.

V6+ schematic and symbol data cannot be downgraded to KiCad 4/5 by pruning a
few S-expression nodes. The implementation needs a real legacy serializer or a
hard warning that the conversion is not supported.

### KiCad 6+ S-Expression Family

KiCad 6 and newer use S-expression files for schematic, symbol library, PCB,
footprint, worksheet, and design rules. Most adjacent migrations can be handled
as version-gated tree rewrites:

- Add or remove feature nodes based on the target version.
- Rename fields when the format changed.
- Convert boolean-list formats to legacy presence atoms, or the reverse.
- Normalize UUID, ID, font, text, and property structures for the target.

## Forward Migration Strategy

Forward migration should preserve source semantics and synthesize safe target
defaults. It should not invent new design intent.

Recommended behavior:

- If the source and target are in the same file family, parse and write the
  newer target format, applying known compatibility rewrites.
- If a format field is absent in the source, omit it when the newer format
  allows omission; otherwise write KiCad-like defaults.
- If the migration crosses from KiCad 4/5 legacy files to KiCad 6+
  S-expression files, use dedicated importers for `.sch`, `.lib`, `.dcm`, and
  `.pro`.
- Prefer using KiCad itself as a reference oracle for difficult legacy imports:
  load the old project in the matching KiCad version or in KiCad 6+, save it,
  then compare the generated structure against the converter output.

### KiCad 6 to KiCad 7 Upgrade

This is a same-family S-expression upgrade. It can be implemented as a
structured rewrite plus target version update.

Key actions:

- Update target versions to symbol `20220914`, schematic `20230121`, board /
  footprint `20221018`, and worksheet `20220228`.
- Preserve existing V6 schematic, symbol, board, footprint, worksheet, and DRC
  objects.
- Add defaults only where KiCad 7 requires a value that KiCad 6 did not write.
- Convert old-compatible fields to the newer spelling where needed, such as
  `netclass_flag` to `directive_label`.
- Normalize text box geometry to the KiCad 7 `at` / `size` representation if an
  older `start` / `end` representation is encountered.
- Keep V6 simulation information valid, but do not fabricate full KiCad 7
  simulation model data unless enough fields are present.
- Preserve or synthesize DNP-related defaults only when the target object type
  supports them.
- For PCB and footprints, preserve V6 objects and enable V7-compatible forms for
  stroke formatting, footprint private layers, dimensions, teardrop keywords,
  net ties, and pad/via zone-layer connections when enough data exists.
- For worksheets, write the V7 worksheet version and preserve font data only if
  it is represented in a KiCad 7-compatible way.

Validation fixtures:

- A V6 schematic with labels, fields, hierarchical sheets, and simulation
  fields.
- A V6 board with vias, pads, zones, dimensions, and footprint text.
- A V6 symbol library with properties and simple primitives.

### KiCad 5 to KiCad 6 Upgrade

This is the most important forward migration boundary because schematic and
symbol files change file families.

Required adapters:

- `.pro` to `.kicad_pro` JSON project migration.
- Legacy `.sch` V4 to `.kicad_sch` `20211123`.
- Legacy `.lib` / `.dcm` 2.4 to `.kicad_sym` `20211014`.
- `.kicad_pcb` / `.kicad_mod` `20171130` to `20211014`.
- Legacy drawing sheet to `.kicad_wks` `20210606`, if worksheet conversion is
  supported.

Key actions:

- Parse legacy schematic records into a typed model before writing KiCad 6
  S-expressions. Avoid line-oriented text replacement for `.sch` files.
- Map legacy schematic symbols to KiCad 6 symbol instances with UUIDs,
  properties, paths, sheet instances, and library identifiers.
- Generate stable UUIDs where KiCad 6 requires them. Use deterministic UUIDs
  derived from source paths and object identity when reproducibility matters.
- Convert legacy library aliases, fields, drawing primitives, pins, and
  documentation records into `.kicad_sym` symbols.
- Preserve `.dcm` descriptions, keywords, and documentation links as KiCad 6
  symbol metadata where possible.
- Convert legacy project settings to `.kicad_pro` and `.kicad_prl` only for
  settings that have clear equivalents. Warn for ignored UI or cache settings.
- Upgrade PCB version to `20211014` while preserving KiCad 5 board features,
  including custom pads, multi-layer keepouts, 3D model offsets, and footprint
  text lock state.

Expected lossy areas:

- Some legacy project settings may not have a KiCad 6 JSON equivalent.
- Legacy library rescue/cache behavior may need explicit symbol remapping.
- V5 schematic constructs that depend on old library lookup rules may require
  warnings or symbol-library sidecar data.

## Backward Migration Strategy

Backward migration should remove or rewrite unsupported constructs and report
loss. The target file should be loadable by the requested KiCad version even
when some newer semantics cannot be preserved.

Recommended behavior:

- Apply downgrade rules from newest to oldest so later features are removed
  before older compatibility rewrites run.
- Keep warning messages specific: name the removed feature, count affected
  nodes, and include the introduction version.
- For same-family S-expression downgrades, preserve object identity where the
  target supports it.
- For V6+ to V5/V4 schematic or symbol downgrades, use a dedicated legacy
  writer and treat unsupported V6+ objects as lossy.

### KiCad 7 to KiCad 6 Downgrade

This is a same-family S-expression downgrade. It should be implemented as
targeted removal and compatibility rewrite.

Target versions:

- Symbol library: `20211014`.
- Schematic: `20211123`.
- Board / footprint: `20211014`.
- Worksheet: `20210606`.
- Design rules: `20200610`.

Key downgrade rules:

- Remove symbol class flags, symbol fonts, text boxes, text colors, unit display
  names, and KiCad 7-only property-ID behavior.
- Remove schematic graphic primitives introduced after V6 where V6 has no
  equivalent, including text boxes and newer label fields.
- Rewrite `directive_label` back to the V6-compatible `netclass_flag` form if a
  faithful mapping exists; otherwise warn.
- Remove or downgrade dash-dot-dot line style, text hyperlinks, field name
  visibility, do-not-autoplace options, DNP support, and V7 simulation model
  fields.
- Move or simplify symbol and sheet instance data to the layout expected by
  KiCad 6.
- Remove PCB text boxes, image objects, first-class net ties, V7 teardrop
  keywords, footprint private-layer changes that V6 cannot parse, and pad/via
  zone-layer-connection additions.
- Convert V7 stroke, font, boolean, lock, and hide formats back to V6-compatible
  forms.
- Remove worksheet font support added in `20220228`.

Expected lossy areas:

- Text boxes, images, net ties, DNP flags, hyperlinks, and modern simulation
  model data may not have faithful V6 equivalents.
- Some V7 PCB dimensions and teardrop settings should be removed or flattened.

### KiCad 6 to KiCad 5 Downgrade

This is a cross-family downgrade for schematic and symbol files.

Required writers:

- `.kicad_sch` `20211123` to legacy `.sch` V4.
- `.kicad_sym` `20211014` to legacy `.lib` 2.4 plus `.dcm`.
- `.kicad_pro` JSON to legacy `.pro`.
- `.kicad_pcb` / `.kicad_mod` `20211014` to `20171130`.

Key downgrade rules:

- Serialize KiCad 6 schematic symbols, wires, buses, labels, junctions, sheets,
  fields, and page settings into the legacy `.sch` record format.
- Convert KiCad 6 UUIDs and paths into legacy timestamps or deterministic
  identifiers where the target requires them.
- Split KiCad 6 symbol metadata into `.lib` symbol definitions and `.dcm`
  documentation records.
- Remove KiCad 6-only schematic and symbol structures that the legacy files
  cannot represent directly. Warn for each removal.
- Convert `.kicad_pro` settings to `.pro` only when there is a known legacy
  equivalent. Ignore or warn for JSON-only settings.
- Downgrade board and footprint versions to `20171130`.
- Remove V6 board/footprint features that KiCad 5 cannot parse, including V6+
  UUID-only structures, newer zone behavior, rule file assumptions, and any
  object introduced after the V5 PCB format.
- Preserve KiCad 5-compatible PCB features: custom pads, multi-layer keepouts,
  3D model offset in mm using the `offset` parameter, and locked footprint text.

Expected lossy areas:

- KiCad 6 schematic and symbol S-expression details do not always map to legacy
  record fields.
- Project JSON settings, modern UUID identity, and some library linkage details
  may be reduced or dropped.
- `.kicad_dru` files have no KiCad 5 standalone equivalent.

### KiCad 5 to KiCad 4 Downgrade

This is mostly a legacy-family downgrade, with PCB still requiring
S-expression feature removal.

Target formats:

- Schematic: `.sch` V2.
- Symbol library: `.lib` 2.3 plus `.dcm`.
- PCB / footprint: version `4`.
- Project: legacy `.pro`.

Key downgrade rules:

- Rewrite the schematic header from `EESchema Schematic File Version 4` to
  `Version 2`.
- Remove or rewrite KiCad 5 schematic records that KiCad 4 cannot parse.
- Downgrade symbol library headers from 2.4-style output to 2.3-style output.
- Remove symbol fields or attributes that do not exist in KiCad 4 libraries.
- Downgrade board and footprint files from `20171130` to version `4`.
- Remove KiCad 5 PCB features introduced after KiCad 4, including differential
  pair settings per net class, long pad names, custom pad shape details,
  multi-layer keepouts, 3D model offset changes that KiCad 4 cannot interpret,
  and locked/unlocked footprint text.
- Convert 3D model offsets to the KiCad 4 representation when a reversible unit
  conversion is known; otherwise warn and remove the unsupported offset fields.

Expected lossy areas:

- Custom pads and multi-layer keepouts may need to be simplified or removed.
- Long pad names may need truncation or deterministic renaming.
- Some net-class and footprint text-lock metadata cannot be preserved.

## Later Adjacent Downgrade Patterns

The same downgrade framework should cover later adjacent versions:

| Route | Main implementation focus |
| --- | --- |
| KiCad 8 to 7 | Remove V8 generator cleanup fields, PCB fields, generated objects, UUID normalization, custom pad spoke templates, modern teardrops, images, rule-area changes, simulation/exclude flags, and worksheet embedded images. |
| KiCad 9 to 8 | Remove embedded files, tables, rule areas, component classes, complex padstacks, via stacks, arbitrary user layers, tenting, multiple netclass assignments, and netclass color highlighting. |
| KiCad 10 to 9 | Remove variants, jumper pads, barcodes, via protection, zone hatch offsets, backdrill, split via types, stopped-netcode assumptions, rounded rectangles, stacked pins, PCB points, and property-formatting updates. |
| Current development to KiCad 10 | Remove post-10.0 development features: extruded 3D body metadata, native ellipses and ellipse arcs, dielectric frequency-dependent stackup fields, net chains, copper thieving, pad `sim_electrical_type`, and PCB table-cell `knockout`. |

Forward migrations for these same routes are usually less lossy than downgrades,
but they still need compatibility rewrites and target defaults. For example,
KiCad 7 to 8 should introduce V8-normalized `generator_version` handling only
where the target format expects it, and KiCad 8 to 9 should not invent embedded
files, component classes, or padstacks unless they already exist in the source
semantics.

## Test Strategy

Create one fixture set per adjacent route and document kind:

- Project files: `.pro` and `.kicad_pro`.
- Schematic files: legacy `.sch` and `.kicad_sch`.
- Symbol libraries: `.lib`, `.dcm`, and `.kicad_sym`.
- PCB files: `.kicad_pcb`.
- Footprints: `.kicad_mod`.
- Worksheets: legacy drawing sheet and `.kicad_wks`.
- Design rules: `.kicad_dru` for KiCad 6+ only.

Each test should verify:

- Source kind and source version are detected correctly.
- Target file family, extension, and version are correct.
- Disallowed target-version tokens are absent.
- Known compatible structures are preserved.
- Lossy changes produce warnings.
- Re-running the same migration is stable and does not keep changing the file.

For V5/V6 and V6/V5 routes, add golden-file tests generated by KiCad itself
where possible. Those routes cross a file-family boundary and are the highest
risk for semantic drift.

## Implementation Order

Recommended order:

1. Finish same-family S-expression downgrades for V7 to V6.
2. Add V5 PCB / footprint downgrade to V4 because it stays in the PCB
   S-expression family.
3. Implement legacy schematic and symbol parsers for V5 to V6 upgrade.
4. Implement legacy schematic and symbol writers for V6 to V5 downgrade.
5. Add V5 to V4 legacy schematic and symbol downgrade.
6. Add forward same-family upgrades such as V6 to V7.
7. Extend the same framework to V8, V9, V10, and current-development routes.

This order delivers useful downgrade coverage early while isolating the much
harder legacy schematic and symbol conversion work.

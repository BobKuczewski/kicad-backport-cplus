# KiCad Backport C++

`kicad-backport` is a standalone command-line converter implemented in portable C++ for moving KiCad projects and files to older KiCad file-format targets. It focuses on parser compatibility: when an older KiCad version has an equivalent representation, the converter rewrites to that representation; when it does not, the unsupported syntax is removed or approximated and reported as a warning.

The implementation is dependency-free, uses a small KiCad-style S-expression parser/formatter, and can be used directly or from a plugin wrapper.

## Documentation

- [Documentation index](docs/README.md)
- [KiCad backport converter format differences](docs/kicad-backport-converter-format-differences.md)
- Localized README files: [简体中文](docs/README.zh-CN.md), [日本語](docs/README.ja-JP.md), [한국어](docs/README.ko-KR.md), [Français](docs/README.fr-FR.md), [Deutsch](docs/README.de-DE.md), [Español](docs/README.es-ES.md), [Italiano](docs/README.it-IT.md)

## Commands

```text
kicad-backport convert [--quiet] --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

Examples:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions --json E:\tmp\project
```

`convert` accepts either a single KiCad document or a project directory. Passing a `.kicad_pro` also converts the containing project directory. The output path is suffixed by target family, for example `project_V9`, unless the suffix is already present.

`inspect` reports the detected file kind and version. `detect-versions` performs a lightweight scan over a file or directory, reports supported KiCad document kinds, and can emit JSON for automation.

## Supported Targets

| Target | Current behavior |
| --- | --- |
| KiCad 10.99 | Development alias. Board and footprint targets write `20260603`; schematic and symbol-library targets remain at the KiCad 10 anchors. |
| KiCad 10 | Removes or rewrites newer development syntax that is not part of the `10.0` target anchors. |
| KiCad 9 | Removes or downgrades KiCad 10-era features such as variants, barcodes, backdrill/post-machining fields, jumper pad metadata, and net-name-only board references. |
| KiCad 8 | Removes or rewrites KiCad 9+ tables, embedded files/fonts, component classes, padstacks, via stacks, rule/placement areas, arbitrary user-layer type qualifiers, and font face fields. |
| KiCad 7 | Applies KiCad 7 parser compatibility for UUID/tstamp forms, PCB footprint fields, teardrops, generated objects, images, text boxes, and stroke/dimension syntax. |
| KiCad 6 | Targets the first modern schematic/symbol/project file family and adds KiCad 6 parser compatibility structures where required. |
| KiCad 5.0/5.1 | Uses board/footprint version `20171130` and writes legacy `.sch`, `.lib/.dcm`, and `.pro` for schematic, symbol-library, and project targets. |
| KiCad 4 | Uses board/footprint version `4`, rewrites V4 legacy schematic/library headers, and simplifies KiCad 5+ PCB constructs where possible. |

The per-file version anchors and conversion details are documented in [KiCad backport converter format differences](docs/kicad-backport-converter-format-differences.md).

## Conversion Policy

The converter prefers compatibility over textual preservation:

- Existing intent is preserved when the target format can represent it.
- Newer syntax is rewritten to older equivalent syntax when one exists.
- Unsupported nodes and fields are removed only when older KiCad parsers cannot read them or no equivalent representation exists.
- Lossy rewrites and removals are emitted as warnings.
- Upgrades are conservative: the converter does not invent new KiCad design features that are absent from the source file.

Crossing the KiCad 5/6 boundary changes file families where required:

| Direction | Examples |
| --- | --- |
| KiCad 5/4 -> KiCad 6+ | `.sch -> .kicad_sch`, `.lib/.dcm -> .kicad_sym`, `.pro -> .kicad_pro` |
| KiCad 6+ -> KiCad 5/4 | `.kicad_sch -> .sch`, `.kicad_sym -> .lib + .dcm`, `.kicad_pro -> .pro` |

Board and footprint files remain S-expression files across supported targets, with version-specific node and field rewrites.

## Project Conversion

When converting a project directory, the tool copies editable KiCad inputs and common local 3D model files, then converts copied KiCad documents in place. It skips generated manufacturing outputs, backups, history folders, Gerbers, BOMs, plot/export directories, and temporary files.

Project-level repair steps include:

- normalizing `sym-lib-table` and `fp-lib-table` for the target family;
- generating or normalizing `.kicad_prl` visibility data for KiCad 6/7/8 targets;
- embedding generated project-local schematic symbols for KiCad 6+ targets;
- rebuilding KiCad 6-style schematic hierarchy instances.

## Build

Native build scripts are provided for the host platform:

```powershell
.\build.ps1
```

```sh
./build.sh
```

Useful POSIX options:

```sh
./build.sh --clean
./build.sh --config Release
./build.sh --compiler g++-8
./build.sh --static-runtime off
```

The scripts read `kicad_backport_sources.txt`, compile the listed sources with `g++` or `clang++`, and copy the executable to `dist/` with plugin-compatible names such as:

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-linux-armhf`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

The implementation avoids newer standard-library facilities that are often missing from older deployment toolchains. It uses project-owned filesystem helpers and falls back from `-std=c++17` to `-std=c++1z` when needed.

## Project Layout

| Path | Responsibility |
| --- | --- |
| `src/kicad_backport.cpp` | CLI flow, project copying/filtering, conversion dispatch. |
| `src/kicad_backport_document.cpp` | KiCad document kind detection. |
| `src/kicad_backport_legacy.cpp` | Legacy `.sch`, `.lib`, `.dcm`, and `.pro` parsing/writing. |
| `src/kicad_backport_rules.cpp` | Downgrade version gates and rule ordering. |
| `src/kicad_backport_upgrade.cpp` | Conservative syntax upgrades for older sources. |
| `src/kicad_backport_versions.cpp` | KiCad aliases and per-file format version anchors. |
| `src/kicad_backport_rule_rewriters.cpp` | S-expression tree rewrite helpers. |
| `src/sexpr.cpp` | Minimal KiCad-style S-expression parser/formatter. |
| `include/kicad_backport/` | Public project headers. |

## Validation

After conversion, open the result with the target KiCad version and run the relevant checks. For KiCad 8/9/10 projects, use schematic ERC and PCB DRC where applicable:

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

Warnings emitted by the converter should be reviewed before treating a downgraded project as production-ready.

## Acknowledgements

Special thanks to Hubert for the help provided during development of this project.

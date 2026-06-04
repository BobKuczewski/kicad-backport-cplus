# KiCad Backport C++

Standalone C++17 implementation of the KiCad Backport downgrade CLI. The tool
converts newer KiCad S-expression project files to older KiCad file formats
while preferring equivalent legacy syntax over deletion.

## Localized Documentation

- [简体中文](docs/README.zh-CN.md)
- [日本語](docs/README.ja-JP.md)
- [한국어](docs/README.ko-KR.md)
- [Français](docs/README.fr-FR.md)
- [Deutsch](docs/README.de-DE.md)
- [Español](docs/README.es-ES.md)
- [Italiano](docs/README.it-IT.md)

Additional references:

- [KiCad file format version differences](docs/kicad-file-format-version-differences.md)
- [Localized file format version differences](docs/README.md#kicad-file-format-version-differences)

## Commands

The command line interface mirrors the Go implementation and is intended to be
usable both directly and from the Python plugin:

```text
kicad-backport convert --target-version <6.0|7.0|8.0|9.0|10.0|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

The converter reads KiCad S-expression files, applies version-driven downgrade
rules, writes a version-suffixed output path, and can copy whole KiCad project
directories before normalizing all KiCad files in the copy.

Examples:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

Supported release aliases are `6.0`, `7.0`, `8.0`, `9.0`, and `10.0`. A raw
KiCad file format version number can also be passed when testing a specific
parser cutoff.

## Support Status

The current implementation targets KiCad 6 through KiCad 10 S-expression file
families:

| Target | Status |
| --- | --- |
| KiCad 10 | Removes post-10.0/current-development syntax, including 20260521 pad `sim_electrical_type` and 20260603 table-cell `knockout`. |
| KiCad 9 | Removes or downgrades KiCad 10/current features such as variants, barcodes, backdrill/post-machining, jumper pads, and netcode omission. |
| KiCad 8 | Removes KiCad 9+ tables, embedded files, component classes, padstacks, via stacks, rule areas, and arbitrary user-layer forms. |
| KiCad 7 | Applies older parser compatibility rewrites for UUID/tstamp forms, PCB footprint fields, teardrops, generated objects, images, and text boxes. |
| KiCad 6 | Basic file downgrade support is largely complete. Converted test projects have been manually opened in the actual KiCad 6 application for validation. |
| KiCad 5 and older | Not implemented yet. The code now separates legacy document detection, path mapping, and upgrade/downgrade rules to prepare for future V5 support. |

## Downgrade Policy

The converter applies the most compatible representation available in the
target format:

- New objects or fields are mapped to older equivalent syntax when possible.
- Visible/manufacturing information is kept where the old format can express it.
- Unsupported syntax is removed only when older KiCad parsers cannot load it or
  the older file format has no equivalent representation.
- Each removal or compatibility rewrite is reported as a warning.

For example, legacy net codes are rebuilt for old PCB formats, newer boolean
field forms are converted to presence atoms where needed, KiCad 7 PCB
dimensions are preserved as visible graphics, and legacy project-local board
visibility files are generated for KiCad 6/7/8 targets.

When converting a project directory or `.kicad_pro`, the tool copies only
editable KiCad inputs and common local 3D model files. Generated manufacturing
outputs, history/backup folders, Gerbers, BOMs, and temporary files are skipped.

## Project Layout

The code is split by responsibility so later KiCad versions can be added with
small, localized changes:

- `src/kicad_backport.cpp`: CLI flow, project copy/filtering, file conversion.
- `src/kicad_backport_document.cpp`: KiCad document kind detection.
- `src/kicad_backport_legacy.cpp`: legacy KiCad document loading scaffolding.
- `src/kicad_backport_pathmap.cpp`: target file extension mapping helpers.
- `src/kicad_backport_report.cpp`: JSON report formatting.
- `src/kicad_backport_rules.cpp`: version gates and downgrade rule ordering.
- `src/kicad_backport_rule_rewriters.cpp`: S-expression tree rewrite helpers.
- `src/kicad_backport_upgrade.cpp`: limited syntax normalization for older source files.
- `src/kicad_backport_versions.cpp`: KiCad release aliases and format versions.
- `src/kicad_backport_util.cpp`: shared string, file, and JSON helpers.
- `src/sexpr.cpp`: minimal KiCad-style S-expression parser/formatter.
- `src/internal/`: private implementation headers used only by source files.
- `include/kicad_backport/`: public project headers used by the executable.

Single-action downgrade rules use a small `applyWhen()` helper instead of
`std::function`, keeping the rules compact without adding heap allocations.
Multi-action rules remain grouped when ordering matters.

The top-level structure is intentionally simple:

```text
kicad-backport-cplus/
  include/kicad_backport/   public headers
  src/                      implementation files
  src/internal/             private headers
  scripts/                  cross-build environment setup
  build/                    generated build trees
  dist/                     packaged command-line binaries
```

## Build

Build on the current platform:

```powershell
.\build.ps1
```

```sh
./build.sh
```

To automatically detect and install the smallest practical toolchain before
building:

```powershell
.\build.ps1 -SetupMissingTools
```

```sh
./build.sh --setup
```

Both scripts try the standard release targets and copy successful outputs to
`dist/` using plugin-compatible names:

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

Use `.\build.ps1 -Clean` or `./build.sh --clean` to remove previous build
outputs before rebuilding.

C++ cross-compilation requires platform toolchains. On Windows, `build.ps1`
builds `windows-amd64` and `windows-arm64` with Visual Studio, and builds
`linux-amd64`/`linux-arm64` through WSL when the WSL toolchain is available.
On Linux, `build.sh` builds native Linux and can build `linux-arm64` when
`aarch64-linux-gnu-g++` is installed. On macOS, `build.sh` builds Darwin
amd64/arm64 with the Apple SDK. Darwin binaries must be generated on macOS.

To build a subset:

```powershell
.\build.ps1 -Targets windows-amd64,windows-arm64
```

```sh
TARGETS="linux-amd64 linux-arm64" ./build.sh
```

Cross-build environment setup:

```powershell
.\scripts\setup-cross.ps1
.\scripts\setup-cross.ps1 -CheckOnly
```

```sh
./scripts/setup-cross.sh
./scripts/setup-cross.sh --check-only
```

The setup scripts automatically install the smallest practical build toolchain
for the host platform. Use `-CheckOnly` or `--check-only` to only report missing
tools without installing anything.

On Windows, the setup script installs or prepares CMake, Visual Studio C++
Build Tools, WSL, Ubuntu, and the minimal WSL packages needed for Linux
amd64/arm64 builds. On Linux, it installs CMake, a native C++ compiler, Ninja,
and the aarch64 Linux cross compiler where supported by the host package
manager. On macOS, it triggers Apple Command Line Tools and installs CMake/Ninja
through Homebrew when available.

Manual CMake build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The implementation is intentionally dependency-free and follows KiCad-style C++
formatting conventions.

## Validation

After conversion, validate each target with the matching KiCad version. For
KiCad 8/9/10 this usually means running schematic ERC and PCB DRC:

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 CLI has a smaller command set, so use netlist and Gerber export to
verify that converted schematic and PCB files load:

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

KiCad 6 has limited CLI validation coverage. For PCB files, a quick parser check
can be done through KiCad 6's Python module:

```powershell
& 'D:\KiCad\6.0\bin\python.exe' -c "import pcbnew; pcbnew.LoadBoard(r'E:\tmp\project_V6\project.kicad_pcb'); print('pcb ok')"
```

For KiCad 6 schematics and symbols, manual GUI opening remains the most useful
end-to-end validation. Current V6 regression samples have been checked this way.

ERC/DRC violations are design-rule findings from the project. They are not
format conversion failures unless KiCad reports a load or parse error.

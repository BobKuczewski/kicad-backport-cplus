# KiCad Backport C++

`kicad-backport` ist ein eigenständiger Kommandozeilenkonverter, der in portablem C++ implementiert ist und KiCad-Projekte sowie -Dateien in ältere KiCad-Dateiformatziele überführt. Ziel ist Parser-Kompatibilität: Gibt es in der älteren KiCad-Version eine äquivalente Darstellung, wird dorthin umgeschrieben; andernfalls wird nicht unterstützte Syntax entfernt oder approximiert und als Warning gemeldet.

Die Implementierung ist dependency-free, enthält einen kleinen KiCad-artigen S-expression Parser/Formatter und kann direkt oder über einen Plugin-Wrapper genutzt werden.

## Dokumentation

- [Dokumentationsindex](README.md)
- [Formatunterschiede des Konverters](kicad-backport-converter-format-differences.de-DE.md)
- [Englisches README](../README.md)

## Befehle

```text
kicad-backport convert [--quiet] --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

Beispiel:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions --json E:\tmp\project
```

`convert` akzeptiert ein einzelnes KiCad-Dokument oder ein Projektverzeichnis. Ein `.kicad_pro` konvertiert das zugehörige Projektverzeichnis. Der Ausgabepfad erhält einen Ziel-Suffix wie `project_V9`.

`inspect` meldet erkannten Dateityp und Version. `detect-versions` führt einen leichten Scan aus und kann JSON ausgeben.

## Unterstützte Ziele

| Ziel | Aktuelles Verhalten |
| --- | --- |
| KiCad 10.99 | Entwicklungsalias. Board/footprint schreiben `20260603`; schematic/symbol-library bleiben auf KiCad-10-Ankern. |
| KiCad 10 | Entfernt oder schreibt neuere Entwicklungssyntax außerhalb der `10.0`-Zielanker um. |
| KiCad 9 | Entfernt oder downgraded variants, barcodes, backdrill/post-machining, jumper pad metadata und board references nur per net name. |
| KiCad 8 | Entfernt oder schreibt KiCad 9+ tables, embedded files/fonts, component classes, padstacks, via stacks, rule/placement areas, user-layer type qualifiers und font face fields um. |
| KiCad 7 | Wendet Kompatibilität für UUID/tstamp, PCB footprint fields, teardrops, generated objects, images, text boxes und stroke/dimension syntax an. |
| KiCad 6 | Ziel ist die erste moderne schematic/symbol/project-Familie mit benötigten Parser-Kompatibilitätsstrukturen. |
| KiCad 5.0/5.1 | Board/footprint nutzen `20171130`; schematic, symbol-library und project schreiben legacy `.sch`, `.lib/.dcm`, `.pro`. |
| KiCad 4 | Board/footprint nutzen `4`; V4 legacy header werden umgeschrieben und KiCad 5+ PCB constructs möglichst vereinfacht. |

Details stehen in den [Formatunterschieden](kicad-backport-converter-format-differences.de-DE.md).

## Konvertierungsrichtlinie

- Bestehende Absicht bleibt erhalten, wenn das Zielformat sie ausdrücken kann.
- Neue Syntax wird in alte äquivalente Syntax umgeschrieben, wenn vorhanden.
- Entfernt wird nur, was alte Parser nicht lesen können oder wofür es keine Entsprechung gibt.
- Verlustbehaftete Rewrites und Entfernungen werden als Warning gemeldet.
- Upgrades erzeugen keine KiCad-Funktionen, die in der Quelle fehlen.

Die KiCad-5/6-Grenze wechselt Dateifamilien: `.sch -> .kicad_sch`, `.lib/.dcm -> .kicad_sym`, `.pro -> .kicad_pro`, sowie zurück `.kicad_sch -> .sch`, `.kicad_sym -> .lib + .dcm`, `.kicad_pro -> .pro`.

## Projektkonvertierung

Bei Projektverzeichnissen kopiert der Konverter nur bearbeitbare KiCad-Eingaben und übliche lokale 3D-Modelle und konvertiert dann die kopierten Dokumente. Fertigungsausgaben, Backups, history, Gerbers, BOM, plot/export-Verzeichnisse und temporäre Dateien werden übersprungen.

Projekt-Reparaturen umfassen `sym-lib-table` / `fp-lib-table`, `.kicad_prl` für KiCad 6/7/8, beim Downgrade neuer board/footprint-Dateien das Extrahieren eingebetteter 3D-Modellressourcen nach `3D/` und das Umschreiben von `kicad-embed://...` model URIs zu `${KIPRJMOD}/3D/...`, Einbettung lokaler schematic symbols in `lib_symbols` und Wiederaufbau der hierarchy instances.

## Build

```powershell
.\build.ps1
```

```sh
./build.sh
```

Nützliche POSIX-Optionen:

```sh
./build.sh --clean
./build.sh --config Release
./build.sh --compiler g++-8
./build.sh --static-runtime off
./build.sh --zstd off
```

PowerShell akzeptiert ebenfalls `-Zstd auto|on|off`. Der Standard `auto` kompiliert den vendored zstd decompressor, wenn `src/third_party/zstd` vorhanden ist. Für Toolchains, die die vendored C-Sourcen nicht bauen können, kann `off` genutzt werden; dann können eingebettete 3D-Modellressourcen nicht extrahiert werden und der Konverter warnt, wenn unsupported embedded model references entfernt werden müssen.

Die Skripte lesen `kicad_backport_sources.txt`, kompilieren mit `g++` oder `clang++` und kopieren das Ergebnis nach `dist/`. Bei Bedarf erfolgt ein Fallback von `-std=c++17` auf `-std=c++1z`.

## Validierung

Nach der Konvertierung sollte das Ergebnis mit der Zielversion von KiCad geöffnet und ERC/DRC ausgeführt werden, sofern zutreffend. Converter-Warnings müssen vor Produktionseinsatz geprüft werden.

## Danksagung

Besonderer Dank gilt Hubert für die Unterstützung während der Entwicklung dieses Projekts.

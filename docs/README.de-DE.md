# KiCad Backport C++

KiCad Backport C++ ist eine eigenständige C++17-Implementierung der KiCad-Backport-Downgrade-CLI. Das Werkzeug konvertiert neuere KiCad-S-Expression-Projektdateien in ältere KiCad-Dateiformate und bevorzugt dabei äquivalente ältere Syntax gegenüber dem Löschen von Daten.

## Befehle

```text
kicad-backport convert --target-version <7.0|8.0|9.0|10.0|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

Beispiele:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

Unterstützte Release-Aliasse sind `7.0`, `8.0`, `9.0` und `10.0`. Für Tests bestimmter Parser-Grenzen kann auch eine rohe KiCad-Dateiformatversion angegeben werden.

## Downgrade-Richtlinie

- Neue Objekte oder Felder werden nach Möglichkeit in äquivalente ältere Syntax umgewandelt.
- Sichtbare Informationen und Fertigungsdaten bleiben erhalten, wenn das Zielformat sie darstellen kann.
- Nicht unterstützte Syntax wird nur entfernt, wenn ältere KiCad-Versionen sie nicht laden können oder kein äquivalenter Ausdruck existiert.
- Jede Entfernung oder Kompatibilitätsumwandlung wird als warning gemeldet.

Bei der Konvertierung eines Projektordners werden nur KiCad-Projektdateien und gängige lokale 3D-Modelle kopiert. `.history`, Backups, Gerber, BOM, PDF, README und andere irrelevante Dateien werden nicht kopiert.

## Build

Windows:

```powershell
.\build.ps1
.\build.ps1 -SetupMissingTools
```

Linux/macOS:

```sh
./build.sh
./build.sh --setup
```

Ausgaben werden in `dist/` erzeugt.

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

Unter Windows erstellt `build.ps1` Windows amd64/arm64 mit Visual Studio und Linux amd64/arm64 über WSL, wenn die Toolchain verfügbar ist. Darwin-Binärdateien benötigen macOS und das Apple SDK.

## Validierung

Nach der Konvertierung sollte mit der passenden KiCad-CLI-Version geprüft werden. Für KiCad 8/9/10 sind ERC und DRC üblich:

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 hat weniger CLI-Befehle. Verwenden Sie Netlist- und Gerber-Export, um das Laden der Dateien zu prüfen:

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

ERC/DRC-Verletzungen sind Ergebnisse der Projektregeln. Sie bedeuten keinen Konvertierungsfehler, solange KiCad keinen Lade- oder Parsefehler meldet.

# KiCad Backport C++

Eigenständige C++17-Implementierung der KiCad Backport-Downgrade-CLI. Das Werkzeug
Konvertiert neuere KiCad S-Expression-Projektdateien in ältere KiCad-Dateiformate
wobei die entsprechende Legacy-Syntax dem Löschen vorgezogen wird.

## Lokalisierte Dokumentation

- [简体中文](docs/README.zh-CN.md)
- [日本語](docs/README.ja-JP.md)
- [한국어](docs/README.ko-KR.md)
- [Français](docs/README.fr-FR.md)
- [Deutsch](docs/README.de-DE.md)
- [Español](docs/README.es-ES.md)
- [Italienisch](docs/README.it-IT.md)

Zusätzliche Referenzen:

- [Versionsunterschiede im KiCad-Dateiformat](docs/kicad-file-format-version-differences.md)
- [Unterschiede in lokalisierten Dateiformatversionen](docs/README.md#kicad-file-format-version-differences)

## Befehle

Die Befehlszeilenschnittstelle spiegelt die Go-Implementierung wider und soll es auch sein
sowohl direkt als auch über das Python-Plugin nutzbar:

```text
kicad-backport convert --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

Der Konverter liest KiCad S-Expression-Dateien und führt ein versioniertes Downgrade durch
Regeln, schreibt einen Ausgabepfad mit Versionssuffix und kann das gesamte KiCad-Projekt kopieren
Verzeichnisse, bevor alle KiCad-Dateien in der Kopie normalisiert werden. Bei der
Konvertierung gibt er fuer jede KiCad-Datei die erkannte Quellversion und die
aufgeloeste Zielversion aus. Lesbare Textausgaben bevorzugen KiCad-Aliase wie
`9.0 (20241229)` oder `10.99-dev (20260513)` statt roher Dateiformatnummern.
`detect-versions` ist ein schneller Verzeichnisscan, der nur so viel Text liest,
wie fuer Dateityp und Version erforderlich ist. Textausgaben verwenden dieselben
Aliase; JSON-Ausgaben behalten rohe Dateiformatversionen. Der Befehl filtert
zuerst nach unterstuetzten KiCad-Dateiendungen und laesst Dateien ohne erkannte
Version aus.

Beispiele:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions E:\tmp\project
```

Unterstützte Release-Aliase sind `4.0`, `5.0`, `5.1`, `6.0`, `7.0`, `8.0`,
`9.0`, `10.0` und `10.99`. Eine rohe KiCad-Dateiformatversion kann ebenfalls für
bestimmte Parser-Grenzen übergeben werden.

## Supportstatus

Die aktuelle Implementierung zielt auf KiCad 4 bis KiCad 10 Dateifamilien ab:

| Ziel | Status |
| --- | --- |
| KiCad 10 | Entfernt die Syntax nach 10/aktueller Entwicklung, einschließlich 20260521 pad `sim_electrical_type` und 20260603 table-cell `knockout`. |
| KiCad 10.99 | Aktuelles Entwicklungsziel fuer board/footprint: schreibt Version `20260603`; Symbolbibliotheken und Schaltplaene verwenden weiter die KiCad-10-Zielversionen (`20251024` / `20260306`). |
| KiCad 9 | Entfernt oder stuft KiCad 10/aktuelle Funktionen wie Varianten, Barcodes, Backdrill/Nachbearbeitung, Jumper-Pads und das Weglassen von Netcodes herunter. |
| KiCad 8 | Entfernt KiCad 9+-Tabellen, eingebettete Dateien, Komponentenklassen, Padstacks, Via-Stacks, Regelbereiche und beliebige Formulare auf Benutzerebene. |
| KiCad 7 | Wendet ältere Parser-Kompatibilitätsumschreibungen für UUID-/Tstamp-Formulare, PCB-Footprint-Felder, Teardrops, generierte Objekte, Bilder und Textfelder an. |
| KiCad 6 | Die grundlegende Unterstützung für das Downgrade von Dateien ist weitgehend abgeschlossen. Konvertierte Testprojekte wurden zur Validierung manuell in der eigentlichen KiCad 6-Anwendung geöffnet. |
| KiCad 5 | Unterstützt Board-/Footprint-Zielversion `20171130` sowie grundlegenden Import/Export von Legacy `.sch`, `.lib`, `.dcm` und `.pro`. Detaillierte Schaltplanobjekte, Symbolgrafiken und Pins bleiben verlustbehaftet und werden mit Warnungen gemeldet. |
| KiCad 4 | Unterstützt Board-/Footprint-Zielversion `4`, V4-Header-Umschreibung für Legacy-Schaltpläne/Bibliotheken sowie V4-Ausgabesuffixe und Erweiterungen. V5-only PCB-Funktionen wie Custom Pads werden soweit möglich vereinfacht. |

## Downgrade-Richtlinie

Der Konverter wendet die kompatibelste Darstellung an, die im verfügbar ist
Zielformat:

- Neue Objekte oder Felder werden nach Möglichkeit der älteren äquivalenten Syntax zugeordnet.
- Sichtbare/Herstellungsinformationen werden dort gespeichert, wo das alte Format sie ausdrücken kann.
- Nicht unterstützte Syntax wird nur entfernt, wenn ältere KiCad-Parser sie nicht laden können oder
Für das ältere Dateiformat gibt es keine entsprechende Darstellung.
- Jede Entfernung oder Kompatibilitätsumschreibung wird als Warnung gemeldet.

Beispielsweise werden ältere Netzcodes für alte PCB-Formate und neuere boolesche Formate neu erstellt
Feldformen werden bei Bedarf in Präsenzatome umgewandelt, KiCad 7 PCB
Die Abmessungen bleiben als sichtbare Grafiken und als Legacy-Projekt-lokale Tafel erhalten
Sichtbarkeitsdateien werden für KiCad 6/7/8-Ziele generiert.

Beim Konvertieren eines Projektverzeichnisses oder `.kicad_pro` kopiert das Tool nur
bearbeitbare KiCad-Eingaben und gängige lokale 3D-Modelldateien. Generierte Fertigung
Ausgaben, Verlaufs-/Sicherungsordner, Gerbers, Stücklisten und temporäre Dateien werden übersprungen.
Beim Ueberschreiten der KiCad-5/6-Grenze werden Erweiterungen automatisch angepasst,
zum Beispiel `.sch -> .kicad_sch`, `.lib -> .kicad_sym`, `.kicad_sch -> .sch`,
`.kicad_sym -> .lib/.dcm` und `.kicad_pro -> .pro`.

## Projektlayout

Der Code ist nach Verantwortlichkeiten aufgeteilt, sodass spätere KiCad-Versionen hinzugefügt werden können
kleine, lokale Veränderungen:

- `src/kicad_backport.cpp`: CLI-Ablauf, Projektkopie/-filterung, Dateikonvertierung.
- `src/kicad_backport_document.cpp`: Erkennung der KiCad-Dokumentart.
- `src/kicad_backport_legacy.cpp`: Helfer zum Lesen/Schreiben alter KiCad-Dateien `.sch`, `.lib`, `.dcm` und `.pro`.
- `src/kicad_backport_pathmap.cpp`: Hilfsprogramme für die Zuordnung von Zieldateierweiterungen.
- `src/kicad_backport_report.cpp`: JSON-Berichtsformatierung.
- `src/kicad_backport_rules.cpp`: Reihenfolge der Versionstore und Downgrade-Regeln.
- `src/kicad_backport_rule_rewriters.cpp`: Helfer zum Umschreiben des S-Ausdrucksbaums.
- `src/kicad_backport_upgrade.cpp`: eingeschränkte Syntaxnormalisierung für ältere Quelldateien.
- `src/kicad_backport_versions.cpp`: KiCad-Release-Aliase und Formatversionen.
- `src/kicad_backport_util.cpp`: gemeinsam genutzte String-, Datei- und JSON-Helfer.
- `src/sexpr.cpp`: minimaler S-Ausdruck-Parser/Formatierer im KiCad-Stil.
- `src/internal/`: Private Implementierungsheader, die nur von Quelldateien verwendet werden.
- `include/kicad_backport/`: öffentliche Projektheader, die von der ausführbaren Datei verwendet werden.

Single-Action-Downgrade-Regeln verwenden stattdessen einen kleinen `applyWhen()`-Helper
`std::function`, wodurch die Regeln kompakt bleiben, ohne Heap-Zuweisungen hinzuzufügen.
Mehrfachaktionsregeln bleiben bei der Bestellung von Angelegenheiten gruppiert.

Die Struktur der obersten Ebene ist bewusst einfach:

```text
kicad-backport-cplus/
  include/kicad_backport/   public headers
  src/                      implementation files
  src/internal/             private headers
  scripts/                  cross-build environment setup
  build/                    generated build trees
  dist/                     packaged command-line binaries
```

## Bauen

Bauen Sie auf der aktuellen Plattform auf:

```powershell
.\build.ps1
```

```sh
./build.sh
```

Verwenden Sie die nativen Helfer, wenn nur das aktuelle Linux- oder macOS-Host-Binary
gebaut werden soll und keine Standard-Cross-Target-Verteilung noetig ist:

```sh
./build-linux.sh
./build-macos.sh
```

Um die kleinste praktische Toolchain automatisch zu erkennen und zu installieren
Gebäude:

```powershell
.\build.ps1 -SetupMissingTools
```

```sh
./build.sh --setup
```

Beide Skripte probieren die Standard-Release-Ziele aus und kopieren erfolgreiche Ausgaben dorthin
`dist/` mit Plugin-kompatiblen Namen:

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

Verwenden Sie `.\build.ps1 -Clean` oder `./build.sh --clean`, um den vorherigen Build zu entfernen
Ausgaben vor dem Neuaufbau.

Verwenden Sie `./build-linux.sh --clean` oder `./build-macos.sh --clean`, um nur
den jeweiligen nativen Build-Baum und die Ausgabe zu entfernen. Beide nativen
Helfer akzeptieren `--config <name>`, `--generator <cmake-generator>` und
`--jobs <n>`.

Für die C++-Cross-Kompilierung sind Plattform-Toolchains erforderlich. Unter Windows: `build.ps1`
erstellt `windows-amd64` und `windows-arm64` mit Visual Studio und erstellt
`linux-amd64`/`linux-arm64` über WSL, wenn die WSL-Toolchain verfügbar ist.
Unter Linux erstellt `build.sh` natives Linux und kann `linux-arm64` erstellen, wenn
`aarch64-linux-gnu-g++` ist installiert. Unter macOS erstellt `build.sh` Darwin
amd64/arm64 mit dem Apple SDK. Darwin-Binärdateien müssen unter macOS generiert werden.
Fuer strikt native Builds nutzt `build-linux.sh` die Linux-C++-Toolchain des Hosts
und `build-macos.sh` nutzt Apple Command Line Tools ueber `xcrun`.

So erstellen Sie eine Teilmenge:

```powershell
.\build.ps1 -Targets windows-amd64,windows-arm64
```

```sh
TARGETS="linux-amd64 linux-arm64" ./build.sh
```

Einrichtung einer Cross-Build-Umgebung:

```powershell
.\scripts\setup-cross.ps1
.\scripts\setup-cross.ps1 -CheckOnly
```

```sh
./scripts/setup-cross.sh
./scripts/setup-cross.sh --check-only
```

Die Setup-Skripte installieren automatisch die kleinste praktische Build-Toolchain
für die Host-Plattform. Verwenden Sie `-CheckOnly` oder `--check-only`, um nur fehlende Elemente zu melden
Tools, ohne etwas zu installieren.

Unter Windows installiert oder bereitet das Setup-Skript CMake und Visual Studio C++ vor
Build Tools, WSL, Ubuntu und die minimalen WSL-Pakete, die für Linux benötigt werden
amd64/arm64-Builds. Unter Linux installiert es CMake, einen nativen C++-Compiler, Ninja,
und der aarch64-Linux-Cross-Compiler, der vom Host-Paket unterstützt wird
Manager. Unter macOS löst es die Apple Command Line Tools aus und installiert CMake/Ninja
über Homebrew, sofern verfügbar.

Manueller CMake-Build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Die Implementierung ist bewusst abhängigkeitsfrei und folgt C++ im KiCad-Stil
Formatierungskonventionen.

## Danksagungen

Besonderer Dank gilt Hubert fuer die Hilfe waehrend der Entwicklung dieses
Projekts.

## Validierung

Validieren Sie nach der Konvertierung jedes Ziel mit der passenden KiCad-Version. Für
KiCad 8/9/10 Dies bedeutet normalerweise, dass Schaltplan-ERC und PCB-DRC ausgeführt werden:

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 CLI verfügt über einen kleineren Befehlssatz. Verwenden Sie daher Netlist und Gerber-Export
Überprüfen Sie, ob die konvertierten Schaltplan- und PCB-Dateien geladen werden:

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

KiCad 6 verfügt über eine begrenzte CLI-Validierungsabdeckung. Für PCB-Dateien eine schnelle Parserprüfung
kann über das Python-Modul von KiCad 6 erfolgen:

```powershell
& 'D:\KiCad\6.0\bin\python.exe' -c "import pcbnew; pcbnew.LoadBoard(r'E:\tmp\project_V6\project.kicad_pcb'); print('pcb ok')"
```

Für KiCad 6-Schaltpläne und -Symbole bleibt das manuelle Öffnen der GUI am nützlichsten
End-to-End-Validierung. Aktuelle V6-Regressionsbeispiele wurden auf diese Weise überprüft.

ERC/DRC-Verstöße sind Design-Rule-Erkenntnisse aus dem Projekt. Das sind sie nicht
Formatkonvertierungsfehler, es sei denn, KiCad meldet einen Lade- oder Analysefehler.

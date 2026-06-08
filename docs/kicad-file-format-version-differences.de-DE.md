# Versionsunterschiede im KiCad-Dateiformat

Dieses Dokument verfolgt die Versionsunterschiede im KiCad-Dateiformat, die vom Backport verwendet werden
Konverter. Es ist so organisiert, dass neuere stabile oder Entwicklungsversionen hinzugefügt werden können
ohne die Datei umzubenennen.

Zuletzt aktualisiert: 2026-06-07.

## Quellen und Methode

Überprüfte Quellen:

- Offizielle KiCad-GitLab-Tags und Quelldateien.
- Lokaler KiCad-Checkout unter `E:/WORKS/MY/kicadProject/kicad`.
- Lokale Referenzen und Tags: `origin/4.0`, `4.0.0`, `origin/5.0`, `origin/5.1`,
  `5.0.0`, `5.1.0`, `6.0.0`, `7.0.0`, `8.0.0`, `9.0.0`, `10.0.0`,
und `origin/10.0`.
- Lokaler KiCad `master`, wird nur zur Identifizierung des Entwicklungszweigs nach 10.0 verwendet
Grenzen.
- `kicad-backport-cplus`-Implementierung, insbesondere:
  - `src/kicad_backport_versions.cpp`
  - `src/kicad_backport_rules.cpp`
  - `src/kicad_backport_rule_rewriters.cpp`
  - `src/kicad_backport_upgrade.cpp`
  - `src/kicad_backport_legacy.cpp`
  - `src/kicad_backport_pathmap.cpp`
  - `src/kicad_backport.cpp`
- Versions-Header-Dateien:
  - `pcbnew/kicad_plugin.h` für KiCad 4/5 PCB-Formate.
  - `pcbnew/plugins/kicad/pcb_plugin.h` für KiCad 6/7 PCB-Formate.
  - `eeschema/sch_file_versions.h`
  - `pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h`
  - `include/drawing_sheet/ds_file_versions.h`
  - `pcbnew/drc/drc_rule_parser.h`
  - `eeschema/general.h` und `eeschema/sch_legacy_plugin.h` für KiCad 4/5
alte Schaltplanformate.

Versionsnummern werden den aktiven KiCad-Quellmakros entnommen:

- `SEXPR_SYMBOL_LIB_FILE_VERSION`
- `SEXPR_SCHEMATIC_FILE_VERSION`
- `SEXPR_BOARD_FILE_VERSION`
- `SEXPR_WORKSHEET_FILE_VERSION`
- `DRC_RULE_FILE_VERSION`

Hinweise:

- Board-S-Expression-Versionen decken auch Footprint-`.kicad_mod`-Dateien ab.
- `.kicad_dru` blieb von KiCad 6.0 bis zu den aktuellen 10.99-Quellen bei `20200610`.
Dies bedeutet lediglich, dass sich das Versionsmakro nicht geändert hat. Regelsemantik kann noch vorhanden sein
geändert.
- `.kicad_pro` ist eine JSON-Projektdatei und verwendet stattdessen Einstellungen/Schemamigration
dieser S-Ausdruck-Datumsversionsmakros. Unterschiede im Projekt-JSON-Schema
sollten gesondert erfasst werden.
- KiCad 4/5-Schaltpläne und Symbolbibliotheken sind Legacy-`.sch`, `.lib` und
`.dcm`-Dateien, nicht `.kicad_sch` oder `.kicad_sym`.

## Hauptdatei-Familienmatrix

| KiCad-Hauptversion | Projekt | Schematisch | Symbolbibliothek | PCB / Footprint | Arbeitsblatt | Designregeln | Kernpunkt |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 4.0 | Legacy `.pro` | Legacy `.sch`, `EESCHEMA_VERSION=2` | `.lib` `EESchema-LIBRARY Version 2.3`, `.dcm` | `.kicad_pcb` / `.kicad_mod` S-Ausdruck, Version `4` | Legacy-Zeichenblatt | Kein eigenständiger `.kicad_dru` | PCB war bereits S-Ausdruck; Schaltplan- und Symbolbibliotheken waren noch immer veraltet. |
| 5.0 / 5.1 | Legacy `.pro` | Legacy `.sch`, `EESCHEMA_VERSION=4` | Im Allgemeinen `.lib` `Version 2.4`, `.dcm` | `.kicad_pcb` / `.kicad_mod` S-Ausdruck, Version `20171130` | Legacy-Zeichenblatt | Kein eigenständiger `.kicad_dru` | Auf der Leiterplatte wurden benutzerdefinierte Pads, mehrschichtige Sperren und 3D-Modell-Offset-Änderungen hinzugefügt. Der Schaltplan blieb ein Erbe. |
| 6.0 | JSON `.kicad_pro`, `.kicad_prl` | `.kicad_sch` `20211123` | `.kicad_sym` `20211014` | `20211014` | `.kicad_wks` `20210606` | `.kicad_dru` `20200610` | Neue Schaltplan- und Symbol-S-Ausdrucksformate. |
| 7.0 | JSON `.kicad_pro` | `.kicad_sch` `20230121` | `.kicad_sym` `20220914` | `20221018` | `.kicad_wks` `20220228` | `20200610` | Textfelder, Schriftarten, DNP, Simulationsmodelländerungen, Netzbindungen, Bilder, Tropfenschlüsselwörter. |
| 8.0 | JSON `.kicad_pro` | `.kicad_sch` `20231120` | `.kicad_sym` `20231120` | `20240108` | `.kicad_wks` `20231118` | `20200610` | `generator_version`, V8-Bereinigung, PCB-Felder, generierte Objekte, UUID-Normalisierung. |
| 9.0 | JSON `.kicad_pro` | `.kicad_sch` `20250114` | `.kicad_sym` `20241209` | `20241229` | `.kicad_wks` `20231118` | `20200610` | Eingebettete Dateien, Tabellen, Regelbereiche, Komponentenklassen, Padstacks, Via-Stacks, beliebige Benutzerebenen. |
| 10.0 | JSON `.kicad_pro` | `.kicad_sch` `20260306` | `.kicad_sym` `20251024` | `20260206` | `.kicad_wks` `20231118` | `20200610` | Varianten, Jumper-Pads, Barcodes, Via-Schutz, Backdrill, geteilte Via-Typen, das Schreiben von Netcodes wurde eingestellt. |

## Stabile Versionsmatrix

| KiCad-Tag | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

## KiCad 4/5 Legacy-Grenze

KiCad 4 und 5 sind nicht nur ältere S-Expression-Versionen für Schaltplandaten. Sie
Verwenden Sie eine andere Schaltplan- und Symbolbibliotheksdateifamilie:

| Bereich | KiCad 4.0 | KiCad 5.0 / 5.1 |
| --- | --- | --- |
| Schematischer Header | `EESchema Schematic File Version 2` | `EESchema Schematic File Version 4` |
| Schematisches Makro | `EESCHEMA_VERSION 2` | `EESCHEMA_VERSION 4` |
| Kopfzeile der Symbolbibliothek | Normalerweise `EESchema-LIBRARY Version 2.3` | Normalerweise `EESchema-LIBRARY Version 2.4` |
| PCB-Version | `4` | `20171130` |

KiCad 5 PCB/Footprint-Version weist vor der KiCad 6-Entwicklungslinie auf:

| Version | Ändern |
| ---: | --- |
| 20160815 | Differentialpaareinstellungen pro Netzklasse |
| 20170123 | `EDA_TEXT`-Refaktor; verschoben `hide` |
| 20170920 | Lange Pad-Namen und benutzerdefinierte Pad-Form |
| 20170922 | Sperrzonen können auf mehreren Ebenen vorhanden sein |
| 20171114 | Speichern Sie den 3D-Modellversatz in mm statt in Zoll |
| 20171125 | Gesperrter/entsperrter Footprint-Text |
| 20171130 | Mit dem Parameter `offset` geschriebener 3D-Modelloffset |

Auswirkungen auf den Backport:

- KiCad 4/5-Schaltplanziele erfordern einen alten `.sch`-Writer, nicht nur einen
`.kicad_sch` Version neu geschrieben.
- KiCad 4/5-Symbolziele erfordern eine Legacy-Ausgabe `.lib` / `.dcm` oder eine explizite
Verlustbehaftete/nicht implementierte Warnung.
- KiCad 4-Board-Ziele verwenden Version `4`; KiCad 5-Board-Ziele verwenden `20171130`.
- V6+ UUIDs, Textfelder, eingebettete Dateien, Varianten, Tabellen, Regelbereiche, Komponente
Klassen, Padstacks, Via-Stacks, Backdrill und ähnliche Strukturen können nicht sein
direkt in V4/V5-Dateien gespeichert.

## Aktuelle Entwicklungsversionsmatrix

Der überprüfte KiCad `master`-Zweig ist bereits in die 11.0-Entwicklung übergegangen.
Diese Ergebnisse sind Post-10.0-Entwicklungselemente und dürfen nicht als KiCad gekennzeichnet werden
Unterstützung des stabilen Formats 10.0:

| Dateityp | Aktuelle Entwicklungsversion | Notizen |
| --- | ---: | --- |
| Vorstand `.kicad_pcb` | `20260603` | Knockout-Flag auf Tabellenzellen |
| Fußabdruck `.kicad_mod` | `20260603` | Footprints verwenden die PCB-S-Expression-Version |
| Schaltplan `.kicad_sch` | `20260512` | Netzketten |
| Symbolbibliothek `.kicad_sym` | `20260508` | Natives Ellipsenprimitiv |
| Arbeitsblatt `.kicad_wks` | `20231118` | Generatorversion / KiCad 8-Bereinigung |
| Designregeln `.kicad_dru` | `20200610` | Es wurde kein entwicklungsspezifischer Versionssprung gefunden |

Bisher gefundene Schritte für Entwicklungsversionen nach 10.0:

| Version | Dateityp | Ändern |
| ---: | --- | --- |
| 20260410 | Board / footprint | Extrudierter 3D-Körper |
| 20260508 | Board / footprint | Native PCB-Ellipsen- und Ellipsenbogen-Primitive |
| 20260508 | Schaltplan/Symbol | Native schematische/symbolische Ellipsen- und Ellipsenbogen-Primitive |
| 20260511 | Planke | Dielektrische frequenzabhängige Stapelmodelle |
| 20260512 | Platine / Schaltplan | Netzketten |
| 20260513 | Planke | Modus zum Füllen der Kupferdiebstahlzone |
| 20260521 | Board / footprint | Pad-Simulation elektrischer Typen |
| 20260603 | Board / footprint | Knockout-Flag auf Tabellenzellen |

## 6,0 bis 7,0

### Symbolbibliothek

| Version | Ändern |
| ---: | --- |
| 20220101 | Klassenflaggen |
| 20220102 | Schriftarten |
| 20220126 | Textfelder |
| 20220328 | Textfeld `start/end` wurde in `at/size` geändert |
| 20220331 | Textfarben |
| 20220914 | Anzeigenamen der Symboleinheiten |
| 20220914 | Property-IDs werden nicht mehr gespeichert |

### Schematisch

| Version | Ändern |
| ---: | --- |
| 20220101 | Kreise, Bögen, Rechtecke, Polys, Béziers |
| 20220102 | Strich-Punkt-Punkt |
| 20220103 | Beschriftungsfelder |
| 20220104 | Schriftarten |
| 20220124 | `netclass_flag` umbenannt in `directive_label` |
| 20220126 | Textfelder |
| 20220328 | Textfeld `start/end` wurde in `at/size` geändert |
| 20220331 | Textfarben |
| 20220404 | Standard-Schaltplansymbol-Instanzdaten |
| 20220622 | Neues Simulationsmodellformat |
| 20220820 | Korrektur der Standardsymbolinstanzdaten |
| 20220822 | Hyperlinks zu Textobjekten |
| 20220903 | Sichtbarkeit des Feldnamens |
| 20220904 | Feldoption nicht automatisch platzieren |
| 20220914 | DNP-Unterstützung |
| 20220929 | Property-IDs werden nicht mehr gespeichert |
| 20221002 | Instanzdaten wurden zurück zur Symboldefinition verschoben |
| 20221004 | Instanzdaten wurden zurück zur Symboldefinition verschoben |
| 20221110 | Blattinstanzdaten wurden in die Blattdefinition verschoben |
| 20221126 | Wert und Footprint aus Instanzdaten entfernt |
| 20221206 | Simulationsmodellfelder V6 bis V7 |
| 20230121 | `SCH_MARKER` Blattpfadserialisierung |

### PCB / Footprint

| Version | Ändern |
| ---: | --- |
| 20211226 | Radiales Maß |
| 20211227 | Der Speichenwinkel mit thermischer Entlastung wird außer Kraft gesetzt |
| 20211228 | `allow_soldermask_bridges` Footprint-Attribut |
| 20211229 | Strichformatierung |
| 20211230 | Abmessungen in Fußabdrücken |
| 20211231 | Private Footprint-Ebenen |
| 20211232 | Schriftarten |
| 20220131 | Textfelder |
| 20220211 | Die Unterstützung der V5-Zonenfüllstrategie wurde beendet |
| 20220225 | TEDIT entfernt |
| 20220308 | Eigenschaft für Aussparungstext und gesperrten Grafiktext |
| 20220331 | Auswahleinstellung „Auf allen Ebenen zeichnen“. |
| 20220417 | Automatische Maßgenauigkeit |
| 20220427 | Ausgeschlossene Edge.Cuts und Margin von privaten Footprint-Ebenen |
| 20220609 | Teardrop-Schlüsselwörter |
| 20220621 | Bildunterstützung |
| 20220815 | `allow-soldermask-bridges-in-FPs`-Flag |
| 20220818 | Erstklassige Netzbindungen |
| 20220914 | Nummernfelder in individueller Form |
| 20221018 | Via- und Pad-Zone-Layer-Verbindungen |

### Arbeitsblatt

| Version | Ändern |
| ---: | --- |
| 20220228 | Schriftartenunterstützung |

## 7,0 bis 8,0

### Symbolbibliothek

| Version | Ändern |
| ---: | --- |
| 20230620 | `ki_description` wurde in das Feld `Description` geändert |
| 20231120 | `generator_version` und V8-Bereinigung |

### Schematisch

| Version | Ändern |
| ---: | --- |
| 20230221 | Moderne Machtsymbole, editierbarer Wert = Netto |
| 20230409 | `exclude_from_sim`-Markup |
| 20230620 | `ki_description` wurde in das Feld `Description` geändert |
| 20230808 | Das Feld „`Sim.Enable`“ wurde in das Attribut „`exclude_from_sim`“ verschoben |
| 20230819 | Mehrere Ebenen der Vererbung von Bibliothekssymbolen |
| 20231120 | `generator_version` und V8-Bereinigung |

### PCB / Footprint

| Version | Ändern |
| ---: | --- |
| 20230410 | DNP-Attribut vom Schaltplan an `attr` weitergegeben |
| 20230517 | Pad- und Via-Teardrop-Parameter |
| 20230620 | PCB-Felder |
| 20230730 | Konnektivität grafischer Formen |
| 20230825 | Explizites Textfeld-Rahmenflag |
| 20230906 | Unterstützung mehrerer Bildtypen |
| 20230913 | Individuell geformte Pad-Speichen-Vorlagen |
| 20231007 | Generative Objekte |
| 20231014 | Normalisierung des V8-Dateiformats |
| 20231212 | Referenzbildsperre/UUIDs, Footprint-Boolesches Format |
| 20231231 | Generatoren und Gruppen verwenden `uuid` anstelle von `id` |
| 20240108 | Teardrop-Parameter wurden in explizite Boolesche Werte geändert |

### Arbeitsblatt

| Version | Ändern |
| ---: | --- |
| 20230607 | Bilder als Base64 gespeichert |
| 20231118 | `generator_version`- und V8-Dateiformatbereinigung |

## 8,0 bis 9,0

### Symbolbibliothek

| Version | Ändern |
| ---: | --- |
| 20240529 | Eingebettete Dateien |
| 20240819 | Der eingebettete Datei-Hash-Algorithmus wurde in Murmur3 geändert |
| 20241209 | `SCH_FIELD` private Flags |

### Schematisch

| Version | Ändern |
| ---: | --- |
| 20240101 | Tische |
| 20240417 | Regelbereiche |
| 20240602 | Blattattribute |
| 20240620 | Eingebettete Dateien |
| 20240716 | Mehrere Netzklassenzuweisungen |
| 20240812 | Netclass-Farbhervorhebung |
| 20240819 | Der eingebettete Datei-Hash-Algorithmus wurde in Murmur3 geändert |
| 20241004 | Das Symbol `hide` verwendet boolesche Werte |
| 20241209 | `SCH_FIELD` private Flags |
| 20250114 | Querverweise auf Textvariablen verwenden vollständige Pfade |

### PCB / Footprint

| Version | Ändern |
| ---: | --- |
| 20240201 | Überschreibungen verwenden nullfähige Eigenschaften |
| 20240202 | Tische |
| 20240225 | `solder_paste_margin` Rationalisierung |
| 20240609 | Schlüsselwort `tenting` |
| 20240617 | Tischwinkel |
| 20240703 | Benutzerebenentypen |
| 20240706 | Eingebettete Dateien |
| 20240819 | Der eingebettete Datei-Hash-Algorithmus wurde in Murmur3 geändert |
| 20240928 | Komponentenklassen |
| 20240929 | Komplexe Padstacks |
| 20241006 | Über Stapel |
| 20241007 | Leiterbahnen können eine Lötstopplackschicht und einen Rand tragen |
| 20241009 | Entwicklung des Bereichsformats für Platzierungsregeln |
| 20241010 | Grafische Formen können eine Lötstopplackschicht und einen Rand enthalten |
| 20241030 | Bemaßungspfeilrichtungen, `suppress_zeroes`-Normalisierung |
| 20241129 | Normalisierte `keep_text_aligned`- und Fülleigenschaften |
| 20241228 | Tropfenkurvenpunkte wurden in boolesche Werte geändert |
| 20241229 | Benutzerebenen auf beliebige Anzahl erweitert |

### Arbeitsblatt

Kein Worksheet-Versions-Bump; bleibt `20231118`.

## 9,0 bis 10,0

### Symbolbibliothek

| Version | Ändern |
| ---: | --- |
| 20250318 | `~` bedeutet nicht mehr leerer Text |
| 20250324 | Jumper-Pin-Gruppen |
| 20250829 | Abgerundete Rechtecke |
| 20250901 | Gestapelte Pin-Notation |
| 20250925 | Bus-Aliase in der Projektdatei |
| 20251024 | Aktualisierungen der Eigenschaftsformatierung: `do_not_autoplace`, `show_name` |

### Schematisch

| Version | Ändern |
| ---: | --- |
| 20250222 | Schraffierte Füllungen für Formen |
| 20250227 | Lokale Machtsymbole |
| 20250318 | `~` bedeutet nicht mehr leerer Text |
| 20250425 | UUIDs für Tabellen |
| 20250513 | Gruppen können den Designblock `lib_id` tragen |
| 20250610 | Regelbereiche unterstützen DNP und andere Flags |
| 20250827 | Individuelle Karosserievarianten |
| 20250829 | Abgerundete Rechtecke |
| 20250901 | Gestapelte Pin-Notation |
| 20250922 | Schematische Varianten |
| 20251012 | Unterstützung flacher Schaltplanhierarchien |
| 20251028 | Aktualisierungen der Eigenschaftsformatierung: `do_not_autoplace`, `show_name` |
| 20260101 | PCB-Varianten |
| 20260306 | Semantik der Variante `in_bom` korrigiert |

### PCB / Footprint

| Version | Ändern |
| ---: | --- |
| 20250210 | Textbox-Knockout |
| 20250222 | PCB-Formen schraffiert |
| 20250228 | IPC-4761 über Schutzfunktionen |
| 20250302 | Zonenschraffur-Offsets |
| 20250309 | Dynamische Zuweisungsregeln für Komponentenklassen |
| 20250324 | Jumper-Pads |
| 20250401 | Optimierung der Zeitbereichslänge |
| 20250513 | Gruppen können den Designblock `lib_id` tragen |
| 20250801 | `(island)` wurde in `(island yes/no)` geändert |
| 20250811 | Eigenschaft bei der Herstellung von Einpresspolstern |
| 20250818 | Footprints unterstützen benutzerdefinierte Ebenenanzahlen |
| 20250829 | Abgerundete Rechtecke |
| 20250901 | PCB-Punkte |
| 20250907 | UUIDs für Tabellen |
| 20250909 | Metadaten der Footprint-Einheit: Einheiten / Pins |
| 20250914 | `PCB_BARCODE` Objekte |
| 20250926 | Die Via-Typen sind unterteilt in „blind“, „vergraben“ und „durchgehend“. |
| 20251027 | Pad-to-Die verzögert Skalierungskorrektur |
| 20251028 | Das Schreiben von Netcodes wurde gestoppt; es handelt sich um interne Implementierungsdetails |
| 20251101 | Backdrill- und tertiäre Bohrunterstützung |
| 20260101 | PCB-Varianten mit Overrides pro Footprint |
| 20260206 | Korrekturen bei der Serialisierung von Barcodes und Variantenattributen |

### Arbeitsblatt

Kein Worksheet-Versions-Bump; bleibt `20231118`.

## 10,0 zur aktuellen Entwicklung

Verglichen mit KiCad 10-Zieldateien, dem überprüften aktuellen Entwicklungszweig
fügt diese neueren Formatierungsschritte hinzu:

| Version | Dateityp | Unterschied |
| ---: | --- | --- |
| 20260410 | Board / footprint | Extrudierte 3D-Körpermetadaten in Footprint-Modellblöcken |
| 20260508 | Board / footprint | Native PCB-Ellipsen- und Ellipsenbogen-Primitive |
| 20260508 | Schaltplan/Symbol | Native schematische/symbolische Ellipsen- und Ellipsenbogen-Primitive |
| 20260511 | Planke | Dielektrische frequenzabhängige Stapelmodellfelder |
| 20260512 | Planke | Netzketten-Aggregationsblock |
| 20260512 | Schematisch | Netzkettenaufzeichnungen |
| 20260513 | Planke | Modus zum Füllen der Kupferdiebstahlzone |
| 20260521 | Board / footprint | Elektrischer Typ der Pad-Simulation, serialisiert als `sim_electrical_type` auf Pads |
| 20260603 | Board / footprint | Tabellenzellen-`knockout`-Flag |

## Backport-Zielzusammenfassung aus aktuellen Entwicklungsdateien

Im Vergleich zu älteren unterstützten Zielen führt 10.99 neuere ein oder behält diese bei
Konstrukte, die beim Backportieren entfernt, vereinfacht oder umbenannt werden müssen:

| Ziel | Board-/Footprint-Ziel | Schematisches Ziel | Symbolziel | Hauptherabstufungsbereiche gegenüber der aktuellen Entwicklung |
| --- | ---: | ---: | ---: | --- |
| KiCad 10 | `20260206` | `20260306` | `20251024` | Entfernen Sie nur für die Entwicklung vorgesehene extrudierte Körpermetadaten, native Ellipsen, dielektrische Frequenzfelder, Netzketten, Kupferdiebstahl, elektrische Pad-Simulationstypen und Knockout-Flags für Tabellenzellen |
| KiCad 9 | `20241229` | `20250114` | `20241209` | KiCad 10 Elemente plus PCB-Formschraffur, Durchkontaktierungsschutz, Zonenschraffur-Offsets, Jumper-Pads, Gruppen-Designblock-IDs, benutzerdefinierte Ebenenanzahlen, abgerundete Rechtecke, PCB-Punkte, Tabellen-UUIDs, Barcodes, geteilte Durchkontaktierungstypen, Netcode-Auslassung, Hinterbohren/Nachbearbeitung, PCB-Varianten, Schaltplanvarianten/Körperstile/abgerundete Rechtecke/gestapelte Stifte/Eigenschaftsformatierung |
| KiCad 8 | `20240108` | `20231120` | `20231120` | KiCad 9-Elemente plus Tabellen, eingebettete Dateien, Komponentenklassen, Padstacks, Via-Stacks, Regelbereiche, Tenting, Benutzerebenenerweiterung, Blattattribute, mehrere Netzklassenzuweisungen, Netzklassen-Farbhervorhebung |
| KiCad 7 | `20221018` | `20230121` | `20220914` | KiCad 8 Elemente plus PCB-Felder, DNP-Attributweitergabe, moderne Teardrops, benutzerdefinierte Pad-Spoke-Vorlagen, Generatoren, UUID/ID-Normalisierung, Textfelder, Bilder, Netzbindungen, Schriftart-/Feldformatierung, Regelbereiche, moderne Schaltplansimulation/Ausschlussflags |

## Abdeckung der C++-Backport-Implementierung

Die `kicad-backport-cplus`-CLI implementiert versionierte S-Ausdruck-Umschreibungen.
Es löst Release-Aliase pro Dokumenttyp auf und wendet dann Downgrade-Regeln an
schreibt das Zielfeld `version`. Es akzeptiert auch ein rohes numerisches Dateiformat
Version zum Testen eines bestimmten Parser-Cutoffs.

Unterstützte Aliaszuordnungen im Code:

| Alias | Symbol | Schematisch | Planke | Fußabdruck | Arbeitsblatt | Designregeln |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |
| 10.99 | 20251024 | 20260306 | 20260603 | 20260603 | 20231118 | 20200610 |

Wenn die Quelldatei bereits genau die angeforderte numerische Version hat, wird die
Der Konverter kopiert es unverändert. Wenn die Quellversion niedriger als die Zielversion ist,
Die C++-Implementierung wendet jetzt zuvor eingeschränkte Kompatibilitäts-Upgrades an
Schreiben des angeforderten Felds `version`:

| Art | Upgrade-Normalisierung implementiert |
| --- | --- |
| Symbolbibliothek | Erweitern Sie Atome im alten Schriftstil für moderne Ziele. Erweitern Sie die Sichtbarkeit der Pins durch Atome. Eigenschaft `hide` aus `effects` verschieben; Entfernen Sie alte Property-IDs. |
| Schematisch | Benennen Sie `tstamp` in `uuid` um; `netclass_flag` in `directive_label` umbenennen; Konvertieren Sie das alte Textfeld `start/end` in `at/size`; Erweitern Sie Atome für die Sichtbarkeit älterer Schriftarten und Pins. Eigenschaft `hide` aus `effects` verschieben; Entfernen Sie alte Property-IDs. |
| Board / footprint | Benennen Sie `tstamp` für moderne Ziele in `uuid` um. Erweitern Sie die Atome im Schriftstil. Footprint erweitern `dnp` Atome; boolesche Werte auf KiCad 7-Stil `yes/no` normalisieren; veraltetes `tedit` entfernen; Konvertieren Sie optional alte numerische Netzreferenzen in Netznamen. |

Dies ist keine vollständige semantische Upgrade-Engine. es normalisiert nur die Syntax
Der Konverter weiß bereits, wie man sich ausdrückt.

### Release-Target-Abdeckung implementiert

Die C++-Regeln sind Cutoff-gesteuert, sodass jeder Release-Alias ​​die Regeln aktiviert, deren
Cutoffs sind neuer als die Zielversion dieser Dateifamilie. Die folgende Zusammenfassung
listet die praktische Abdeckung für die stabilen Nicht-V6-Ziele auf.

#### KiCad 10 Target

KiCad 10-Ziele entfernen größtenteils Post-10.0-/aktuelle Entwicklungskonstrukte:

| Art | Handhabung implementiert |
| --- | --- |
| Symbolbibliothek | Entfernen Sie native Ellipsen- und Ellipsenbogen-Primitive, die nach dem 10.0-Symbolziel eingeführt wurden. |
| Schematisch | Entfernen Sie nach 10.0 `locked`-Felder, native Ellipsenprimitive und `net_chain` / `net_chains`. |
| Board / footprint | Entfernen oder stufen Sie nach 10.0 typisierte/extrudierte Modellblöcke, native Ellipsenprimitive, dielektrische Frequenzstapelfelder, Netzketten, Kupferdiebstahl-Füllmodus, Pad `sim_electrical_type` und Tabellenzelle `knockout` herunter oder stufen Sie sie herunter. |
| Projektseitige Dateien | Für das V10-Suffix wird kein veralteter `.kicad_prl` oder keine Umschreibung der Bibliothekstabellenkompatibilität generiert. |

#### KiCad 9 Target

KiCad 9-Ziele entfernen KiCad 10 und die Syntax der aktuellen Entwicklung unter Beibehaltung
Funktionen, die für die KiCad 9-Dateiversionen gültig sind:

| Art | Handhabung implementiert |
| --- | --- |
| Symbolbibliothek | Entfernen Sie Jumper-Pin-Gruppen, abgerundete Rechtecke, native Ellipsen, Symbolklassen-Flags `in_pos_files`, `duplicate_pin_numbers_are_jumpers`, `power`, Eigenschaft `show_name` / `do_not_autoplace` und Schriftart `face`. |
| Schematisch | Entfernen Sie abgerundete Rechtecke, schematische Varianten, native Ellipsen, Netzketten, Post-Target-`locked`, `embedded_fonts`, benutzerdefinierte Körperstile, Blattmontage-/Simulationsflags, Symbol `in_pos_files`, Jumper-/Leistungsklassenflags, Schriftart `face`, Eigenschaftsformatierungsfelder und Stammknoten `group`. |
| Board / footprint | IPC-4761 durch Schutz, Jumper-Pad-Felder, Komponentenklassen-Platzierungsquellen, PCB-Schraffurfüllungen, benutzerdefinierte Ebenenanzahlen, abgerundete Rechtecke, PCB-Punktobjekte, Barcodes, Backdrill-/Nachbearbeitungsfelder, PCB-Varianten, aktuelle Entwicklungsfunktionen und Schriftart `face` entfernen oder herabstufen; Erstellen Sie alte Netzcodes für numerische Karten neu. Tenting wird für diesen Zielbereich von booleschen Front-/Back-Listen auf Legacy-Atome herabgestuft. |
| Projektseitige Dateien | Für V9 wird kein Legacy-`.kicad_prl`-Rewrite generiert. |

#### KiCad 8 Target

KiCad 8-Ziele entfernen die Syntax von KiCad 9/10/aktuell und normalisieren auch mehrere
Die späte KiCad-8-Entwicklung geht zurück auf die 8.0.0-Dateiversionen:

| Art | Handhabung implementiert |
| --- | --- |
| Symbolbibliothek | Entfernen Sie in V9+ eingebettete Dateien/private Felder und in V10+ Jumper-, abgerundete Rechteck- und Ellipsensyntax. `embedded_fonts`, Schriftart `face`, Symbol-/Eigenschaftsformatierungsfelder entfernen; Fügen Sie Legacy-Eigenschafts-IDs hinzu und verschieben Sie die Eigenschaftssichtbarkeit in `effects`; Konvertieren Sie den Schriftstil und die Pin-Sichtbarkeit von Booleschen Werten in die ältere Atom-Syntax. |
| Schematisch | Entfernen Sie V9+-Tabellen, Regelbereiche, eingebettete Dateien/private Felder und die V10+-Syntax für abgerundete Rechtecke, Varianten, Textkörper und Ellipsen/Netzketten. Text- und Blattsimulations-/Assembly-Flags, Symbol-/Eigenschaftsformatierungsfelder und Schriftart `face` entfernen; Fügen Sie Legacy-Eigenschafts-IDs hinzu und verschieben Sie die Eigenschaftssichtbarkeit in `effects`; Konvertieren Sie boolesche Werte für die Sichtbarkeit von Schriftarten und Pins in die ältere Atom-Syntax. Entfernen Sie Root-`group`-Knoten. |
| Board / footprint | Entfernen Sie V9+-Tabellen, Tenting, eingebettete Dateien/Schriftarten, Komponentenklassen, komplexe Padstacks, Via-Stacks, Regelbereiche, Via-Schutz, beliebige Benutzer-Layer-Qualifizierer, benutzerdefinierte Layer-Anzahlen, abgerundete Rechtecke, PCB-Punkte, Barcodes, Backdrill/Nachbearbeitung, Varianten und aktuelle Entwicklungsfunktionen. Entfernen Sie außerdem Grafik-/Spur-Lötstopprand-/Ebenenfelder, Tabellenzellenwinkel, Textrender-Caches, Textfeld-/Tabellenzellen-/Ebenen-Knockouts, Modell `hide` und Schriftart `face` und fügen Sie ältere numerische Netcodes hinzu. `solder_paste_margin_ratio` wird in `solder_paste_ratio` umbenannt. |
| Projektseitige Dateien | Generieren Sie alte Anzeigeeinstellungen für die numerische ID `.kicad_prl` für V8-Boards. |

#### KiCad 7 Target

KiCad 7-Ziele entfernen die KiCad 8/9/10/aktuelle Syntax und wenden zusätzlichen Parser an
Kompatibilitätsänderungen rund um PCB-Felder, UUIDs und Footprint-Daten:

| Art | Handhabung implementiert |
| --- | --- |
| Symbolbibliothek | Entfernen Sie V8+ `generator_version`, eingebettete Schriftarten/Dateien, private V9-Felder, V10-Jumper-/Rund-/Ellipsensyntax, Symbol `exclude_from_sim`, Positionsdatei- und Eigenschaftsformatierungsfelder, Jumper-/Leistungsklassenflags und Schriftart `face`; Legacy-Eigenschafts-IDs hinzufügen; Eigenschaftssichtbarkeit in `effects` verschieben; Konvertieren Sie boolesche Werte für Schriftart und Sichtbarkeit in Atomsyntax. |
| Schematisch | Entfernen Sie V8+ `generator_version` und `fields_autoplaced`, V9+ Tabellen/Regelbereiche/eingebettete/private Felder, V10+ abgerundete/Varianten-/Körperstil-Syntax, Post-Target-Simulationsausschlussfelder, Blattmontage-/Simulationsflags, Symbol-/Eigenschaftsformatierungsfelder, Schriftart `face` und Stammknoten `group`. UUID-Atome werden für KiCad 6/7-Parser nicht in Anführungszeichen gesetzt und die Sichtbarkeit/IDs von Eigenschaften werden auf die alte Platzierung herabgestuft. |
| Board / footprint | Entfernen Sie in V8+ generierte Objekte, Teardrops, Tabellen, eingebettete Dateien/Schriftarten, Komponentenklassen, Pad-/Via-Stacks, Regelbereiche, Via-Schutz und neuere Zielsyntax. Konvertieren Sie Typqualifizierer der Benutzerebene in `user`; Entfernen Sie Grafik-/Spur-Lötmaskenfelder, Tischwinkel, Render-Caches, Knockout-Flags, Modell `hide`, grafische Netzkonnektivität, gruppengesperrte Felder, Via-Layer-Verbindungsfelder, Footprint-Jumper-/Net-Tie-/Einheitsfelder, Schriftart `face` und Legacy-inkompatible Footprint-Attr-Atome. Konvertieren Sie PCB-Footprint-Eigenschaften zurück in `fp_text`, benennen Sie `uuid`/`id` wieder in `tstamp`/`id`-Legacy-Formulare um, benennen Sie Lötpasten- und Thermofelder um, konvertieren Sie Striche in Legacy-`width`, konvertieren Sie Bemaßungen in sichtbare Grafiken, stufen Sie Boolesche Werte/Anwesenheitsatome herab und erstellen Sie numerische Netcodes neu. |
| Projektseitige Dateien | Generieren Sie alte Anzeigeeinstellungen für die numerische ID `.kicad_prl` für V7-Boards. |

### Dokumentenerkennung und Projektabwicklung

Die C++-Implementierung erkennt den KiCad-Dokumenttyp hauptsächlich im Stammverzeichnis
S-Ausdruckskopf:

| Wurzelkopf | Art |
| --- | --- |
| `kicad_symbol_lib` | Symbolbibliothek |
| `kicad_sch` | Schematisch |
| `kicad_pcb` | Planke |
| `footprint` | Fußabdruck |
| `kicad_dru` | Designregeln |
| `kicad_wks`, `drawing_sheet` | Arbeitsblatt |

Wenn der Root-Kopf fehlt oder unbekannt ist, wird auf die Dateierweiterung zurückgegriffen:
`.kicad_sym`, `.kicad_sch`, `.kicad_pcb`, `.kicad_mod`, `.kicad_dru` und
`.kicad_wks`. Die älteren Versionen `.sch`, `.lib`, `.dcm` und `.pro` werden ebenfalls erkannt
ältere KiCad-Typen, die direkte Konvertierung aus diesen älteren Dateifamilien jedoch nicht
in der aktuellen Phase umgesetzt.

Beim Konvertieren eines Projektverzeichnisses oder `.kicad_pro` werden nur bearbeitbare Dateien kopiert
KiCad-Projekteingaben und allgemeine lokale 3D-Modelldateien. Generierte Ausgaben,
Verlaufs-/Sicherungsordner, Gerbers, Fertigungsausgaben, Stücklisten und temporäre Dateien
werden übersprungen. Für KiCad 6-, 7- und 8-Board-Ziele wird auch ein Legacy erstellt
`.kicad_prl` lokale Board-Anzeigeeinstellungen mit numerischem `visible_items`, vollständig
`visible_layers` und die ältere Metaversion für lokale Einstellungen konvertierten also Objekte
bleiben in älteren GUIs sichtbar. Für das KiCad 6-Projekt ist dies zusätzlich vorgesehen
Entfernt `version`-Knoten der obersten Ebene aus `sym-lib-table` / `fp-lib-table` und
Erstellt schematische Hierarchieinstanztabellen auf Stammebene über untergeordnete Blätter hinweg neu.

### Regeln der Symbolbibliothek

Generische Parser-Gates entfernen diese eingeführten Knoten, wenn das Zieldateiformat vorliegt
ist älter als die Einführungsversion:

| Eingeführt | Köpfe entfernt | Grund |
| ---: | --- | --- |
| 20220126 | `text_box`, `textbox` | Symboltextfelder |
| 20240529 | `embedded_files`, `embedded_file` | Eingebettete Dateien |
| 20241209 | `private` | Private SCH_FIELD-Flags |
| 20250324 | `pin_group`, `pin_groups` | Jumper-Pin-Gruppen |
| 20250829 | `rounded_rectangle`, `roundrect` | Abgerundete Rechtecke |
| 20260508 | `ellipse`, `ellipse_arc` | Native Ellipsenprimitive |

Kompatibilitätsänderungen:

| Ziel-Cutoff | Umschreiben |
| ---: | --- |
| `< 20231120` | Entfernen Sie die Stammfelder `generator_version` |
| `< 20241209` | Entfernen Sie `embedded_fonts`; Legacy-Eigenschafts-IDs hinzufügen; Verschieben Sie die Flags der Eigenschaft `hide` nach `effects` |
| `< 20230409` | Entfernen Sie die Simulationsausschlussflags der Symbolbibliothek `symbol/exclude_from_sim` |
| `< 20240108` | Konvertieren Sie die Listen der Schriftarten `(bold yes/no)` und `(italic yes/no)` in veraltete Präsenzatome |
| `<= 20241209` | Entfernen Sie die Felder der Schriftart `face` |
| `< 20241004` | Konvertieren Sie boolesche `hide`-Listen in Legacy-Atome. `pin_names` / `pin_numbers`-Ausblendlisten reduzieren |
| `<= 20211014` | Fügen Sie KiCad 6-Standard-Eigenschafts-IDs hinzu: `Reference=0`, `Value=1`, `Footprint=2`, `Datasheet=3`, `ki_keywords=4`, `ki_description=5`, `ki_fp_filters=6` |
| `< 20251024` | Symbol `in_pos_files` entfernen; Entfernen Sie die Eigenschaften `show_name` und `do_not_autoplace` |
| `< 20250324` | `duplicate_pin_numbers_are_jumpers` entfernen |
| `< 20250227` | Entfernen Sie die Flags der Symbolklasse `power` |

### Schematische Regeln

Generische Parser-Gates:

| Eingeführt | Köpfe entfernt | Grund |
| ---: | --- | --- |
| 20220126 | `text_box`, `textbox` | Schematische Textfelder |
| 20220622 | `simulation_model`, `sim_model` | Neues Simulationsmodellformat |
| 20240101 | `table` | Schematische Tabellen |
| 20240417 | `rule_area` | Schematische Regelgebiete |
| 20240620 | `embedded_files`, `embedded_file` | Eingebettete Dateien |
| 20241209 | `private` | Private SCH_FIELD-Flags |
| 20250829 | `rounded_rectangle`, `roundrect` | Abgerundete Rechtecke |
| 20250922 | `variants`, `variant` | Schematische Varianten |
| 20260508 | `ellipse`, `ellipse_arc` | Native Ellipsenprimitive |
| 20260512 | `net_chain`, `net_chains` | Schematische Netzketten |

Kompatibilitätsänderungen:

| Ziel-Cutoff | Umschreiben |
| ---: | --- |
| `< 20231120` | Entfernen Sie `generator_version`; Entfernen Sie `fields_autoplaced` aus Symbolen und Blättern |
| `< 20260326` | Entfernen Sie die nach dem Ziel eingeführten schematischen `locked`-Felder |
| `< 20260306` | Entfernen Sie `embedded_fonts`; Blatt `exclude_from_sim`, `in_bom`, `on_board`, `dnp` entfernen; Entfernen Sie die `group`-Knoten des Stammschaltplans |
| `< 20250827` | Entfernen Sie `body_styles` und `body_style` |
| `< 20250114` | Text/Textfeld `exclude_from_sim` entfernen |
| `<= 20230121` | Entfernen Sie alle verbleibenden `exclude_from_sim` |
| `< 20220822` | Entfernen Sie die Felder „text“, „label“ und „directive-label“ `hyperlink` |
| `< 20220914` | Entfernen Sie die `dnp`-Flags für platzierte Symbole |
| `< 20220124` | Benennen Sie die Root-`directive_label`-Knoten wieder in `netclass_flag` um. |
| `< 20251024` | Symbol `in_pos_files` entfernen |
| `< 20250324` | `duplicate_pin_numbers_are_jumpers` entfernen |
| `< 20250227` | Entfernen Sie die Flags der Symbolklasse `power` |
| `< 20241004` | Konvertieren Sie boolesche `hide`-Listen in Legacy-Atome. Reduzieren Sie die Pin-Sichtbarkeit und verbergen Sie die Listen |
| `<= 20211123` | Entfernen Sie die `pin/alternate`-Definitionen des Bibliothekssymbols |
| `< 20240108` | Konvertieren Sie boolesche Listen in Fett-/Kursivschrift in alte Atome |
| `<= 20250114` | Entfernen Sie die Felder der Schriftart `face` |
| `<= 20230121` | `uuid`-Atome für KiCad 6/7-Parser in Anführungszeichen setzen |
| `<= 20211123` | Generieren Sie `sheet_instances` und `symbol_instances` auf KiCad 6-Stammebene, wenn der Quell-Stammschaltplan bereits Instanzdaten enthält; Untergeordnete Blätter erhalten keine Stamminstanztabellen |
| `<= 20211123` | Fügen Sie KiCad 6-Standardschemata-Eigenschafts-IDs hinzu und normalisieren Sie Blatteigenschaftsnamen/-IDs zu `Sheet name=0` und `Sheet file=1` |
| `<= 20211123` | Entfernen Sie symbolinterne `instances`-Blöcke, nachdem die KiCad 6-Root-Instanztabelle generiert wurde |
| `< 20241209` | Fügen Sie Legacy-Eigenschafts-IDs hinzu; Verschieben Sie die Flags der Eigenschaft `hide` nach `effects` |
| `< 20251028` | Entfernen Sie die Eigenschaften `show_name` und `do_not_autoplace` |

### Vorstands- und Footprint-Regeln

Generische Parser-Gates:

| Eingeführt | Köpfe entfernt | Grund |
| ---: | --- | --- |
| 20220131 | `gr_text_box`, `fp_text_box`, `text_box`, `textbox` | PCB-Textfelder |
| 20220621 | `image` | PCB-Bildobjekte |
| 20220818 | `net_tie`, `net_ties` | Erstklassige Netzaufbewahrung |
| 20231007 | `generated` | Generative Objekte |
| 20240108 | `teardrop`, `teardrops` | Teardrop-Parameter |
| 20240202 | `table` | PCB-Tische |
| 20240609 | `tenting` | Stichwort „Zelten“. |
| 20240706 | `embedded_files`, `embedded_file`, `embedded_fonts` | Eingebettete Dateien |
| 20240928 | `component_class`, `component_classes` | Komponentenklassen |
| 20240929 | `padstack` | Komplexe Padstacks |
| 20241006 | `via_stack`, `viastack` | Über Stapel |
| 20241009 | `rule_area` | Platzierungs-/Regelbereiche |
| 20250228 | `via_protection`, `covering`, `plugging`, `filling`, `capping` | IPC-4761 über Schutz |
| 20250818 | `custom_layer_count`, `custom_layer_counts` | Anzahl der benutzerdefinierten Footprint-Layer |
| 20250829 | `rounded_rectangle`, `roundrect` | Abgerundete Rechtecke |
| 20250901 | `point` | PCB-Punktobjekte |
| 20250914 | `barcode`, `pcb_barcode`, `gr_barcode`, `fp_barcode` | PCB-Barcode-Objekte |
| 20251101 | `backdrill`, `tertiary_drill`, `front_post_machining`, `back_post_machining` | Backdrill- und Tertiärbohrfelder |
| 20260101 | `variants`, `variant` | PCB-Varianten |
| 20260410 | `extruded` | Extrudierte 3D-Körpermodelle mit Fußabdruck |
| 20260508 | `gr_ellipse`, `gr_ellipse_arc`, `fp_ellipse`, `fp_ellipse_arc` | Native PCB-Ellipsenprimitive |
| 20260511 | `spec_frequency`, `dielectric_model` | Von der dielektrischen Frequenz abhängige Stapelfelder |
| 20260512 | `net_chains`, `net_chain` | PCB-Netzketten |
| 20260513 | `thieving` | Modus zum Füllen der Kupferdiebstahlzone |

Aktuelle Hinweise zur Entwicklungsberichterstattung von lokalem KiCad `10.99.0-1273-gd90e32b6a0`:

| Eingeführt | Handhabung | Notizen |
| ---: | --- | --- |
| 20260521 | Umgesetzt | Pad-Kind `sim_electrical_type` wird für Ziele entfernt, die älter als `20260521` sind. |
| 20260603 | Umgesetzt | Das untergeordnete Tabellenzellenelement `knockout` wird kontextabhängig für Ziele entfernt, die älter als `20260603` sind. `knockout` wird nicht als globales Token-Gate verwendet, da es auch von anderen Objekttypen verwendet wird. |

Kompatibilitätsänderungen:

| Ziel-Cutoff | Umschreiben |
| ---: | --- |
| `< 20260410` | Entfernen Sie typisierte/extrudierte 3D-Modellblöcke, indem Sie `model`-Knoten entfernen, die `type` enthalten. |
| `< 20260513` | Ersetzen Sie den Füllmodus „Kupferdiebstahlzone“ durch Polygonfüllung |
| `>= 20220225` | Entfernen Sie veraltete Footprint-`tedit`-Felder |
| `>= 20200628` | Entfernen Sie veraltete Board-`visible_elements`-Einstellungen |
| `< 20260603` | Entfernen Sie die `knockout`-Felder der PCB-Tabellenzelle |
| `< 20240703` | Konvertieren Sie die Typqualifizierer der Benutzerebene `front`, `back`, `auxiliary` in `user` |
| `< 20241010` | Entfernen Sie grafische `solder_mask_margin`-Felder |
| `< 20241030` | Konvertieren Sie boolesche Dimensionsfelder in Legacy-Atome. Dimension `arrow_direction` entfernen |
| `< 20250210` | PCB-Text entfernen `render_cache`; Textfeld entfernen `knockout`; `knockout`-Atome aus Ebenenlisten entfernen; Fügen Sie bei Bedarf `filled_areas_thickness no` zu zwischengespeicherten Zonenfüllungen hinzu |
| `< 20241009` | Entfernen Sie die Zonenfelder `placement` |
| `<= 20221018` | Zone `attr` entfernen; Pad/Zone entfernen `thermal_bridge_angle`; Pad/Zone `thermal_bridge_width` in Legacy `thermal_width` umbenennen |
| `< 20240108` | Entfernen Sie `setup/allow_soldermask_bridges_in_footprints`; Gruppe `locked` entfernen; Entfernen Sie über Layer-Verbindungsfelder wie `keep_end_layers`, `start_end_only` und `zone_layer_connections` |
| `< 20241007` | Entfernen Sie die Spurfelder `solder_mask_margin` und `solder_mask_layer` |
| `< 20240617` | PCB-Tabellenzelle `angle` entfernen |
| `< 20260521` | Pad `sim_electrical_type` entfernen |
| `< 20250228` | Konvertieren Sie Tenting-Boolesche Listen vorne/hinten in Legacy-Atome. IPC-4761-Schutzfelder entfernen |
| `< 20231212` | Konvertieren Sie die booleschen Listen `locked` und `hide` in Präsenzatome. entferne `unlocked`; Modell `hide` entfernen |
| `< 20231014` | `generator_version` entfernen |
| `< 20230924` | Konvertieren Sie `pcbplotparams` `yes/no` Boolesche Werte in `true/false`; Formfüllung `no` in `none` umwandeln |
| `< 20230730` | Entfernen Sie die Konnektivität der grafischen Form `net` |
| `< 20240108` | Konvertieren Sie boolesche Listen in Fett-/Kursivschrift in alte Atome |
| `< 20230620` | Konvertieren Sie die Footprint-Eigenschaften `Reference` und `Value` zurück in `fp_text reference` / `fp_text value`; dies ist eine PCB-Referenzbezeichner-Kompatibilitätsanforderung. `Description` in `ki_description` konvertieren; Ordnen Sie `sheetname`/`sheetfile` den Eigenschaften zu |
| `< 20231231` | Benennen Sie die bereichsbezogenen `uuid`-Felder wieder in `tstamp` um; Benennen Sie die Gruppe/den generierten `uuid` wieder in `id` um. |
| `< 20250324` | Entfernen Sie die Footprint-Jumper-Pad-Felder: `duplicate_pad_numbers_are_jumpers` und `jumper_pad_groups` |
| `<= 20221018` | Entfernen Sie die Footprint-Attribute `dnp`, `net_tie_pad_groups`, `units` und `allow_missing_courtyard`; Pad/Via entfernen `remove_unused_layers`; Bemaßungen in sichtbare Grafiken umwandeln; Legacy-inkompatiblen `locked` entfernen; Downgrade kostenlos über Felder; Konvertieren Sie PCB-Grafik-`stroke`-Blöcke in ältere `width`-Felder |
| `< 20250309` | Entfernen Sie `component_class` aus den Platzierungsregeln |
| `< 20250222` | Konvertieren Sie PCB-Schraffur-/Umkehrschraffur-/Kreuzschraffur-Formfüllungen in Vollfüllungen |
| `<= 20241229` | Entfernen Sie die Felder für die PCB-Schriftart `face` |
| `< 20251101` | Pad/Via-Nachbearbeitungsfelder entfernen |
| `< 20251028` | Erstellen Sie alte Netcodes und Net-Deklarationen auf Root-Ebene für numerische Boards neu |

KiCad 6-Parser-spezifische Korrekturen, die in Tests auf Projektebene beobachtet wurden:

| Bereich | Fix implementiert |
| --- | --- |
| PCB-Setup | Entfernen Sie `setup/allow_soldermask_bridges_in_footprints` für Board-Ziele vor 8. |
| PCB-Footprints | Entfernen Sie `net_tie_pad_groups`, `units`, Jumper-Pad-Gruppen und `allow_missing_courtyard`-Attr-Atome für KiCad 6/7-Board-Ziele. |
| PCB-Zonen und Pads | Entfernen Sie Zone `attr`, entfernen Sie `thermal_bridge_angle` und benennen Sie `thermal_bridge_width` in `thermal_width` für KiCad 6/7-Board-Ziele um. |
| PCB-Texte und Tabellen | Entfernen Sie Text `render_cache`, Textfeld `knockout`, Tabellenzelle `knockout` und Ebenenliste `knockout`, wo ältere Parser sie ablehnen. |
| Symbolbibliotheken | Entfernen Sie das Symbol `exclude_from_sim` für Ziele, die älter als `20230409` sind, und fügen Sie KiCad 6-Standard-Eigenschafts-IDs hinzu. |
| Schaltpläne | Entfernen Sie den Pin `alternate`, generieren Sie KiCad 6-Root-Instanztabellen, normalisieren Sie Root-Blatt-Instanzpfade, normalisieren Sie Blatteigenschaftsnamen/-IDs und entfernen Sie symbolintern `instances`. Platzierte Symbol-Pin-UUID-Blöcke werden absichtlich beibehalten, da KiCad 6 sie für die Beispielzuordnung verwendet. |
| Projektseitige Dateien | Generieren Sie Anzeigeeinstellungen für numerische IDs `.kicad_prl` für V6/V7/V8 und entfernen Sie `version`-Knoten der obersten Ebene der Bibliothekstabelle für V6. |

### Arbeitsblatt und Designregeln

Für die Arbeitsblattverarbeitung ist derzeit ein Parser-Gate implementiert:

| Ziel-Cutoff | Umschreiben |
| ---: | --- |
| `< 20220228` | Entfernen Sie die `font`-Blöcke des Arbeitsblatts |

Designregeln werden erkannt und verfügen über Zielversionsaliase, jedoch kein Downgrade
Umschreibungen werden derzeit implementiert, da das Dateiformatversionsmakro bestehen bleibt
`20200610` in den verfolgten KiCad-Versionen.

### Warnungs- und Berichtssemantik

Bei jeder implementierten Entfernung oder Kompatibilitätsumschreibung, die den Baum ändert, wird ein hinzugefügt
Warnung. Generische Feature-Gates melden die Anzahl der entfernten Knoten und die
Einführungsversion. Zu den Berichten gehören Pfad, erkannte Art, Quellversion,
Zielversion, geändertes Flag und Warnungen.

## Konverteranforderungen

### Pfad lesen

- Behalten Sie die Quelldatei `version` bei; nicht nur als aktuell interpretieren
KiCad-Format.
- Unterstützen Sie Kompatibilitätsaliase für ältere Dateien:
  - `page` bis `paper`
  - Legacy-Overbar `~...~` bis `~{...}`
  - Altes Textfeldformat `start/end` in neues `at/size`
  - Alter `id` bis `uuid`
  - Alte boolesche/Präsenz-Token-Formate zu expliziten booleschen Werten
- Erkennen Sie zukünftige Formate und geben Sie einen eindeutigen Fehler oder eine definierte Downgrade-Strategie zurück.

### Pfad schreiben

- `--target-version` muss mehr tun, als nur die Versionsnummer der obersten Ebene zu ändern. Es
muss die Semantik entsprechend dem angeforderten Ziel beschneiden oder neu schreiben.
- Jede Zielversion benötigt Feature Gates:
  - KiCad 6 darf keine V7-Simulationsmodellfelder, DNP oder Post-V6-Textfelder schreiben
Strukturen.
  - KiCad 7 darf keine Strukturen schreiben, die erst nach V8 stabil wurden
`generator_version` Bereinigung.
  - KiCad 8 darf keine eingebetteten V9-Dateien, Komponentenklassen oder komplexen Dateien schreiben
Padstacks.
  - KiCad 9 darf keine V10-Varianten, Barcodes, Backdrills, Split-via-Typen usw. schreiben
ähnliche Konstrukte.
  - KiCad 10 darf keine nativen extrudierten Körpermetadaten der aktuellen Entwicklung schreiben
Ellipsen, dielektrische Frequenzfelder, Netzketten, Kupferdiebstahl, Pad
elektrische Simulationstypen oder Tischzellen-Knockout-Flags.
- Verlustbehaftete Downgrades sollten Warnungen oder Sidecar-Metadaten anstelle von Stille erzeugen
Streichung.

### Testpfad

- Erstellen Sie minimale Vorrichtungen für KiCad 6, 7, 8, 9 und 10:
  - Symbolbibliothek
  - Schematisch
  - Planke
  - Fußabdruck
  - Arbeitsblatt
  - Designregeln
- Bei jeder versionübergreifenden Konvertierung sollte Folgendes überprüft werden:
  - Die Quellversion wurde korrekt gelesen
  - Die Zielversionsnummer ist korrekt geschrieben
  - Unzulässige Token werden entfernt oder herabgestuft
  - Die Schlüsselsemantik bleibt erhalten
  - Warnungen decken verlustbehaftete Konvertierungen ab

## Wartungshinweise

Beim Hinzufügen zukünftiger Versionsunterschiede:

1. Fügen Sie zuerst die Versionsmatrix hinzu oder aktualisieren Sie sie.
2. Fügen Sie einen neuen Intervallabschnitt hinzu, z. B. `10.0 to 11.0` oder
`10.99 / current to 11.99 / current`.
3. Halten Sie die Ergebnisse des Entwicklungszweigs von den veröffentlichten stabilen Tags getrennt, bis der
Die entsprechende KiCad-Version ist markiert.
4. Aktualisieren Sie die Zusammenfassung des Backport-Ziels, wenn eine neue Quellversion eingeführt wird
Konstrukte, die sich auf bestehende Downgrade-Ziele auswirken.
5. Verfolgen Sie `.kicad_pro` JSON-Schemamigrationen in einem separaten Dokument.

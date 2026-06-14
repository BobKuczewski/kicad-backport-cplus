# KiCad-Backport-Konverter: Formatunterschiede und Konvertierungsverhalten

Dieses Dokument beschreibt die KiCad-Formatunterschiede, die von der aktuellen `kicad-backport`-Implementierung tatsächlich behandelt werden: Dateifamilien, Versionsanker, Konvertierungslogik, Rewrite-Regeln und verlustbehaftete Pfade. Die maßgebliche Referenz für KiCad-Dateiformate bleibt die KiCad-Entwicklerdokumentation.

- https://dev-docs.kicad.org/en/file-formats/index.html

## Kurzüberblick für KiCad-Entwickler

| Thema | Verhalten des Konverters |
| --- | --- |
| Formaterkennung | Moderne Dateien werden primär über S-expression root tokens erkannt: `kicad_sch`, `kicad_symbol_lib`, `kicad_pcb`, `footprint`, `kicad_wks` / `drawing_sheet`. Die Erweiterung ist ein Fallback. |
| Versionserkennung | Moderne S-expression Dateien lesen das Top-Level `(version ...)`. `.kicad_pro` wird als `kicad-project-json` gemeldet. Legacy `.sch/.lib/.dcm/.pro` verwenden Legacy-Aliasse. |
| KiCad-5/6-Grenze | KiCad 6 ist die Dateifamiliengrenze für Schaltpläne, Symbolbibliotheken und Projekte. KiCad 4/5 `.sch/.lib/.pro` und KiCad 6+ `.kicad_sch/.kicad_sym/.kicad_pro` sind unterschiedliche Syntaxfamilien. |
| PCB und Footprints | KiCad 4-10 Boards und Footprints werden als S-expressions behandelt. Relevant sind Versionsanker, Knotenmengen und Feldsyntax. |
| `.kicad_pro` | Modernes Projekt-JSON wird im Allgemeinen nicht pro Ziel-Hauptversion umgeschrieben. Für KiCad 6/7/8 werden eingebettete Worksheet-URIs `kicad-embed://...kicad_wks` in Projekt-Page-Layouts geleert; für KiCad 5/4 wird ein Legacy-`.pro` erzeugt. |
| `.kicad_wks` | Worksheets werden erkannt und können eine Version-Rewrite erhalten. Derzeit gibt es nur eine kleine Worksheet-Downgrade-Regel und keinen KiCad-4/5-Legacy-Writer. |
| `.kicad_dru` | Design-Rule-Dateien werden erkannt und nur kopiert, wenn das Ziel denselben festen `.kicad_dru`-Anker unterstützt. Ziele ohne `.kicad_dru`-Support überspringen die Datei mit einer Warnung. |

## Implementierungsmodell

| Schritt | Implementierung | Bedeutung |
| --- | --- | --- |
| Lesen | `loadDocumentImpl()` liest Text, routet `.kicad_pro` und Legacy-Dateien per Erweiterung und parst alle anderen Dateien als S-expressions. | JSON und Legacy-Text werden nicht als S-expression fehlinterpretiert. |
| Typ erkennen | `DetectKind()` bevorzugt root tokens und nutzt Erweiterungen als Fallback. | Korrekt gerootete S-expression Dateien können auch mit ungewöhnlichen Namen gelesen werden. |
| Ziel auflösen | `ResolveTargetVersion()` mappt KiCad-Aliasse auf dateitypspezifische Formatversionen. | Eine KiCad-Version hat nicht für alle Dateitypen dieselbe Formatversion. |
| Ausgabeerweiterung | `withTargetFamilyExtension()` wechselt an der KiCad-5/6-Grenze zwischen `.sch/.lib/.pro` und `.kicad_sch/.kicad_sym/.kicad_pro`. | 5/6-Konvertierung ist kein reines `(version ...)`-Edit. |
| Gleiche Version | Gleiche S-expression Quell- und Zielversion wird kopiert oder unverändert belassen. Projektkonvertierungen können für KiCad 6/7/8 dennoch inkompatible eingebettete Worksheet-URIs leeren. | Keine unnötige Formatierung, aber bekannte Projekt-Ladefehler werden vermieden. |
| Upgrade | Ältere Quelle als Ziel: `ApplyUpgradeRules()` führt konservative Syntaxnormalisierung aus. | Neue Entwurfsabsicht wird nicht erfunden. |
| Downgrade | Neuere Quelle als Ziel: `ApplyDowngradeRules()` entfernt, benennt um, flacht ab oder approximiert. | Alte KiCad-Parser sehen keine unbekannten Knoten. |

## Zielversionsanker

| Ziel | Symbolbibliothek | Schaltplan | Board | Footprint | Worksheet | Hinweise |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `4.0` | legacy `.lib` 2.3 | legacy `.sch` V2 | `4` | `4` | undefiniert | Schaltplan/Symbol nutzen Legacy-Writer. |
| `5.0` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | undefiniert | Gleicher PCB/Footprint-Anker wie `5.1`. |
| `5.1` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | undefiniert | Identisch zu `5.0`. |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | Start moderner Schaltplan-/Symbolfamilien. |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | Moderne S-expression-Erweiterungen. |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | Grenze für `generator_version`, UUID/id und PCB fields. |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | Embedded data, tables, rule areas und komplexe PCB-Objekte. |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | Höchster regulärer Zielalias im Code. |
| `10.99` | `20251024` | `20260306` | `20260603` | `20260603` | `20231118` | Entwicklungsalias; nur Board/Footprint liegen über `10.0`. |

## Konvertierung nach Dateifamilie

| Datei | Verhalten | Einschränkung |
| --- | --- | --- |
| `.kicad_pro` | Raw-JSON-Kopie für KiCad 6+; bei Projektkonvertierung nach KiCad 6/7/8 werden eingebettete Worksheet-Page-Layout-URIs geleert. Minimales Legacy-`.pro` für KiCad 5/4. | Kein vollständiger JSON-Rewrite pro Ziel-Hauptversion. |
| legacy `.pro` | Minimales `.kicad_pro` JSON für KiCad 6+. | Nur erkannte Legacy-Settings und Bibliotheksnamen bleiben erhalten. |
| `.kicad_sch` | Legacy `.sch` Writer für KiCad 5/4; S-expression-Regeln für KiCad 6+. Projektkonvertierungen fügen eine Cache-Library-Referenz hinzu. | Moderne Properties, Instances und komplexe Objekte sind in Legacy verlustbehaftet; mehrzeiliger Text wird als escaped `\n` auf einer Zeile ausgegeben. |
| legacy `.sch` | Konvertierung zu `.kicad_sch` für KiCad 6+; Header-Rewrite für KiCad 5/4. | Legacy non-wire drawing ist nicht vollständig gemappt. |
| `.kicad_sym` | Ausgabe von `.lib` und `.dcm` für KiCad 5/4; Projektkonvertierungen kopieren die erzeugte `.lib` zusätzlich nach `<project>-cache.lib`. | Moderne Symbol-Properties, Grafik und verschachtelte Symbole werden approximiert; Legacy-`DEF`-Referenzen verwenden Präfixe wie `U`, `BT` oder `#PWR`. |
| legacy `.lib/.dcm` | Erzeugt `.kicad_sym` für KiCad 6+. | `.dcm` allein wird ein Symbol-Skelett; Dokumentationsmetadata ist unvollständig. |
| `.kicad_pcb/.kicad_mod` | Bleibt S-expression; version, nodes und fields werden umgeschrieben. | Nicht unterstützte Geometry/Electrical/Manufacturing/Cache-Felder werden entfernt oder approximiert. |
| `.kicad_wks` | Version-Rewrite und begrenzte Worksheet-Regel für KiCad 6+. | Kein KiCad-4/5-Legacy-Worksheet-Writer. |
| `.kicad_dru` | Wird nur kopiert, wenn das Ziel denselben festen Design-Rule-Anker unterstützt. | Keine Design-Rule-Formatkonvertierung; nicht unterstützte Ziele überspringen die Datei mit Warnung. |

## Downgrade-Prinzipien

| Fall | Implementierungswahl | Beispiele |
| --- | --- | --- |
| Zielparser kann Knoten nicht lesen | Knoten/Feld entfernen und warning ausgeben. | `embedded_files`, `variants`, `barcodes`, `net_chains`, native ellipse. |
| Alte Darstellung existiert | Umbenennen, Abflachen oder Legacy-Feld. | `directive_label -> netclass_flag`, `stroke -> width`, `uuid/tstamp/id`. |
| Ziel hat schwächere Geometrie | In einfachere Geometrie konvertieren. | PCB rectangle zu lines, track arcs zu segments, custom pads zu rectangular pads. |
| Altes Property-Layout | Properties verschieben, IDs hinzufügen oder entfernen. | property hide Position, standard property id. |
| Quelle enthält keine neue Semantik | Keine neuen Feature-Objekte erzeugen. | Keine automatischen padstacks, variants, component classes oder barcodes. |

## Aktuelle Kompatibilitätskorrekturen

| Bereich | Verhalten |
| --- | --- |
| KiCad 6/7/8 Projekt-Worksheets | Projekt-Page-Layout-Referenzen auf `kicad-embed://...kicad_wks` werden geleert, damit KiCad 6/7/8 nicht versucht, nicht unterstützte eingebettete Worksheet-Pfade zu laden. |
| Eingebettete 3D-Modelle in board/footprint | Wenn zstd einkompiliert ist, werden eingebettete `type model` Ressourcen nach `3D/` extrahiert und `kicad-embed://...` model URIs zu `${KIPRJMOD}/3D/...` umgeschrieben. Ohne zstd können die Modelldaten nicht dekomprimiert werden; unsupported embedded model references werden mit Warnungen entfernt. |
| KiCad 6 Schaltpläne | Root-`uuid`, platzierte Symbol-Pin-UUID-Blöcke, nicht unterstützte root-level Drawing-Primitives (`rectangle`, `circle`, `arc`, `polyline`, `bezier`) und inkompatible Fill-Colors werden entfernt oder auf kompatible Werte zurückgesetzt. |
| KiCad 4/5 Legacy-Schaltpläne | Mehrzeiliger Text wird als escaped `\n` in einer Textzeile geschrieben; Projektkonvertierungen erzeugen `<project>-cache.lib` und ergänzen `LIBS:<project>-cache`. |
| KiCad 4/5 Legacy-Symbolbibliotheken | `DEF`-Referenzfelder werden als Präfixe geschrieben, nicht als Instanzreferenzen wie `U1`. |
| PCB/Footprint Upgrade | Footprint-`attr dnp` bleibt als `attr`-Atom erhalten; es wird nicht zu `(dnp yes/no)` erweitert. |

## Projektverzeichnis-Konvertierung

| Verarbeitung | Implementierung |
| --- | --- |
| Eingabe | Verzeichnis oder `.kicad_pro` wird als project tree behandelt. |
| Kopierte Dateien | KiCad documents, legacy documents, library tables, `.kicad_prl`, 3D model files. |
| Übersprungene Verzeichnisse | VCS, history, backup, archive, gerber/fabrication/output/plot/BOM/assembly/vendor output. |
| Erweiterungen | KiCad 5/4 mappt `.kicad_sch/.kicad_sym/.kicad_pro` nach `.sch/.lib/.pro`; KiCad 6+ umgekehrt. |
| `.dcm` | Bei KiCad 6+ wird eine `.dcm` mit passender `.lib` nicht separat konvertiert. |
| `.kicad_dru` | Wird für Ziele ohne `.kicad_dru`-Support übersprungen; bei gleichem Design-Rule-Anker wird sie kopiert. |
| Project worksheet refs | Für KiCad 6/7/8 werden eingebettete Worksheet-Page-Layout-Referenzen in `.kicad_pro` geleert. |
| Embedded 3D models | Mit zstd-Support werden eingebettete 3D-Modelle nach `3D/` extrahiert und als `${KIPRJMOD}/3D/...` referenziert; ohne zstd werden nicht extrahierbare embedded model references gewarnt und entfernt. |
| Library tables | `sym-lib-table` / `fp-lib-table` werden für die Zielfamilie normalisiert. |
| Schematic support | Für KiCad 6+ werden project-local Symbole in `lib_symbols` eingebettet und hierarchy instances neu aufgebaut. |
| Legacy schematic cache | Für KiCad 5/4 wird `Library.lib` nach `<project>-cache.lib` kopiert und jede erzeugte `.sch` erhält `LIBS:<project>-cache`. |

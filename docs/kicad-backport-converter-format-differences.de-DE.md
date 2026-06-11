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
| `.kicad_pro` | Modernes Projekt-JSON wird nicht pro Ziel-Hauptversion umgeschrieben. Für KiCad 6+ wird es meist unverändert kopiert; für KiCad 5/4 wird ein minimales `.pro` erzeugt. |
| `.kicad_wks` | Worksheets werden erkannt und können eine Version-Rewrite erhalten. Derzeit gibt es nur eine kleine Worksheet-Downgrade-Regel und keinen KiCad-4/5-Legacy-Writer. |
| `.kicad_dru` | Der Code kann es erkennen und verwendet den festen Anker `20200610`, es ist aber nicht Teil der primären benutzer sichtbaren Formatmatrix. |

## Implementierungsmodell

| Schritt | Implementierung | Bedeutung |
| --- | --- | --- |
| Lesen | `loadDocumentImpl()` liest Text, routet `.kicad_pro` und Legacy-Dateien per Erweiterung und parst alle anderen Dateien als S-expressions. | JSON und Legacy-Text werden nicht als S-expression fehlinterpretiert. |
| Typ erkennen | `DetectKind()` bevorzugt root tokens und nutzt Erweiterungen als Fallback. | Korrekt gerootete S-expression Dateien können auch mit ungewöhnlichen Namen gelesen werden. |
| Ziel auflösen | `ResolveTargetVersion()` mappt KiCad-Aliasse auf dateitypspezifische Formatversionen. | Eine KiCad-Version hat nicht für alle Dateitypen dieselbe Formatversion. |
| Ausgabeerweiterung | `withTargetFamilyExtension()` wechselt an der KiCad-5/6-Grenze zwischen `.sch/.lib/.pro` und `.kicad_sch/.kicad_sym/.kicad_pro`. | 5/6-Konvertierung ist kein reines `(version ...)`-Edit. |
| Gleiche Version | Gleiche S-expression Quell- und Zielversion wird kopiert oder unverändert belassen. | Keine unnötige Formatierung. |
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
| `.kicad_pro` | Raw-JSON-Kopie für KiCad 6+, minimales `.pro` für KiCad 5/4. | Kein JSON-Rewrite pro Ziel-Hauptversion. |
| legacy `.pro` | Minimales `.kicad_pro` JSON für KiCad 6+. | Nur erkannte Legacy-Settings und Bibliotheksnamen bleiben erhalten. |
| `.kicad_sch` | Legacy `.sch` Writer für KiCad 5/4; S-expression-Regeln für KiCad 6+. | Moderne Properties, Instances und komplexe Objekte sind in Legacy verlustbehaftet. |
| legacy `.sch` | Konvertierung zu `.kicad_sch` für KiCad 6+; Header-Rewrite für KiCad 5/4. | Legacy non-wire drawing ist nicht vollständig gemappt. |
| `.kicad_sym` | Ausgabe von `.lib` und `.dcm` für KiCad 5/4. | Moderne Symbol-Properties, Grafik und verschachtelte Symbole werden approximiert. |
| legacy `.lib/.dcm` | Erzeugt `.kicad_sym` für KiCad 6+. | `.dcm` allein wird ein Symbol-Skelett; Dokumentationsmetadata ist unvollständig. |
| `.kicad_pcb/.kicad_mod` | Bleibt S-expression; version, nodes und fields werden umgeschrieben. | Nicht unterstützte Geometry/Electrical/Manufacturing/Cache-Felder werden entfernt oder approximiert. |
| `.kicad_wks` | Version-Rewrite und begrenzte Worksheet-Regel für KiCad 6+. | Kein KiCad-4/5-Legacy-Worksheet-Writer. |

## Downgrade-Prinzipien

| Fall | Implementierungswahl | Beispiele |
| --- | --- | --- |
| Zielparser kann Knoten nicht lesen | Knoten/Feld entfernen und warning ausgeben. | `embedded_files`, `variants`, `barcodes`, `net_chains`, native ellipse. |
| Alte Darstellung existiert | Umbenennen, Abflachen oder Legacy-Feld. | `directive_label -> netclass_flag`, `stroke -> width`, `uuid/tstamp/id`. |
| Ziel hat schwächere Geometrie | In einfachere Geometrie konvertieren. | PCB rectangle zu lines, track arcs zu segments, custom pads zu rectangular pads. |
| Altes Property-Layout | Properties verschieben, IDs hinzufügen oder entfernen. | property hide Position, standard property id. |
| Quelle enthält keine neue Semantik | Keine neuen Feature-Objekte erzeugen. | Keine automatischen padstacks, variants, component classes oder barcodes. |

## Projektverzeichnis-Konvertierung

| Verarbeitung | Implementierung |
| --- | --- |
| Eingabe | Verzeichnis oder `.kicad_pro` wird als project tree behandelt. |
| Kopierte Dateien | KiCad documents, legacy documents, library tables, `.kicad_prl`, 3D model files. |
| Übersprungene Verzeichnisse | VCS, history, backup, archive, gerber/fabrication/output/plot/BOM/assembly/vendor output. |
| Erweiterungen | KiCad 5/4 mappt `.kicad_sch/.kicad_sym/.kicad_pro` nach `.sch/.lib/.pro`; KiCad 6+ umgekehrt. |
| `.dcm` | Bei KiCad 6+ wird eine `.dcm` mit passender `.lib` nicht separat konvertiert. |
| Library tables | `sym-lib-table` / `fp-lib-table` werden für die Zielfamilie normalisiert. |
| Schematic support | Für KiCad 6+ werden project-local Symbole in `lib_symbols` eingebettet und hierarchy instances neu aufgebaut. |


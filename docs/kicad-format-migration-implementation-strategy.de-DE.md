# Implementierungsstrategie für KiCad-Formatmigrationen

Dieser Text ist strukturell mit der englischen Referenz synchronisiert; technische KiCad-Begriffe, Tokens und Dateinamen bleiben absichtlich unverändert.

Dieses Dokument leitet aus `kicad-file-format-version-differences.md` konkrete
Implementierungshinweise für Migrationen zwischen benachbarten KiCad-Hauptversionen ab.

Die wichtigste Grenze liegt zwischen KiCad 5 und KiCad 6. KiCad 4 und 5 verwenden für
Schaltpläne und Symbolbibliotheken noch Legacy-Dateien (`.sch`, `.lib`, `.dcm`), während
KiCad 6 und neuer S-expression-Dateien (`.kicad_sch`, `.kicad_sym`) verwenden. PCB- und
Footprint-Dateien sind in diesen Versionen bereits S-expression-Dateien, benötigen aber
weiterhin explizite Rewrite-Regeln für Features und Versionsnummern.

Zuletzt aktualisiert: 2026-06-04.

## Umfang

Dies ist eine Implementierungs-Roadmap, keine Zusage für Release-Support. Sie beschreibt:

- Vorwärtsmigrationen wie KiCad 6 nach 7 und KiCad 5 nach 6.
- Rückwärtsmigrationen wie KiCad 7 nach 6, KiCad 6 nach 5 und KiCad 5 nach 4.
- Erweiterungspunkte für spätere benachbarte Versionen wie 7 nach 8, 8 nach 9,
  9 nach 10 und 10 zum aktuellen Entwicklungszweig.

Der aktuelle C++-Konverter ist hauptsächlich eine Downgrade-Engine. Wenn die
Quellformatversion älter als das angeforderte Ziel ist, kopiert er die Datei derzeit
unverändert, statt ein Upgrade auszuführen. Upgrade-Support sollte daher als separater
Migrationspfad ergänzt werden und nicht durch eine Abschwächung der Downgrade-Regeln.

## Zielversionsmatrix

| KiCad-Ziel | Symbolbibliothek | Schaltplan | PCB / Footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: |
| 4.0 | Legacy `.lib` 2.3 | Legacy `.sch` V2 | `4` | Legacy drawing sheet | Keine |
| 5.0 / 5.1 | Legacy `.lib` 2.4 | Legacy `.sch` V4 | `20171130` | Legacy drawing sheet | Keine |
| 6.0 | `20211014` | `20211123` | `20211014` | `20210606` | `20200610` |
| 7.0 | `20220914` | `20230121` | `20221018` | `20220228` | `20200610` |
| 8.0 | `20231120` | `20231120` | `20240108` | `20231118` | `20200610` |
| 9.0 | `20241209` | `20250114` | `20241229` | `20231118` | `20200610` |
| 10.0 | `20251024` | `20260306` | `20260206` | `20231118` | `20200610` |

## Aufbau der Migrations-Engine

Migrationen sollten als Pipeline mit File-Family-Adaptern implementiert werden:

1. Dokumenttyp über root S-expression head oder Legacy-Erweiterung/Header erkennen.
2. In einen veränderbaren Dokumentbaum oder ein typed intermediate model parsen.
3. Route von Quell-Family/Version zu Ziel-Family/Version auswählen.
4. Geordnete Migrationsschritte anwenden. Jeder Schritt sollte durch Dokumenttyp,
   Quellversion und Zielversion geschützt sein.
5. Zielfamilie mit Zielversion und Zielerweiterung schreiben.
6. Für jede verlustbehaftete Entfernung, jeden Fallback-Default und jede nicht
   implementierte semantische Konvertierung Warnungen ausgeben.

Migration darf nicht nur als Änderung der obersten Versionsnummer implementiert werden.
Jede Versionsgrenze kann Tokens, Layout-Strukturen, Objektattribute oder ganze
Dateifamilien einführen oder entfernen.

## Regeln für Dateifamilien-Grenzen

### KiCad 4/5 Legacy-Familie

KiCad 4 und 5 benötigen Legacy-Writer für Schaltplan- und Symbolbibliotheksausgaben:

- Schaltplan: `.sch`, mit `EESchema Schematic File Version 2` für KiCad 4 und
  `Version 4` für KiCad 5.
- Symbolbibliothek: `.lib` plus optional `.dcm`, typischerweise
  `EESchema-LIBRARY Version 2.3` für KiCad 4 und `Version 2.4` für KiCad 5.
- Projekt: Legacy `.pro`.

V6+-Schaltplan- und Symboldaten können nicht durch Entfernen einiger S-expression-Nodes
nach KiCad 4/5 downgradet werden. Die Implementierung benötigt einen echten
Legacy-Serializer oder eine klare Warnung, dass die Konvertierung nicht unterstützt wird.

### KiCad 6+ S-Expression-Familie

KiCad 6 und neuer verwenden S-expression-Dateien für Schaltplan, Symbolbibliothek, PCB,
Footprint, Worksheet und Design rules. Die meisten benachbarten Migrationen können als
versionsgesteuerte Tree-Rewrites behandelt werden:

- Feature-Nodes je nach Zielversion hinzufügen oder entfernen.
- Felder umbenennen, wenn sich das Format geändert hat.
- Boolean-list-Formate in Legacy-presence-atoms konvertieren oder umgekehrt.
- UUID-, ID-, font-, text- und property-Strukturen für das Ziel normalisieren.

## Strategie für Vorwärtsmigrationen

Vorwärtsmigration sollte die Quellsemantik erhalten und sichere Ziel-Defaults erzeugen.
Sie sollte keine neue Designabsicht erfinden.

Empfohlenes Verhalten:

- Wenn Quelle und Ziel in derselben Dateifamilie liegen, parsen, bekannte
  Kompatibilitäts-Rewrites anwenden und im neueren Zielformat schreiben.
- Wenn ein Formatfeld in der Quelle fehlt, es weglassen, wenn das neuere Format dies
  erlaubt; andernfalls KiCad-ähnliche Defaults schreiben.
- Wenn die Migration von KiCad 4/5 Legacy nach KiCad 6+ S-expression wechselt,
  dedizierte Importer für `.sch`, `.lib`, `.dcm` und `.pro` verwenden.
- Für schwierige Legacy-Imports KiCad selbst als Referenz nutzen: altes Projekt in der
  passenden KiCad-Version oder in KiCad 6+ laden, speichern und die erzeugte Struktur
  mit der Konverterausgabe vergleichen.

### Upgrade von KiCad 6 nach KiCad 7

Dies ist ein S-expression-Upgrade innerhalb derselben Familie. Es kann als strukturierter
Rewrite plus Zielversionsupdate implementiert werden.

Wichtige Aktionen:

- Zielversionen auf Symbol `20220914`, Schaltplan `20230121`, Board / Footprint
  `20221018` und Worksheet `20220228` setzen.
- Bestehende V6-Schaltplan-, Symbol-, Board-, Footprint-, Worksheet- und DRC-Objekte
  erhalten.
- Defaults nur dort ergänzen, wo KiCad 7 einen Wert verlangt, den KiCad 6 nicht schrieb.
- Alte kompatible Felder bei Bedarf auf neue Schreibweisen abbilden, etwa
  `netclass_flag` zu `directive_label`.
- Text-box-Geometrie auf KiCad-7-`at` / `size` normalisieren, falls eine ältere
  `start` / `end`-Darstellung vorkommt.
- V6-Simulationsinformationen gültig halten, aber keine vollständigen KiCad-7-
  simulation model data erzeugen, wenn die Felder nicht ausreichen.
- DNP-bezogene Defaults nur erhalten oder erzeugen, wenn der Zielobjekttyp sie unterstützt.
- Bei PCB und Footprints V6-Objekte erhalten und, wenn genug Daten vorhanden sind,
  V7-kompatible Formen für stroke formatting, footprint private layers, dimensions,
  teardrop keywords, net ties und pad/via zone-layer connections aktivieren.
- Bei Worksheets die V7-Worksheet-Version schreiben und font data nur erhalten, wenn sie
  KiCad-7-kompatibel dargestellt werden können.

Validierungs-Fixtures:

- V6-Schaltplan mit Labels, Feldern, hierarchischen Sheets und simulation fields.
- V6-Board mit Vias, Pads, Zonen, Dimensions und Footprint-Text.
- V6-Symbolbibliothek mit Properties und einfachen Primitives.

### Upgrade von KiCad 5 nach KiCad 6

Dies ist die wichtigste Vorwärtsgrenze, weil Schaltplan- und Symbol-Dateien die
Dateifamilie wechseln.

Benötigte Adapter:

- `.pro` nach `.kicad_pro` JSON-Projektmigration.
- Legacy `.sch` V4 nach `.kicad_sch` `20211123`.
- Legacy `.lib` / `.dcm` 2.4 nach `.kicad_sym` `20211014`.
- `.kicad_pcb` / `.kicad_mod` `20171130` nach `20211014`.
- Legacy drawing sheet nach `.kicad_wks` `20210606`, falls Worksheet-Konvertierung
  unterstützt wird.

Wichtige Aktionen:

- Legacy-Schaltplanrecords in ein typed model parsen, bevor KiCad-6-S-expressions
  geschrieben werden. Keine zeilenorientierte Textersetzung für `.sch` verwenden.
- Legacy-Schematic-Symbole auf KiCad-6-symbol instances mit UUIDs, properties, paths,
  sheet instances und library identifiers abbilden.
- Stabile UUIDs erzeugen, wo KiCad 6 sie verlangt. Für reproduzierbare Ausgaben
  deterministische UUIDs aus Quellpfaden und Objektidentität ableiten.
- Legacy library aliases, fields, drawing primitives, pins und documentation records in
  `.kicad_sym`-Symbole konvertieren.
- `.dcm` descriptions, keywords und documentation links nach Möglichkeit als KiCad-6-
  Symbolmetadaten erhalten.
- Legacy project settings nur dann nach `.kicad_pro` und `.kicad_prl` konvertieren,
  wenn klare Entsprechungen existieren. Für ignorierte UI- oder Cache-Settings warnen.
- PCB-Version auf `20211014` anheben und KiCad-5-Boardfeatures erhalten, einschließlich
  custom pads, multi-layer keepouts, 3D model offsets und footprint text lock state.

Erwartete Verlustbereiche:

- Manche Legacy-Projekteinstellungen haben möglicherweise kein KiCad-6-JSON-Äquivalent.
- Legacy library rescue/cache behavior kann explizites symbol remapping benötigen.
- V5-Schaltplankonstrukte, die von alten Library-Lookup-Regeln abhängen, können Warnungen
  oder Symbolbibliotheks-Sidecar-Daten erfordern.

## Strategie für Rückwärtsmigrationen

Rückwärtsmigration sollte nicht unterstützte Konstrukte entfernen oder umschreiben und den
Verlust melden. Die Zieldatei sollte von der angeforderten KiCad-Version ladbar sein, auch
wenn neuere Semantik nicht vollständig erhalten werden kann.

Empfohlenes Verhalten:

- Downgrade-Regeln von neu nach alt anwenden, damit spätere Features entfernt werden,
  bevor ältere Kompatibilitäts-Rewrites laufen.
- Warnungen konkret halten: entferntes Feature nennen, betroffene Nodes zählen und
  Einführungsversion angeben.
- Bei S-expression-Downgrades innerhalb derselben Familie Objektidentität erhalten, wo das
  Ziel sie unterstützt.
- Für V6+ nach V5/V4 bei Schaltplan oder Symbol einen dedizierten Legacy-Writer verwenden
  und nicht unterstützte V6+-Objekte als verlustbehaftet behandeln.

### Downgrade von KiCad 7 nach KiCad 6

Dies ist ein S-expression-Downgrade innerhalb derselben Familie und sollte als gezielte
Entfernung plus Kompatibilitäts-Rewrite implementiert werden.

Zielversionen:

- Symbolbibliothek: `20211014`.
- Schaltplan: `20211123`.
- Board / Footprint: `20211014`.
- Worksheet: `20210606`.
- Design rules: `20200610`.

Wichtige Downgrade-Regeln:

- Symbol class flags, symbol fonts, text boxes, text colors, unit display names und
  KiCad-7-only property-ID behavior entfernen.
- Schematic graphic primitives entfernen, die nach V6 eingeführt wurden und kein
  V6-Äquivalent haben, einschließlich text boxes und neuerer label fields.
- `directive_label` zurück in die V6-kompatible `netclass_flag`-Form schreiben, wenn
  eine treue Abbildung existiert; andernfalls warnen.
- Dash-dot-dot line style, text hyperlinks, field name visibility, do-not-autoplace
  options, DNP support und V7 simulation model fields entfernen oder downgraden.
- Symbol- und sheet instance data in die von KiCad 6 erwartete Struktur verschieben oder
  vereinfachen.
- PCB text boxes, image objects, first-class net ties, V7 teardrop keywords,
  footprint private-layer changes, die V6 nicht parsen kann, sowie pad/via
  zone-layer-connection additions entfernen.
- V7 stroke-, font-, boolean-, lock- und hide-Formate zurück in V6-kompatible Formen
  konvertieren.
- Worksheet font support entfernen, der in `20220228` eingeführt wurde.

Erwartete Verlustbereiche:

- Text boxes, images, net ties, DNP flags, hyperlinks und modern simulation model data
  haben möglicherweise keine treuen V6-Äquivalente.
- Einige V7-PCB-dimensions und teardrop settings sollten entfernt oder abgeflacht werden.

### Downgrade von KiCad 6 nach KiCad 5

Dies ist ein dateifamilienübergreifender Downgrade für Schaltplan- und Symboldateien.

Benötigte Writer:

- `.kicad_sch` `20211123` nach Legacy `.sch` V4.
- `.kicad_sym` `20211014` nach Legacy `.lib` 2.4 plus `.dcm`.
- `.kicad_pro` JSON nach Legacy `.pro`.
- `.kicad_pcb` / `.kicad_mod` `20211014` nach `20171130`.

Wichtige Downgrade-Regeln:

- KiCad-6 schematic symbols, wires, buses, labels, junctions, sheets, fields und page
  settings in das Legacy-`.sch`-Recordformat serialisieren.
- KiCad-6 UUIDs und paths in Legacy timestamps oder deterministische identifiers
  umwandeln, wo das Ziel dies verlangt.
- KiCad-6 symbol metadata in `.lib` symbol definitions und `.dcm` documentation records
  aufteilen.
- KiCad-6-only Schaltplan- und Symbolstrukturen entfernen, die Legacy-Dateien nicht direkt
  darstellen können. Für jede Entfernung warnen.
- `.kicad_pro` settings nur nach `.pro` konvertieren, wenn ein bekanntes Legacy-Äquivalent
  existiert. JSON-only settings ignorieren oder melden.
- Board- und Footprint-Versionen auf `20171130` downgraden.
- V6-Board/Footprint-Features entfernen, die KiCad 5 nicht parsen kann, einschließlich
  V6+ UUID-only structures, neuerem zone behavior, rule-file-Annahmen und allen Objekten,
  die nach dem V5-PCB-Format eingeführt wurden.
- KiCad-5-kompatible PCB-Features erhalten: custom pads, multi-layer keepouts,
  3D model offset in mm mit dem `offset`-Parameter und locked footprint text.

Erwartete Verlustbereiche:

- KiCad-6-Schaltplan- und Symbol-S-expressions lassen sich nicht immer auf Legacy-Record-
  Felder abbilden.
- Project-JSON-Settings, moderne UUID-Identität und manche Library-Linkage-Details können
  reduziert oder verworfen werden.
- `.kicad_dru`-Dateien haben kein eigenständiges KiCad-5-Äquivalent.

### Downgrade von KiCad 5 nach KiCad 4

Dies ist überwiegend ein Downgrade innerhalb der Legacy-Familie, wobei PCB weiterhin
S-expression-Feature-Entfernung benötigt.

Zielformate:

- Schaltplan: `.sch` V2.
- Symbolbibliothek: `.lib` 2.3 plus `.dcm`.
- PCB / Footprint: Version `4`.
- Projekt: Legacy `.pro`.

Wichtige Downgrade-Regeln:

- Schaltplanheader von `EESchema Schematic File Version 4` nach `Version 2` umschreiben.
- KiCad-5-schematic records entfernen oder umschreiben, die KiCad 4 nicht parsen kann.
- Symbolbibliotheksheader von 2.4-style output auf 2.3-style output downgraden.
- Symbol fields oder attributes entfernen, die in KiCad-4-Bibliotheken nicht existieren.
- Board- und Footprint-Dateien von `20171130` auf Version `4` downgraden.
- KiCad-5-PCB-Features entfernen, die nach KiCad 4 eingeführt wurden, einschließlich
  differential pair settings per net class, long pad names, custom pad shape details,
  multi-layer keepouts, 3D model offset changes, die KiCad 4 nicht interpretieren kann,
  und locked/unlocked footprint text.
- 3D model offsets in die KiCad-4-Darstellung konvertieren, wenn eine reversible
  Einheitenkonvertierung bekannt ist; andernfalls warnen und nicht unterstützte
  offset-Felder entfernen.

Erwartete Verlustbereiche:

- Custom pads und multi-layer keepouts müssen eventuell vereinfacht oder entfernt werden.
- Long pad names müssen eventuell gekürzt oder deterministisch umbenannt werden.
- Einige net-class- und footprint text-lock-Metadaten können nicht erhalten werden.

## Spätere benachbarte Downgrade-Muster

Dasselbe Downgrade-Framework sollte spätere benachbarte Versionen abdecken:

| Route | Hauptfokus der Implementierung |
| --- | --- |
| KiCad 8 nach 7 | V8 generator cleanup fields, PCB fields, generated objects, UUID normalization, custom pad spoke templates, modern teardrops, images, rule-area changes, simulation/exclude flags und worksheet embedded images entfernen. |
| KiCad 9 nach 8 | Embedded files, tables, rule areas, component classes, complex padstacks, via stacks, arbitrary user layers, tenting, multiple netclass assignments und netclass color highlighting entfernen. |
| KiCad 10 nach 9 | Variants, jumper pads, barcodes, via protection, zone hatch offsets, backdrill, split via types, stopped-netcode assumptions, rounded rectangles, stacked pins, PCB points und property-formatting updates entfernen. |
| Aktueller Entwicklungsstand nach KiCad 10 | Post-10.0-Entwicklungsfeatures entfernen: extruded 3D body metadata, native ellipses and ellipse arcs, dielectric frequency-dependent stackup fields, net chains, copper thieving, pad `sim_electrical_type` und PCB table-cell `knockout`. |

Vorwärtsmigrationen für dieselben Routen sind meist weniger verlustbehaftet als
Downgrades, benötigen aber weiterhin Kompatibilitäts-Rewrites und Ziel-Defaults. KiCad 7
nach 8 sollte beispielsweise V8-normalized `generator_version` handling nur dort einführen,
wo das Zielformat es erwartet, und KiCad 8 nach 9 sollte keine embedded files,
component classes oder padstacks erfinden, wenn sie nicht bereits in der Quellsemantik
vorhanden sind.

## Teststrategie

Für jede benachbarte Route und jeden Dokumenttyp sollte ein Fixture-Set erstellt werden:

- Projektdateien: `.pro` und `.kicad_pro`.
- Schaltplandateien: Legacy `.sch` und `.kicad_sch`.
- Symbolbibliotheken: `.lib`, `.dcm` und `.kicad_sym`.
- PCB-Dateien: `.kicad_pcb`.
- Footprints: `.kicad_mod`.
- Worksheets: Legacy drawing sheet und `.kicad_wks`.
- Design rules: `.kicad_dru` nur für KiCad 6+.

Jeder Test sollte prüfen:

- Quelltyp und Quellversion werden korrekt erkannt.
- Ziel-Dateifamilie, Erweiterung und Version sind korrekt.
- Für die Zielversion verbotene Tokens fehlen.
- Bekannte kompatible Strukturen bleiben erhalten.
- Verlustbehaftete Änderungen erzeugen Warnungen.
- Ein erneutes Ausführen derselben Migration ist stabil und ändert die Datei nicht weiter.

Für V5/V6- und V6/V5-Routen sollten nach Möglichkeit von KiCad selbst erzeugte
Golden-File-Tests ergänzt werden. Diese Routen überschreiten eine Dateifamiliengrenze und
sind besonders anfällig für semantische Abweichungen.

## Implementierungsreihenfolge

Empfohlene Reihenfolge:

1. Same-family S-expression-Downgrades von V7 nach V6 abschließen.
2. V5-PCB-/Footprint-Downgrade nach V4 hinzufügen, weil dies in der PCB-S-expression-
   Familie bleibt.
3. Legacy-Schaltplan- und Symbolparser für das Upgrade von V5 nach V6 implementieren.
4. Legacy-Schaltplan- und Symbolwriter für das Downgrade von V6 nach V5 implementieren.
5. Legacy-Schaltplan- und Symboldowngrade von V5 nach V4 hinzufügen.
6. Vorwärtsmigrationen innerhalb derselben Familie wie V6 nach V7 ergänzen.
7. Dasselbe Framework auf V8-, V9-, V10- und aktuelle Entwicklungsrouten erweitern.

Diese Reihenfolge liefert früh nützliche Downgrade-Abdeckung und isoliert gleichzeitig die
deutlich schwierigere Legacy-Schaltplan- und Symbolkonvertierung.

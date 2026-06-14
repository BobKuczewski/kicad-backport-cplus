# Differenze di formato e comportamento di conversione di KiCad Backport

Questo documento descrive le differenze di formato KiCad realmente gestite dall’implementazione attuale di `kicad-backport`: famiglie di file, ancore di versione, dispatch di conversione, regole di riscrittura e percorsi con perdita. La fonte normativa per i formati KiCad rimane la documentazione per sviluppatori KiCad.

- https://dev-docs.kicad.org/en/file-formats/index.html

## Lettura rapida per sviluppatori KiCad

| Tema | Comportamento del convertitore |
| --- | --- |
| Rilevamento formato | I file moderni sono identificati prima dai root token S-expression: `kicad_sch`, `kicad_symbol_lib`, `kicad_pcb`, `footprint`, `kicad_wks` / `drawing_sheet`. L’estensione è un fallback. |
| Rilevamento versione | I file S-expression moderni leggono il campo top-level `(version ...)`. `.kicad_pro` è riportato come `kicad-project-json`. I legacy `.sch/.lib/.dcm/.pro` usano alias legacy. |
| Confine KiCad 5/6 | KiCad 6 è il confine di famiglia per schemi, librerie simboli e progetti. KiCad 4/5 `.sch/.lib/.pro` e KiCad 6+ `.kicad_sch/.kicad_sym/.kicad_pro` non hanno la stessa sintassi. |
| PCB e footprint | Board e footprint KiCad 4-10 sono trattati come S-expression. Le differenze rilevanti sono version anchor, insiemi di nodi e sintassi dei campi. |
| `.kicad_pro` | Il JSON progetto moderno in genere non viene riscritto per major version target. Per KiCad 6/7/8 gli URI worksheet embedded `kicad-embed://...kicad_wks` nei page layout di progetto sono svuotati; per KiCad 5/4 viene generato un `.pro` legacy. |
| `.kicad_wks` | I worksheet sono rilevati e possono avere la version riscritta. Esiste solo una piccola regola worksheet downgrade e nessun writer legacy KiCad 4/5. |
| `.kicad_dru` | I file di regole di progetto sono rilevati e copiati solo se il target supporta lo stesso anchor fisso `.kicad_dru`. I target senza supporto `.kicad_dru` saltano il file con un warning. |

## Modello di implementazione

| Fase | Implementazione | Significato |
| --- | --- | --- |
| Lettura | `loadDocumentImpl()` legge testo, instrada `.kicad_pro` e file legacy per estensione e parse gli altri come S-expression. | Evita di trattare JSON o testo legacy come S-expression. |
| Tipo | `DetectKind()` preferisce root token e usa l’estensione come fallback. | Un S-expression con root corretto può essere gestito anche con nome insolito. |
| Target | `ResolveTargetVersion()` mappa ogni alias KiCad a una versione per tipo di file. | Una release KiCad non usa una sola versione formato per tutti i file. |
| Estensione output | `withTargetFamilyExtension()` cambia `.sch/.lib/.pro` e `.kicad_sch/.kicad_sym/.kicad_pro` al confine KiCad 5/6. | La conversione 5/6 non è una semplice modifica di `(version ...)`. |
| Stessa versione | Se source e target S-expression hanno la stessa versione, il file è copiato o lasciato invariato. Le conversioni progetto possono comunque svuotare URI worksheet embedded incompatibili per KiCad 6/7/8. | Evita riscritture inutili e corregge errori noti di caricamento progetto. |
| Upgrade | Se la source è più vecchia del target, `ApplyUpgradeRules()` normalizza la sintassi in modo conservativo. | Non inventa nuova intenzione progettuale. |
| Downgrade | Se la source è più nuova del target, `ApplyDowngradeRules()` elimina, rinomina, appiattisce o approssima. | I vecchi parser KiCad non incontrano nodi sconosciuti. |

## Version anchor target

| Target | Symbol library | Schematic | Board | Footprint | Worksheet | Note |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `4.0` | legacy `.lib` 2.3 | legacy `.sch` V2 | `4` | `4` | non definito | Schematic/symbol usano writer legacy. |
| `5.0` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | non definito | Stesso anchor PCB/footprint di `5.1`. |
| `5.1` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | non definito | Uguale a `5.0`. |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | Punto iniziale delle famiglie schematic/symbol moderne. |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | Estensioni S-expression moderne. |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | Confine `generator_version`, UUID/id e PCB fields. |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | Embedded data, tables, rule areas e oggetti PCB complessi. |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | Massimo alias target regolare supportato dal codice. |
| `10.99` | `20251024` | `20260306` | `20260603` | `20260603` | `20231118` | Alias di sviluppo; solo board/footprint avanzano oltre `10.0`. |

## Conversione per famiglia

| File | Comportamento | Limite |
| --- | --- | --- |
| `.kicad_pro` | Copia JSON raw per KiCad 6+; in conversione progetto verso KiCad 6/7/8 svuota gli URI page-layout worksheet embedded. `.pro` legacy minimo per KiCad 5/4. | Nessuna riscrittura JSON completa per major version target. |
| legacy `.pro` | Genera `.kicad_pro` JSON minimo per KiCad 6+. | Conserva solo setting legacy e nomi libreria riconosciuti. |
| `.kicad_sch` | Writer legacy `.sch` per KiCad 5/4; regole S-expression per KiCad 6+. Le conversioni progetto aggiungono un riferimento alla cache library. | Proprietà, istanze e oggetti moderni complessi sono con perdita in legacy; il testo multilinea è emesso su una riga con `\n` escaped. |
| legacy `.sch` | Converte a `.kicad_sch` per KiCad 6+; riscrive header per KiCad 5/4. | I disegni legacy non-wire non sono mappati completamente. |
| `.kicad_sym` | Scrive `.lib` e `.dcm` per KiCad 5/4; le conversioni progetto copiano anche la `.lib` generata in `<project>-cache.lib`. | Proprietà simbolo, grafica e simboli annidati moderni sono approssimati; i riferimenti legacy `DEF` usano prefissi come `U`, `BT` o `#PWR`. |
| legacy `.lib/.dcm` | Genera `.kicad_sym` per KiCad 6+. | `.dcm` da solo produce uno skeleton simboli; metadata documentale incompleta. |
| `.kicad_pcb/.kicad_mod` | Rimane S-expression; riscrive version, nodes e fields. | Campi geometry/electrical/manufacturing/cache non supportati sono rimossi o approssimati. |
| `.kicad_wks` | Version rewrite e regola worksheet limitata per KiCad 6+. | Nessun writer worksheet legacy KiCad 4/5. |
| `.kicad_dru` | Copiato solo se il target supporta lo stesso anchor fisso di design-rule. | Nessuna conversione del formato regole; i target non supportati saltano il file con warning. |

## Principi di downgrade

| Caso | Scelta implementativa | Esempi |
| --- | --- | --- |
| Il parser target non legge un nodo | Rimuove nodo/campo e produce warning. | `embedded_files`, `variants`, `barcodes`, `net_chains`, native ellipse. |
| Esiste rappresentazione vecchia | Rinomina, appiattisce o usa campo legacy. | `directive_label -> netclass_flag`, `stroke -> width`, `uuid/tstamp/id`. |
| Geometria target più debole | Converte in primitive più semplici. | PCB rectangles in lines, track arcs in segments, custom pads in rectangular pads. |
| Layout proprietà vecchio | Sposta proprietà, aggiunge o rimuove ID. | Posizione property hide, standard property id. |
| La source non contiene nuova semantica | Non crea oggetti di nuove funzioni. | Non genera automaticamente padstacks, variants, component classes o barcodes. |

## Correzioni di compatibilità attuali

| Area | Comportamento |
| --- | --- |
| Worksheet progetto KiCad 6/7/8 | I riferimenti page-layout a `kicad-embed://...kicad_wks` sono svuotati per evitare che KiCad 6/7/8 carichi percorsi worksheet embedded non supportati. |
| Schematic KiCad 6 | Root `uuid`, blocchi UUID dei pin di simboli piazzati, primitive di disegno root-level non supportate (`rectangle`, `circle`, `arc`, `polyline`, `bezier`) e fill color incompatibili sono rimossi o riportati a valori compatibili. |
| Schematic legacy KiCad 4/5 | Il testo multilinea è scritto come `\n` escaped su una sola riga; le conversioni progetto creano `<project>-cache.lib` e aggiungono `LIBS:<project>-cache`. |
| Librerie simboli legacy KiCad 4/5 | I campi reference `DEF` sono scritti come prefissi, non come riferimenti di istanza come `U1`. |
| Upgrade PCB/footprint | Footprint `attr dnp` resta un atomo `attr`; non viene espanso in `(dnp yes/no)`. |

## Conversione project directory

| Trattamento | Implementazione |
| --- | --- |
| Input | Directory o `.kicad_pro` è trattato come project tree. |
| Copia | KiCad documents, legacy documents, library tables, `.kicad_prl`, 3D model files. |
| Skip | VCS, history, backup, archive, gerber/fabrication/output/plot/BOM/assembly/vendor output. |
| Estensioni | KiCad 5/4 mappa `.kicad_sch/.kicad_sym/.kicad_pro` a `.sch/.lib/.pro`; KiCad 6+ fa l’opposto. |
| `.dcm` | In KiCad 6+, se esiste una `.lib` associata, `.dcm` non viene convertito separatamente. |
| `.kicad_dru` | Saltato per target senza supporto `.kicad_dru`; copiato quando l’anchor design-rule coincide. |
| Riferimenti worksheet progetto | Per KiCad 6/7/8, i riferimenti page-layout worksheet embedded sono svuotati in `.kicad_pro`. |
| Library tables | `sym-lib-table` / `fp-lib-table` sono normalizzati per la famiglia target. |
| Supporto schematic | Per KiCad 6+, i simboli locali sono embedded in `lib_symbols` e le hierarchy instances sono ricostruite. |
| Cache schematic legacy | Per KiCad 5/4, `Library.lib` è copiato in `<project>-cache.lib` e ogni `.sch` generato riceve `LIBS:<project>-cache`. |

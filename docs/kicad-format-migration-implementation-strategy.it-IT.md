# Strategia di implementazione per le migrazioni di formato KiCad

Questo testo è sincronizzato strutturalmente con il riferimento inglese; termini tecnici
KiCad, token e nomi di file sono lasciati intenzionalmente invariati.

Questo documento trasforma le differenze di formato in
`kicad-file-format-version-differences.md` in indicazioni di implementazione per migrazioni
tra versioni maggiori adiacenti di KiCad.

Il confine principale è KiCad 5 verso KiCad 6. KiCad 4 e 5 usano file legacy per schemi e
librerie di simboli (`.sch`, `.lib`, `.dcm`), mentre KiCad 6 e versioni successive usano
file S-expression (`.kicad_sch`, `.kicad_sym`). I file PCB e footprint sono già
S-expression in queste versioni, ma i loro insiemi di funzionalità e numeri di versione
richiedono comunque regole esplicite di riscrittura.

Ultimo aggiornamento: 2026-06-04.

## Ambito

Questo documento è una roadmap di implementazione, non una promessa di supporto release.
Descrive come implementare:

- Migrazioni in avanti come KiCad 6 a 7 e KiCad 5 a 6.
- Migrazioni all'indietro come KiCad 7 a 6, KiCad 6 a 5 e KiCad 5 a 4.
- Punti di estensione per versioni adiacenti successive come 7 a 8, 8 a 9,
  9 a 10 e 10 al ramo di sviluppo corrente.

Il convertitore C++ attuale è principalmente un motore di downgrade. Se la versione del
formato sorgente è più vecchia del target richiesto, attualmente copia il file senza
modifiche invece di aggiornarlo. Il supporto upgrade va quindi aggiunto come percorso di
migrazione separato, non indebolendo le regole di downgrade.

## Mappa delle versioni target

| Target KiCad | Libreria simboli | Schema | PCB / footprint | Worksheet | Regole di progettazione |
| --- | ---: | ---: | ---: | ---: | ---: |
| 4.0 | Legacy `.lib` 2.3 | Legacy `.sch` V2 | `4` | Legacy drawing sheet | Nessuna |
| 5.0 / 5.1 | Legacy `.lib` 2.4 | Legacy `.sch` V4 | `20171130` | Legacy drawing sheet | Nessuna |
| 6.0 | `20211014` | `20211123` | `20211014` | `20210606` | `20200610` |
| 7.0 | `20220914` | `20230121` | `20221018` | `20220228` | `20200610` |
| 8.0 | `20231120` | `20231120` | `20240108` | `20231118` | `20200610` |
| 9.0 | `20241209` | `20250114` | `20241229` | `20231118` | `20200610` |
| 10.0 | `20251024` | `20260306` | `20260206` | `20231118` | `20200610` |

## Forma del motore di migrazione

Implementare le migrazioni come pipeline con adattatori di famiglia file:

1. Rilevare il tipo di documento dal root S-expression head o da estensione/header legacy.
2. Eseguire il parse in un albero mutabile o in un typed intermediate model.
3. Selezionare una route dalla famiglia/versione sorgente alla famiglia/versione target.
4. Applicare passi di migrazione ordinati. Ogni passo deve essere protetto da tipo di
   documento, versione sorgente e versione target.
5. Scrivere la famiglia file target con versione ed estensione target.
6. Emettere warning per ogni rimozione lossy, default di fallback o conversione semantica
   non implementata.

Non implementare la migrazione come semplice riscrittura del numero di versione principale.
Ogni confine di versione può introdurre o rimuovere token, strutture di layout, attributi
oggetto o intere famiglie di file.

## Regole tra famiglie di file

### Famiglia legacy KiCad 4/5

KiCad 4 e 5 richiedono writer legacy per output schema e libreria simboli:

- Schema: `.sch`, con `EESchema Schematic File Version 2` per KiCad 4 e
  `Version 4` per KiCad 5.
- Libreria simboli: `.lib` più `.dcm` opzionale, comunemente
  `EESchema-LIBRARY Version 2.3` per KiCad 4 e `Version 2.4` per KiCad 5.
- Progetto: legacy `.pro`.

I dati schematici e simboli V6+ non possono essere downgradati a KiCad 4/5 potando pochi
node S-expression. L'implementazione richiede un vero serializer legacy o un warning chiaro
che la conversione non è supportata.

### Famiglia S-expression KiCad 6+

KiCad 6 e successivi usano file S-expression per schema, libreria simboli, PCB, footprint,
worksheet e regole di progettazione. La maggior parte delle migrazioni adiacenti può essere
gestita come tree rewrite guidato dalla versione:

- Aggiungere o rimuovere feature nodes in base alla versione target.
- Rinominare i campi quando il formato è cambiato.
- Convertire formati boolean-list in legacy presence atoms, o il contrario.
- Normalizzare strutture UUID, ID, font, text e property per il target.

## Strategia di migrazione in avanti

La migrazione in avanti deve preservare la semantica sorgente e sintetizzare default target
sicuri. Non deve inventare nuova intenzione progettuale.

Comportamento consigliato:

- Se sorgente e target sono nella stessa famiglia, fare parse e scrivere il formato target
  più nuovo applicando le riscritture di compatibilità note.
- Se un campo di formato è assente nella sorgente, ometterlo quando il formato nuovo lo
  consente; altrimenti scrivere default simili a quelli di KiCad.
- Se la migrazione attraversa da KiCad 4/5 legacy a KiCad 6+ S-expression, usare importer
  dedicati per `.sch`, `.lib`, `.dcm` e `.pro`.
- Per import legacy difficili, usare KiCad stesso come riferimento: caricare il vecchio
  progetto nella versione KiCad corrispondente o in KiCad 6+, salvare e confrontare la
  struttura generata con l'output del convertitore.

### Upgrade da KiCad 6 a KiCad 7

È un upgrade S-expression nella stessa famiglia. Può essere implementato come rewrite
strutturato più aggiornamento della versione target.

Azioni chiave:

- Aggiornare le versioni target a symbol `20220914`, schematic `20230121`, board /
  footprint `20221018` e worksheet `20220228`.
- Preservare gli oggetti V6 esistenti di schema, simboli, board, footprint, worksheet e DRC.
- Aggiungere default solo dove KiCad 7 richiede un valore che KiCad 6 non scriveva.
- Convertire campi vecchi ma compatibili alla nuova grafia quando serve, ad esempio
  `netclass_flag` in `directive_label`.
- Normalizzare la geometria text box alla rappresentazione KiCad 7 `at` / `size` se si
  incontra una vecchia rappresentazione `start` / `end`.
- Mantenere valide le informazioni di simulazione V6, ma non fabbricare dati completi di
  simulation model KiCad 7 senza campi sufficienti.
- Preservare o sintetizzare default legati a DNP solo quando il tipo oggetto target li supporta.
- Per PCB e footprints, preservare oggetti V6 e abilitare forme compatibili V7 per
  stroke formatting, footprint private layers, dimensions, teardrop keywords, net ties e
  pad/via zone-layer connections quando ci sono dati sufficienti.
- Per worksheets, scrivere la versione V7 e preservare font data solo se rappresentabile
  in modo compatibile con KiCad 7.

Fixture di validazione:

- Uno schema V6 con labels, fields, hierarchical sheets e simulation fields.
- Un board V6 con vias, pads, zones, dimensions e footprint text.
- Una libreria simboli V6 con properties e primitives semplici.

### Upgrade da KiCad 5 a KiCad 6

È il confine di migrazione in avanti più importante perché file schematici e simboli cambiano
famiglia.

Adattatori richiesti:

- Migrazione progetto `.pro` a `.kicad_pro` JSON.
- Legacy `.sch` V4 a `.kicad_sch` `20211123`.
- Legacy `.lib` / `.dcm` 2.4 a `.kicad_sym` `20211014`.
- `.kicad_pcb` / `.kicad_mod` `20171130` a `20211014`.
- Legacy drawing sheet a `.kicad_wks` `20210606`, se la conversione worksheet è supportata.

Azioni chiave:

- Parsare record schematici legacy in un typed model prima di scrivere S-expression KiCad 6.
  Evitare sostituzioni testuali riga per riga sui file `.sch`.
- Mappare simboli schematici legacy a KiCad 6 symbol instances con UUIDs, properties, paths,
  sheet instances e library identifiers.
- Generare UUIDs stabili dove KiCad 6 li richiede. Usare UUIDs deterministici derivati da
  path sorgente e identità oggetto quando conta la riproducibilità.
- Convertire legacy library aliases, fields, drawing primitives, pins e documentation
  records in simboli `.kicad_sym`.
- Preservare descriptions, keywords e documentation links `.dcm` come metadati simbolo
  KiCad 6 quando possibile.
- Convertire legacy project settings a `.kicad_pro` e `.kicad_prl` solo per impostazioni
  con equivalenti chiari. Avvisare per impostazioni UI o cache ignorate.
- Aggiornare la versione PCB a `20211014` preservando le feature KiCad 5, incluse custom
  pads, multi-layer keepouts, 3D model offsets e footprint text lock state.

Aree lossy attese:

- Alcune legacy project settings potrebbero non avere un equivalente JSON KiCad 6.
- Il comportamento legacy library rescue/cache può richiedere symbol remapping esplicito.
- Costrutti schematici V5 che dipendono da vecchie regole di library lookup possono
  richiedere warning o dati sidecar di libreria simboli.

## Strategia di migrazione all'indietro

La migrazione all'indietro deve rimuovere o riscrivere costrutti non supportati e segnalare
la perdita. Il file target deve essere caricabile dalla versione KiCad richiesta anche quando
semantiche più nuove non possono essere preservate.

Comportamento consigliato:

- Applicare regole di downgrade dalla più nuova alla più vecchia, così le feature successive
  sono rimosse prima dei rewrite di compatibilità più vecchi.
- Tenere i warning specifici: nominare la feature rimossa, contare i node coinvolti e
  includere la versione di introduzione.
- Per downgrade S-expression nella stessa famiglia, preservare identità oggetto dove il
  target la supporta.
- Per downgrade V6+ a V5/V4 di schema o simboli, usare un writer legacy dedicato e trattare
  oggetti V6+ non supportati come lossy.

### Downgrade da KiCad 7 a KiCad 6

È un downgrade S-expression nella stessa famiglia. Va implementato come rimozione mirata e
rewrite di compatibilità.

Versioni target:

- Libreria simboli: `20211014`.
- Schema: `20211123`.
- Board / footprint: `20211014`.
- Worksheet: `20210606`.
- Design rules: `20200610`.

Regole chiave:

- Rimuovere symbol class flags, symbol fonts, text boxes, text colors, unit display names e
  comportamento KiCad 7-only property-ID.
- Rimuovere schematic graphic primitives introdotte dopo V6 senza equivalente V6, incluse
  text boxes e nuovi label fields.
- Riscrivere `directive_label` nella forma V6-compatible `netclass_flag` se esiste un
  mapping fedele; altrimenti avvisare.
- Rimuovere o downgradare dash-dot-dot line style, text hyperlinks, field name visibility,
  do-not-autoplace options, DNP support e V7 simulation model fields.
- Spostare o semplificare symbol and sheet instance data nel layout atteso da KiCad 6.
- Rimuovere PCB text boxes, image objects, first-class net ties, V7 teardrop keywords,
  footprint private-layer changes che V6 non può parsare e pad/via zone-layer-connection
  additions.
- Convertire formati V7 stroke, font, boolean, lock e hide in forme compatibili con V6.
- Rimuovere worksheet font support aggiunto in `20220228`.

Aree lossy attese:

- Text boxes, images, net ties, DNP flags, hyperlinks e modern simulation model data possono
  non avere equivalenti V6 fedeli.
- Alcune V7 PCB dimensions e teardrop settings devono essere rimosse o appiattite.

### Downgrade da KiCad 6 a KiCad 5

È un downgrade cross-family per file schematici e simboli.

Writer richiesti:

- `.kicad_sch` `20211123` a legacy `.sch` V4.
- `.kicad_sym` `20211014` a legacy `.lib` 2.4 più `.dcm`.
- `.kicad_pro` JSON a legacy `.pro`.
- `.kicad_pcb` / `.kicad_mod` `20211014` a `20171130`.

Regole chiave:

- Serializzare KiCad 6 schematic symbols, wires, buses, labels, junctions, sheets, fields e
  page settings nel formato record legacy `.sch`.
- Convertire KiCad 6 UUIDs e paths in legacy timestamps o identifiers deterministici dove
  il target lo richiede.
- Dividere KiCad 6 symbol metadata in `.lib` symbol definitions e `.dcm`
  documentation records.
- Rimuovere strutture schematiche e simboli KiCad 6-only che i file legacy non possono
  rappresentare direttamente. Avvisare per ogni rimozione.
- Convertire `.kicad_pro` settings a `.pro` solo quando esiste un equivalente legacy noto.
  Ignorare o avvisare per settings JSON-only.
- Downgradare versioni board e footprint a `20171130`.
- Rimuovere feature V6 board/footprint che KiCad 5 non può parsare, incluse V6+
  UUID-only structures, newer zone behavior, rule file assumptions e qualsiasi oggetto
  introdotto dopo il formato PCB V5.
- Preservare feature PCB compatibili KiCad 5: custom pads, multi-layer keepouts,
  3D model offset in mm usando il parametro `offset` e locked footprint text.

Aree lossy attese:

- Dettagli S-expression schematici e simboli KiCad 6 non si mappano sempre a campi record legacy.
- Project JSON settings, identità UUID moderna e alcuni dettagli di library linkage possono
  essere ridotti o scartati.
- I file `.kicad_dru` non hanno equivalente standalone KiCad 5.

### Downgrade da KiCad 5 a KiCad 4

È principalmente un downgrade nella famiglia legacy, con PCB che richiede ancora rimozione
di feature S-expression.

Formati target:

- Schema: `.sch` V2.
- Libreria simboli: `.lib` 2.3 più `.dcm`.
- PCB / footprint: version `4`.
- Progetto: legacy `.pro`.

Regole chiave:

- Riscrivere l'header schema da `EESchema Schematic File Version 4` a `Version 2`.
- Rimuovere o riscrivere record schematici KiCad 5 che KiCad 4 non può parsare.
- Downgradare header libreria simboli da output stile 2.4 a output stile 2.3.
- Rimuovere symbol fields o attributes non esistenti nelle librerie KiCad 4.
- Downgradare file board e footprint da `20171130` a version `4`.
- Rimuovere feature PCB KiCad 5 introdotte dopo KiCad 4, incluse differential pair settings
  per net class, long pad names, custom pad shape details, multi-layer keepouts,
  3D model offset changes che KiCad 4 non può interpretare e locked/unlocked footprint text.
- Convertire 3D model offsets alla rappresentazione KiCad 4 quando è nota una conversione
  di unità reversibile; altrimenti avvisare e rimuovere i campi offset non supportati.

Aree lossy attese:

- Custom pads e multi-layer keepouts possono dover essere semplificati o rimossi.
- Long pad names possono richiedere troncamento o rinomina deterministica.
- Alcuni metadati net-class e footprint text-lock non possono essere preservati.

## Pattern di downgrade adiacenti successivi

Lo stesso framework di downgrade dovrebbe coprire versioni adiacenti successive:

| Route | Focus principale dell'implementazione |
| --- | --- |
| KiCad 8 a 7 | Rimuovere V8 generator cleanup fields, PCB fields, generated objects, UUID normalization, custom pad spoke templates, modern teardrops, images, rule-area changes, simulation/exclude flags e worksheet embedded images. |
| KiCad 9 a 8 | Rimuovere embedded files, tables, rule areas, component classes, complex padstacks, via stacks, arbitrary user layers, tenting, multiple netclass assignments e netclass color highlighting. |
| KiCad 10 a 9 | Rimuovere variants, jumper pads, barcodes, via protection, zone hatch offsets, backdrill, split via types, stopped-netcode assumptions, rounded rectangles, stacked pins, PCB points e property-formatting updates. |
| Sviluppo corrente a KiCad 10 | Rimuovere feature post-10.0: extruded 3D body metadata, native ellipses and ellipse arcs, dielectric frequency-dependent stackup fields, net chains, copper thieving, pad `sim_electrical_type` e PCB table-cell `knockout`. |

Le migrazioni in avanti per queste route sono di solito meno lossy dei downgrade, ma
richiedono comunque rewrite di compatibilità e default target. Per esempio, KiCad 7 a 8
dovrebbe introdurre V8-normalized `generator_version` handling solo dove il formato target
lo richiede, e KiCad 8 a 9 non dovrebbe inventare embedded files, component classes o
padstacks se non esistono già nella semantica sorgente.

## Strategia di test

Creare un set di fixture per ogni route adiacente e tipo documento:

- File progetto: `.pro` e `.kicad_pro`.
- File schematici: legacy `.sch` e `.kicad_sch`.
- Librerie simboli: `.lib`, `.dcm` e `.kicad_sym`.
- File PCB: `.kicad_pcb`.
- Footprints: `.kicad_mod`.
- Worksheets: legacy drawing sheet e `.kicad_wks`.
- Design rules: `.kicad_dru` solo per KiCad 6+.

Ogni test dovrebbe verificare:

- Tipo sorgente e versione sorgente rilevati correttamente.
- Famiglia file, estensione e versione target corrette.
- Token vietati dalla versione target assenti.
- Strutture compatibili note preservate.
- Cambiamenti lossy producono warning.
- Rieseguire la stessa migrazione è stabile e non continua a modificare il file.

Per route V5/V6 e V6/V5, aggiungere golden-file tests generati da KiCad dove possibile.
Queste route attraversano un confine di famiglia file e hanno il rischio più alto di deriva
semantica.

## Ordine di implementazione

Ordine consigliato:

1. Completare downgrade S-expression same-family da V7 a V6.
2. Aggiungere downgrade V5 PCB / footprint a V4 perché resta nella famiglia PCB S-expression.
3. Implementare parser legacy schema e simboli per upgrade V5 a V6.
4. Implementare writer legacy schema e simboli per downgrade V6 a V5.
5. Aggiungere downgrade legacy schema e simboli V5 a V4.
6. Aggiungere upgrade same-family come V6 a V7.
7. Estendere lo stesso framework a route V8, V9, V10 e sviluppo corrente.

Questo ordine consegna presto una copertura downgrade utile, isolando al tempo stesso il
lavoro molto più difficile di conversione legacy di schemi e simboli.

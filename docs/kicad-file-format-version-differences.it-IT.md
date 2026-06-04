# Differenze di versione del formato file KiCad

Questo documento tiene traccia delle differenze di versione del formato file KiCad utilizzate dal backport
convertitore. È organizzato in modo da poter aggiungere nuove versioni stabili o di sviluppo
senza rinominare il file.

Ultimo aggiornamento: 2026-06-05.

## Fonti e metodo

Fonti recensite:

- Tag GitLab ufficiali di KiCad e file sorgente.
- Checkout locale di KiCad in `E:/WORKS/MY/kicadProject/kicad`.
- Riferimenti e tag locali: `origin/4.0`, `4.0.0`, `origin/5.0`, `origin/5.1`,
  `5.0.0`, `5.1.0`, `6.0.0`, `7.0.0`, `8.0.0`, `9.0.0`, `10.0.0`,
e `origin/10.0`.
- KiCad locale `master`, utilizzato solo per identificare il ramo di sviluppo successivo alla 10.0
confini.
- `kicad-backport-cplus` implementazione, in particolare:
  - `src/kicad_backport_versions.cpp`
  - `src/kicad_backport_rules.cpp`
  - `src/kicad_backport_rule_rewriters.cpp`
  - `src/kicad_backport_upgrade.cpp`
  - `src/kicad_backport.cpp`
- File di intestazione della versione:
  - `pcbnew/kicad_plugin.h` per i formati PCB KiCad 4/5.
  - `pcbnew/plugins/kicad/pcb_plugin.h` per i formati PCB KiCad 6/7.
  - `eeschema/sch_file_versions.h`
  - `pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h`
  - `include/drawing_sheet/ds_file_versions.h`
  - `pcbnew/drc/drc_rule_parser.h`
  - `eeschema/general.h` e `eeschema/sch_legacy_plugin.h` per KiCad 4/5
formati schematici legacy.

I numeri di versione sono presi dalle macro sorgente attive di KiCad:

- `SEXPR_SYMBOL_LIB_FILE_VERSION`
- `SEXPR_SCHEMATIC_FILE_VERSION`
- `SEXPR_BOARD_FILE_VERSION`
- `SEXPR_WORKSHEET_FILE_VERSION`
- `DRC_RULE_FILE_VERSION`

Note:

- Le versioni Board S-expression coprono anche i file di impronte `.kicad_mod`.
- `.kicad_dru` è rimasto a `20200610` da KiCad 6.0 fino ai sorgenti attuali 10.99.
Ciò significa solo che la macro della versione non è cambiata; la semantica delle regole può ancora avere
cambiato.
- `.kicad_pro` è un file di progetto JSON e utilizza invece la migrazione delle impostazioni/schema
di queste macro della versione della data di espressione S. Differenze nello schema JSON del progetto
dovrebbero essere monitorati separatamente.
- Gli schemi e le librerie di simboli di KiCad 4/5 sono legacy `.sch`, `.lib` e
file `.dcm`, non `.kicad_sch` o `.kicad_sym`.

## Matrice principale della famiglia di file

| Versione principale di KiCad | Progetto | Schematico | Libreria di simboli | PCB/ingombro | Foglio di lavoro | Regole di progettazione | Punto chiave |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 4.0 | `.pro` precedente | Precedenti `.sch`, `EESCHEMA_VERSION=2` | `.lib` `EESchema-LIBRARY Version 2.3`, `.dcm` | `.kicad_pcb` / `.kicad_mod` Espressione S, versione `4` | Foglio di disegno legacy | Nessun `.kicad_dru` autonomo | Il PCB era già espressione S; le librerie di schemi e simboli erano ancora legacy. |
| 5.0 / 5.1 | `.pro` precedente | Precedenti `.sch`, `EESCHEMA_VERSION=4` | Comunemente `.lib` `Version 2.4`, `.dcm` | `.kicad_pcb` / `.kicad_mod` Espressione S, versione `20171130` | Foglio di disegno legacy | Nessun `.kicad_dru` autonomo | PCB ha aggiunto pad personalizzati, protezioni multistrato e modifiche all'offset del modello 3D; lo schema è rimasto un'eredità. |
| 6.0 | JSON `.kicad_pro`, `.kicad_prl` | `.kicad_sch` `20211123` | `.kicad_sym` `20211014` | `20211014` | `.kicad_wks` `20210606` | `.kicad_dru` `20200610` | Nuovi formati di espressioni S di schemi e simboli. |
| 7.0 | JSON `.kicad_pro` | `.kicad_sch` `20230121` | `.kicad_sym` `20220914` | `20221018` | `.kicad_wks` `20220228` | `20200610` | Caselle di testo, caratteri, DNP, modifiche al modello di simulazione, legami netti, immagini, parole chiave a goccia. |
| 8.0 | JSON `.kicad_pro` | `.kicad_sch` `20231120` | `.kicad_sym` `20231120` | `20240108` | `.kicad_wks` `20231118` | `20200610` | `generator_version`, pulizia V8, campi PCB, oggetti generati, normalizzazione UUID. |
| 9.0 | JSON `.kicad_pro` | `.kicad_sch` `20250114` | `.kicad_sym` `20241209` | `20241229` | `.kicad_wks` `20231118` | `20200610` | File incorporati, tabelle, aree di regole, classi di componenti, padstack, via stack, livelli utente arbitrari. |
| 10.0 | JSON `.kicad_pro` | `.kicad_sch` `20260306` | `.kicad_sym` `20251024` | `20260206` | `.kicad_wks` `20231118` | `20200610` | Varianti, jumper pad, codici a barre, protezione via, backdrill, tipi via divisi, smesso di scrivere netcode. |

## Matrice delle versioni stabili

| Etichetta KiCad | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

## Confini legacy di KiCad 4/5

KiCad 4 e 5 non sono solo versioni precedenti di espressioni S per dati schematici. Essi
utilizzare una diversa famiglia di file di schemi e librerie di simboli:

| Zona | KiCad 4.0 | KiCad 5.0/5.1 |
| --- | --- | --- |
| Intestazione schematica | `EESchema Schematic File Version 2` | `EESchema Schematic File Version 4` |
| Macro schematica | `EESCHEMA_VERSION 2` | `EESCHEMA_VERSION 4` |
| Intestazione della libreria di simboli | Comunemente `EESchema-LIBRARY Version 2.3` | Comunemente `EESchema-LIBRARY Version 2.4` |
| Versione PCB | `4` | `20171130` |

La versione PCB/impronta di KiCad 5 punta prima della linea di sviluppo di KiCad 6:

| Versione | Modifica |
| ---: | --- |
| 20160815 | Impostazioni di coppia differenziali per classe di rete |
| 20170123 | `EDA_TEXT` refactoring; spostato `hide` |
| 20170920 | Nomi lunghi dei pad e forma personalizzata dei pad |
| 20170922 | Le zone vietate possono esistere su più livelli |
| 20171114 | Salva l'offset del modello 3D in mm anziché in pollici |
| 20171125 | Testo dell'impronta bloccata/sbloccata |
| 20171130 | Offset del modello 3D scritto utilizzando il parametro `offset` |

Implicazioni del backport:

- Gli obiettivi schematici di KiCad 4/5 richiedono uno scrittore `.sch` legacy, non solo un file
`.kicad_sch` riscrittura della versione.
- Le destinazioni dei simboli di KiCad 4/5 richiedono l'output legacy `.lib` / `.dcm` o un output esplicito
avviso con perdita/non implementato.
- Gli obiettivi della scheda KiCad 4 utilizzano la versione `4`; Le destinazioni della scheda KiCad 5 utilizzano `20171130`.
- UUID V6+, caselle di testo, file incorporati, varianti, tabelle, aree di regole, componenti
classi, padstack, via stack, backdrill e strutture simili non possono esserlo
conservato direttamente nei file V4/V5.

## Matrice della versione di sviluppo attuale

Il ramo KiCad `master` recensito è già passato allo sviluppo 11.0.
Questi risultati sono elementi di sviluppo successivi alla versione 10.0 e non devono essere etichettati come KiCad
Supporto formato stabile 10.0:

| Tipo di file | Versione di sviluppo attuale | Note |
| --- | ---: | --- |
| Scheda `.kicad_pcb` | `20260603` | Flag di eliminazione sulle celle della tabella |
| Impronta `.kicad_mod` | `20260603` | I footprint utilizzano la versione PCB S-expression |
| Schema `.kicad_sch` | `20260512` | Catene nette |
| Libreria simboli `.kicad_sym` | `20260508` | Primitiva dell'ellisse nativa |
| Foglio di lavoro `.kicad_wks` | `20231118` | Versione del generatore/pulizia di KiCad 8 |
| Regole di progettazione `.kicad_dru` | `20200610` | Nessun bump di versione specifico per lo sviluppo corrente trovato |

Passaggi della versione di sviluppo successiva alla 10.0 trovati finora:

| Versione | Tipo di file | Modifica |
| ---: | --- | --- |
| 20260410 | Board / footprint | Corpo 3D estruso |
| 20260508 | Board / footprint | Primitive PCB native di ellisse e arco di ellisse |
| 20260508 | Schema/simbolo | Primitive native di ellisse schematica/simbolo e arco di ellisse |
| 20260511 | Asse | Modelli di stackup dielettrici dipendenti dalla frequenza |
| 20260512 | Scheda/schema | Catene nette |
| 20260513 | Asse | Modalità riempimento zona ladro rame |
| 20260521 | Board / footprint | Tipi elettrici di simulazione del pad |
| 20260603 | Board / footprint | Flag di eliminazione sulle celle della tabella |

## 6.0-7.0

### Biblioteca dei simboli

| Versione | Modifica |
| ---: | --- |
| 20220101 | Bandiere di classe |
| 20220102 | Caratteri |
| 20220126 | Caselle di testo |
| 20220328 | Casella di testo `start/end` modificata in `at/size` |
| 20220331 | Colori del testo |
| 20220914 | Nomi visualizzati delle unità simbolo |
| 20220914 | Gli ID proprietà non vengono più salvati |

### Schematico

| Versione | Modifica |
| ---: | --- |
| 20220101 | Cerchi, archi, rettangoli, poligoni, bezier |
| 20220102 | Trattino-punto-punto |
| 20220103 | Campi etichetta |
| 20220104 | Caratteri |
| 20220124 | `netclass_flag` rinominato in `directive_label` |
| 20220126 | Caselle di testo |
| 20220328 | Casella di testo `start/end` modificata in `at/size` |
| 20220331 | Colori del testo |
| 20220404 | Dati dell'istanza del simbolo dello schema predefinito |
| 20220622 | Nuovo formato del modello di simulazione |
| 20220820 | Correzione dei dati dell'istanza del simbolo predefinita |
| 20220822 | Collegamenti ipertestuali a oggetti di testo |
| 20220903 | Visibilità del nome del campo |
| 20220904 | Opzione di campo Non posizionare automaticamente |
| 20220914 | Supporto DNP |
| 20220929 | Gli ID proprietà non vengono più salvati |
| 20221002 | I dati dell'istanza sono tornati alla definizione del simbolo |
| 20221004 | I dati dell'istanza sono tornati alla definizione del simbolo |
| 20221110 | I dati dell'istanza del foglio sono stati spostati nella definizione del foglio |
| 20221126 | Valore e impronta rimossi dai dati dell'istanza |
| 20221206 | Campi del modello di simulazione da V6 a V7 |
| 20230121 | `SCH_MARKER` serializzazione del percorso del foglio |

### PCB/impronta

| Versione | Modifica |
| ---: | --- |
| 20211226 | Dimensione radiale |
| 20211227 | L'angolo dei raggi di scarico termico viene ignorato |
| 20211228 | Attributo impronta `allow_soldermask_bridges` |
| 20211229 | Formattazione del tratto |
| 20211230 | Dimensioni in impronte |
| 20211231 | Livelli di impronte private |
| 20211232 | Caratteri |
| 20220131 | Caselle di testo |
| 20220211 | Supporto della strategia di riempimento della zona V5 terminato |
| 20220225 | Rimosso TEDIT |
| 20220308 | Proprietà del testo eliminato e del testo grafico bloccato |
| 20220331 | Traccia su tutte le impostazioni di selezione dei livelli |
| 20220417 | Precisioni dimensionali automatiche |
| 20220427 | Edge.Cuts e Margin esclusi dai layer privati ​​del footprint |
| 20220609 | Parole chiave a goccia |
| 20220621 | Supporto per immagini |
| 20220815 | `allow-soldermask-bridges-in-FPs` contrassegno |
| 20220818 | Legami netti di prima classe |
| 20220914 | Caselle numeriche per pad di forma personalizzata |
| 20221018 | Connessioni zona-strato tramite e pad |

### Foglio di lavoro

| Versione | Modifica |
| ---: | --- |
| 20220228 | Supporto per i caratteri |

## 7.0-8.0

### Biblioteca dei simboli

| Versione | Modifica |
| ---: | --- |
| 20230620 | `ki_description` cambiato nel campo `Description` |
| 20231120 | `generator_version` e pulizia V8 |

### Schematico

| Versione | Modifica |
| ---: | --- |
| 20230221 | Simboli di potere moderni, valore modificabile = netto |
| 20230409 | `exclude_from_sim` markup |
| 20230620 | `ki_description` cambiato nel campo `Description` |
| 20230808 | Il campo `Sim.Enable` è stato spostato nell'attributo `exclude_from_sim` |
| 20230819 | Livelli multipli di ereditarietà dei simboli della libreria |
| 20231120 | `generator_version` e pulizia V8 |

### PCB/impronta

| Versione | Modifica |
| ---: | --- |
| 20230410 | Attributo DNP propagato dallo schema a `attr` |
| 20230517 | Pad e tramite parametri a goccia |
| 20230620 | Campi PCB |
| 20230730 | Connettività di forme grafiche |
| 20230825 | Flag di bordo esplicito della casella di testo |
| 20230906 | Supporto per più tipi di immagini |
| 20230913 | Modelli per raggi con imbottitura personalizzata |
| 20231007 | Oggetti generativi |
| 20231014 | Normalizzazione del formato file V8 |
| 20231212 | Blocco immagine di riferimento/UUID, formato booleano impronta |
| 20231231 | Generatori e gruppi utilizzano `uuid` invece di `id` |
| 20240108 | I parametri a goccia sono stati modificati in booleani espliciti |

### Foglio di lavoro

| Versione | Modifica |
| ---: | --- |
| 20230607 | Immagini salvate come base64 |
| 20231118 | `generator_version` e pulizia del formato file V8 |

## 8.0-9.0

### Biblioteca dei simboli

| Versione | Modifica |
| ---: | --- |
| 20240529 | File incorporati |
| 20240819 | Algoritmo hash del file incorporato modificato in Murmur3 |
| 20241209 | `SCH_FIELD` flag privati |

### Schematico

| Versione | Modifica |
| ---: | --- |
| 20240101 | Tabelle |
| 20240417 | Aree di regola |
| 20240602 | Attributi del foglio |
| 20240620 | File incorporati |
| 20240716 | Assegnazioni multiple di netclass |
| 20240812 | Evidenziazione del colore Netclass |
| 20240819 | Algoritmo hash del file incorporato modificato in Murmur3 |
| 20241004 | Il simbolo `hide` utilizza valori booleani |
| 20241209 | `SCH_FIELD` flag privati |
| 20250114 | I riferimenti incrociati alle variabili di testo utilizzano percorsi completi |

### PCB/impronta

| Versione | Modifica |
| ---: | --- |
| 20240201 | Le sostituzioni utilizzano proprietà nullable |
| 20240202 | Tabelle |
| 20240225 | `solder_paste_margin` razionalizzazione |
| 20240609 | Parola chiave `tenting` |
| 20240617 | Angoli del tavolo |
| 20240703 | Tipi di livello utente |
| 20240706 | File incorporati |
| 20240819 | Algoritmo hash del file incorporato modificato in Murmur3 |
| 20240928 | Classi dei componenti |
| 20240929 | Padstack complessi |
| 20241006 | Tramite pile |
| 20241007 | Le tracce possono trasportare lo strato e il margine della maschera di saldatura |
| 20241009 | Evoluzione del formato dell'area delle regole di posizionamento |
| 20241010 | Le forme grafiche possono contenere uno strato e un margine della maschera di saldatura |
| 20241030 | Direzioni delle frecce di quotatura, normalizzazione `suppress_zeroes` |
| 20241129 | Proprietà `keep_text_aligned` e riempimento normalizzate |
| 20241228 | I punti della curva a goccia sono diventati booleani |
| 20241229 | Livelli utente espansi fino a un conteggio arbitrario |

### Foglio di lavoro

Nessun aumento della versione del foglio di lavoro; rimane `20231118`.

## dalle 9.0 alle 10.0

### Biblioteca dei simboli

| Versione | Modifica |
| ---: | --- |
| 20250318 | `~` non significa più testo vuoto |
| 20250324 | Gruppi di pin dei ponticelli |
| 20250829 | Rettangoli arrotondati |
| 20250901 | Notazione dei pin impilati |
| 20250925 | Alias ​​del bus nel file di progetto |
| 20251024 | Aggiornamenti alla formattazione delle proprietà: `do_not_autoplace`, `show_name` |

### Schematico

| Versione | Modifica |
| ---: | --- |
| 20250222 | Riempimenti tratteggiati per le forme |
| 20250227 | Simboli del potere locale |
| 20250318 | `~` non significa più testo vuoto |
| 20250425 | UUID per le tabelle |
| 20250513 | I gruppi possono contenere il blocco di progettazione `lib_id` |
| 20250610 | Le aree delle regole supportano DNP e altri flag |
| 20250827 | Stili di carrozzeria personalizzati |
| 20250829 | Rettangoli arrotondati |
| 20250901 | Notazione dei pin impilati |
| 20250922 | Varianti schematiche |
| 20251012 | Supporto gerarchico schematico piatto |
| 20251028 | Aggiornamenti alla formattazione delle proprietà: `do_not_autoplace`, `show_name` |
| 20260101 | Varianti del circuito stampato |
| 20260306 | Semantica della variante `in_bom` corretta |

### PCB/impronta

| Versione | Modifica |
| ---: | --- |
| 20250210 | Eliminazione della casella di testo |
| 20250222 | Tratteggio di forme PCB |
| 20250228 | IPC-4761 tramite funzionalità di protezione |
| 20250302 | Offset del tratteggio delle zone |
| 20250309 | Regole di assegnazione dinamica della classe componente |
| 20250324 | Cuscinetti per ponticelli |
| 20250401 | Regolazione della lunghezza nel dominio del tempo |
| 20250513 | I gruppi possono contenere il blocco di progettazione `lib_id` |
| 20250801 | `(island)` cambiato in `(island yes/no)` |
| 20250811 | Proprietà di fabbricazione del cuscinetto a pressione |
| 20250818 | Le impronte supportano conteggi di livelli personalizzati |
| 20250829 | Rettangoli arrotondati |
| 20250901 | Punti PCB |
| 20250907 | UUID per le tabelle |
| 20250909 | Metadati delle unità dell'impronta: unità/pin |
| 20250914 | `PCB_BARCODE` oggetti |
| 20250926 | Via tipologie suddivise in cieche/interrate/passanti |
| 20251027 | Correzione del ridimensionamento dei ritardi pad-to-die |
| 20251028 | Ho smesso di scrivere netcode; sono dettagli di implementazione interna |
| 20251101 | Backdrill e supporto per trapano terziario |
| 20260101 | Varianti PCB con sostituzioni per ingombro |
| 20260206 | Correzioni della serializzazione di codici a barre e attributi di variante |

### Foglio di lavoro

Nessun aumento della versione del foglio di lavoro; rimane `20231118`.

## 10.0 allo sviluppo attuale

Rispetto ai file di destinazione di KiCad 10, il ramo di sviluppo attuale rivisto
aggiunge questi passaggi di formattazione più recenti:

| Versione | Tipo di file | Differenza |
| ---: | --- | --- |
| 20260410 | Board / footprint | Metadati del corpo 3D estrusi nei blocchi del modello di impronta |
| 20260508 | Board / footprint | Primitive PCB native di ellisse e arco di ellisse |
| 20260508 | Schema/simbolo | Primitive native di ellisse schematica/simbolo e arco di ellisse |
| 20260511 | Asse | Campi del modello di stackup dipendenti dalla frequenza dielettrica |
| 20260512 | Asse | Blocco di aggregazione della catena netta |
| 20260512 | Schematico | Record della catena netta |
| 20260513 | Asse | Modalità riempimento zona ladro rame |
| 20260521 | Board / footprint | Tipo elettrico di simulazione del pad, serializzato come `sim_electrical_type` sui pad |
| 20260603 | Board / footprint | Flag `knockout` della cella della tabella |

## Riepilogo destinazione backport dai file di sviluppo correnti

Rispetto ai target supportati più vecchi, 10.99 introduce o mantiene quelli più recenti
costrutti che devono essere rimossi, semplificati o rinominati durante il backport:

| Bersaglio | Obiettivo scheda/impronta | Obiettivo schematico | Bersaglio simbolo | Principali aree di downgrade rispetto allo sviluppo attuale |
| --- | ---: | ---: | ---: | --- |
| KiCad 10 | `20260206` | `20260306` | `20251024` | Rimozione dei metadati del corpo estruso di solo sviluppo, delle ellissi native, dei campi di frequenza dielettrica, delle catene di rete, del furto di rame, dei tipi elettrici di simulazione dei pad e dei flag di eliminazione delle celle della tabella |
| KiCad 9 | `20241229` | `20250114` | `20241209` | KiCad 10 elementi più tratteggio forma PCB, protezione tramite, offset tratteggio di zona, jumper pad, ID blocchi di progettazione di gruppo, conteggi di livelli personalizzati, rettangoli arrotondati, punti PCB, UUID tabella, codici a barre, tipi divisi tramite, omissione netcode, backdrill/post-lavorazione, varianti PCB, varianti schema/stili corpo/rettangoli arrotondati/pin impilati/formattazione proprietà |
| KiCad8 | `20240108` | `20231120` | `20231120` | Elementi di KiCad 9 più tabelle, file incorporati, classi di componenti, padstack, tramite stack, aree di regole, tende, espansione del livello utente, attributi del foglio, assegnazioni multiple di netclass, evidenziazione colore di netclass |
| KiCad7 | `20221018` | `20230121` | `20220914` | Elementi KiCad 8 più campi PCB, propagazione degli attributi DNP, lacrime moderne, modelli di raggi pad personalizzati, generatori, normalizzazione UUID/id, caselle di testo, immagini, collegamenti netti, formattazione di caratteri/campi, aree di regole, simulazione di schemi moderni/flag di esclusione |

## Copertura dell'implementazione del backport C++

La CLI `kicad-backport-cplus` implementa riscritture di espressioni S basate sulla versione.
Risolve gli alias di rilascio per tipo di documento, quindi applica le regole di downgrade
scrive il campo `version` di destinazione. Accetta anche un formato di file numerico non elaborato
versione per testare un cutoff specifico del parser.

Mappature alias supportate nel codice:

| Alias | Simbolo | Schematico | Asse | Orma | Foglio di lavoro | Regole di progettazione |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |

Se il file sorgente ha già esattamente la versione numerica richiesta, il file
il convertitore lo copia senza modifiche. Se la versione di origine è inferiore a quella di destinazione,
l'implementazione C++ ora applica prima aggiornamenti di compatibilità limitati
scrivendo il campo `version` richiesto:

| Tipo | Implementata la normalizzazione dell'aggiornamento |
| --- | --- |
| Libreria di simboli | Espandi gli atomi di stile dei caratteri legacy per obiettivi moderni; espandere gli atomi di visibilità dei pin; sposta la proprietà `hide` fuori `effects`; rimuovere gli ID proprietà legacy. |
| Schematico | Rinomina `tstamp` in `uuid`; rinominare `netclass_flag` in `directive_label`; converti la vecchia casella di testo `start/end` in `at/size`; espandere i caratteri legacy e gli atomi di visibilità dei pin; sposta la proprietà `hide` fuori `effects`; rimuovere gli ID proprietà legacy. |
| Board / footprint | Rinominare `tstamp` in `uuid` per target moderni; espandere gli atomi in stile carattere; espandere l'impronta `dnp` atomi; normalizza i booleani in stile KiCad 7 `yes/no`; rimuovi `tedit` obsoleto; facoltativamente convertire i riferimenti netti numerici legacy in nomi di rete. |

Questo non è un motore di aggiornamento semantico completo; normalizza solo la sintassi the
il convertitore sa già come esprimere.

### Copertura del target di rilascio implementata

Le regole C++ sono guidate dal cutoff, quindi ogni alias di rilascio attiva le regole di cui
i cutoff sono più recenti della versione di destinazione di quella famiglia di file. Il seguente riassunto
elenca la copertura pratica per gli obiettivi stabili non V6.

#### Obiettivo KiCad 10

Gli obiettivi di KiCad 10 rimuovono principalmente i costrutti post-10.0/current-development:

| Tipo | Gestione implementata |
| --- | --- |
| Libreria di simboli | Rimuove le primitive native di ellisse e arco di ellisse introdotte dopo la destinazione del simbolo 10.0. |
| Schematico | Rimuovere i campi `locked` successivi alla versione 10.0, le primitive ellittiche native e `net_chain` / `net_chains`. |
| Board / footprint | Rimuovere o eseguire il downgrade dei blocchi del modello digitato/estruso post-10.0, delle primitive ellittiche native, dei campi di accumulo della frequenza dielettrica, delle catene di rete, della modalità di riempimento del furto di rame, del pad `sim_electrical_type` e della cella della tabella `knockout`. |
| File laterali del progetto | Per il suffisso V10 non viene generata alcuna riscrittura di compatibilità legacy `.kicad_prl` o tabella di libreria. |

#### Obiettivo KiCad 9

Le destinazioni di KiCad 9 rimuovono KiCad 10 e la sintassi di sviluppo attuale mantenendola
funzionalità valide nelle versioni dei file KiCad 9:

| Tipo | Gestione implementata |
| --- | --- |
| Libreria di simboli | Rimuovere i gruppi di pin dei ponticelli, i rettangoli arrotondati, le ellissi native, il simbolo `in_pos_files`, `duplicate_pin_numbers_are_jumpers`, i flag di classe `power`, la proprietà `show_name` / `do_not_autoplace` e il carattere `face`. |
| Schematico | Rimuovi rettangoli arrotondati, varianti di schema, ellissi native, catene di rete, post-target `locked`, `embedded_fonts`, stili di corpo personalizzati, flag di assemblaggio/simulazione del foglio, simbolo `in_pos_files`, flag di jumper/classe di potenza, carattere `face`, campi di formattazione delle proprietà e nodi radice `group`. |
| Board / footprint | Rimuovere o eseguire il downgrade di IPC-4761 tramite protezione, campi jumper pad, origini di posizionamento di classi di componenti, riempimenti di tratteggi PCB, conteggi di livelli personalizzati, rettangoli arrotondati, oggetti punto PCB, codici a barre, campi backdrill/post-lavorazione, varianti PCB, funzionalità di sviluppo attuale e carattere `face`; ricostruire i netcode della scheda numerica legacy. Il tenting è stato declassato dagli elenchi booleani fronte/retro ad atomi legacy per questo intervallo target. |
| File laterali del progetto | Non viene generata alcuna riscrittura `.kicad_prl` legacy per V9. |

#### Obiettivo KiCad 8

I target di KiCad 8 rimuovono la sintassi di KiCad 9/10/attuale e ne normalizzano anche diversi
I moduli di sviluppo della fine di KiCad-8 risalgono alle versioni dei file 8.0.0:

| Tipo | Gestione implementata |
| --- | --- |
| Libreria di simboli | Rimuovere i file incorporati/campi privati ​​V9+ e il ponticello V10+, il rettangolo arrotondato e la sintassi dell'ellisse; rimuovere `embedded_fonts`, font `face`, campi di formattazione simbolo/proprietà; aggiungi gli ID delle proprietà legacy e sposta la visibilità delle proprietà in `effects`; converte lo stile del carattere e i booleani di visibilità dei pin nella sintassi atom precedente. |
| Schematico | Rimuovere le tabelle V9+, le aree delle regole, i file incorporati/campi privati ​​e la sintassi V10+ di rettangolo arrotondato, variante, stile corpo ed ellisse/catena di rete; rimuovere testo e flag di simulazione/assemblaggio del foglio, campi di formattazione di simboli/proprietà, font `face`; aggiungi gli ID delle proprietà legacy e sposta la visibilità delle proprietà in `effects`; convertire i booleani di visibilità dei caratteri e dei pin nella sintassi atom precedente; rimuovere i nodi root `group`. |
| Board / footprint | Rimuovi tabelle V9+, tende, file/caratteri incorporati, classi di componenti, padstack complessi, stack di via, aree di regole, protezione di via, qualificatori di livello utente arbitrari, conteggi di livelli personalizzati, rettangoli arrotondati, punti PCB, codici a barre, backdrill/post-lavorazione, varianti e funzionalità di sviluppo corrente. Rimuovi anche i campi del margine/livello della maschera di saldatura grafica/della traccia, l'angolo della cella della tabella, le cache di rendering del testo, il knockout della casella di testo/cella della tabella/del livello, il modello `hide`, il carattere `face` e aggiungi i netcode numerici legacy. `solder_paste_margin_ratio` viene rinominato `solder_paste_ratio`. |
| File laterali del progetto | Genera impostazioni di visualizzazione dell'ID numerico legacy `.kicad_prl` per le schede V8. |

#### Obiettivo KiCad 7

Le destinazioni di KiCad 7 rimuovono la sintassi corrente di KiCad 8/9/10 e applicano un parser aggiuntivo
la compatibilità riscrive i campi PCB, gli UUID e i dati di impronta:

| Tipo | Gestione implementata |
| --- | --- |
| Libreria di simboli | Rimuovere V8+ `generator_version`, caratteri/file incorporati, campi privati ​​V9, sintassi ponticello/arrotondato/ellisse V10, simbolo `exclude_from_sim`, campi di formattazione proprietà e file di posizione, flag ponticello/classe di potenza e carattere `face`; aggiungere ID proprietà legacy; sposta la visibilità della proprietà in `effects`; convertire booleani di carattere e visibilità nella sintassi atom. |
| Schematico | Rimuovi V8+ `generator_version` e `fields_autoplaced`, tabelle V9+/aree di regole/campi incorporati/privati, sintassi V10+ arrotondata/variante/stile corpo, campi di esclusione della simulazione post-destinazione, flag di assemblaggio/simulazione di fogli, campi di formattazione simbolo/proprietà, carattere `face` e nodi root `group`. Gli atomi UUID non sono quotati per i parser KiCad 6/7 e la visibilità/ID delle proprietà vengono declassati al posizionamento legacy. |
| Board / footprint | Rimuovi oggetti generati da V8+, lacrime, tabelle, file/caratteri incorporati, classi di componenti, stack pad/via, aree di regole, protezione via e sintassi di destinazione più recente. Converti i qualificatori di tipo livello utente in `user`; rimuovere i campi della maschera di saldatura grafica/traccia, gli angoli della tabella, le cache di rendering, i flag di eliminazione, il modello `hide`, la connettività di rete grafica, i campi bloccati di gruppo, i campi di connessione tramite livello, i campi jumper/collegamento di rete/unità di impronta, il carattere `face` e gli atomi di impronta di impronta incompatibili con versioni precedenti. Convertire le proprietà dell'impronta PCB in `fp_text`, rinominare `uuid`/`id` in `tstamp`/`id` moduli legacy, rinominare pasta saldante e campi termici, convertire tratti in legacy `width`, convertire dimensioni in grafica visibile, downgrade di atomi booleani/presenza e ricostruzione di netcode numerici. |
| File laterali del progetto | Genera impostazioni di visualizzazione dell'ID numerico legacy `.kicad_prl` per le schede V7. |

### Rilevamento dei documenti e gestione dei progetti

L'implementazione C++ rileva il tipo di documento KiCad principalmente dalla radice
Testa di espressione S:

| Testa della radice | Tipo |
| --- | --- |
| `kicad_symbol_lib` | Libreria di simboli |
| `kicad_sch` | Schematico |
| `kicad_pcb` | Asse |
| `footprint` | Orma |
| `kicad_dru` | Regole di progettazione |
| `kicad_wks`, `drawing_sheet` | Foglio di lavoro |

Se l'intestazione della radice è mancante o sconosciuta, si ricorre all'estensione del file:
`.kicad_sym`, `.kicad_sch`, `.kicad_pcb`, `.kicad_mod`, `.kicad_dru` e
`.kicad_wks`. Legacy `.sch`, `.lib`, `.dcm`, and `.pro` are also detected as
tipi legacy di KiCad, ma la conversione diretta da quelle famiglie di file legacy non lo è
implementato nella fase attuale.

Quando si converte una directory di progetto o `.kicad_pro`, copia solo quelli modificabili
Input di progetto KiCad e file di modello 3D locali comuni. Output generati,
cartelle di cronologia/backup, Gerber, output di fabbricazione, distinte materiali e file temporanei
vengono saltati. Per le destinazioni scheda KiCad 6, 7 e 8 crea anche legacy
`.kicad_prl` impostazioni visualizzazione scheda locale con numerico `visible_items`, completo
`visible_layers` e la vecchia versione meta delle impostazioni locali hanno quindi convertito oggetti
rimangono visibili nelle GUI precedenti. Per KiCad 6 il progetto lo indirizza ulteriormente
rimuove i nodi `version` di livello superiore da `sym-lib-table` / `fp-lib-table` e
ricostruisce le tabelle delle istanze della gerarchia schematica a livello di root sui fogli secondari.

### Regole della libreria di simboli

Le porte parser generiche rimuovono questi nodi introdotti quando viene creato il formato del file di destinazione
è precedente alla versione introduttiva:

| Introdotto | Teste rimosse | Motivo |
| ---: | --- | --- |
| 20220126 | `text_box`, `textbox` | Caselle di testo dei simboli |
| 20240529 | `embedded_files`, `embedded_file` | File incorporati |
| 20241209 | `private` | Flag SCH_FIELD privati |
| 20250324 | `pin_group`, `pin_groups` | Gruppi di pin dei ponticelli |
| 20250829 | `rounded_rectangle`, `roundrect` | Rettangoli arrotondati |
| 20260508 | `ellipse`, `ellipse_arc` | Primitive dell'ellisse nativa |

La compatibilità riscrive:

| Taglio del bersaglio | Riscrivere |
| ---: | --- |
| `< 20231120` | Rimuovi i campi root `generator_version` |
| `< 20241209` | Rimuovi `embedded_fonts`; aggiungere ID proprietà legacy; sposta i flag della proprietà `hide` in `effects` |
| `< 20230409` | Rimuovi i flag di esclusione della simulazione della libreria di simboli `symbol/exclude_from_sim` |
| `< 20240108` | Converti elenchi di caratteri `(bold yes/no)` e `(italic yes/no)` in atomi di presenza legacy |
| `<= 20241209` | Rimuovi i campi `face` del carattere |
| `< 20241004` | Converti elenchi booleani `hide` in atomi legacy; appiattisci `pin_names` / `pin_numbers` nascondi elenchi |
| `<= 20211014` | Aggiungi gli ID proprietà standard di KiCad 6: `Reference=0`, `Value=1`, `Footprint=2`, `Datasheet=3`, `ki_keywords=4`, `ki_description=5`, `ki_fp_filters=6` |
| `< 20251024` | Rimuovi il simbolo `in_pos_files`; rimuovi la proprietà `show_name` e `do_not_autoplace` |
| `< 20250324` | Rimuovi `duplicate_pin_numbers_are_jumpers` |
| `< 20250227` | Rimuovi i flag di classe del simbolo `power` |

### Regole schematiche

Porte parser generiche:

| Introdotto | Teste rimosse | Motivo |
| ---: | --- | --- |
| 20220126 | `text_box`, `textbox` | Caselle di testo schematiche |
| 20220622 | `simulation_model`, `sim_model` | Nuovo formato del modello di simulazione |
| 20240101 | `table` | Tavole schematiche |
| 20240417 | `rule_area` | Aree di regole schematiche |
| 20240620 | `embedded_files`, `embedded_file` | File incorporati |
| 20241209 | `private` | Flag SCH_FIELD privati |
| 20250829 | `rounded_rectangle`, `roundrect` | Rettangoli arrotondati |
| 20250922 | `variants`, `variant` | Varianti schematiche |
| 20260508 | `ellipse`, `ellipse_arc` | Primitive dell'ellisse nativa |
| 20260512 | `net_chain`, `net_chains` | Catene nette schematiche |

La compatibilità riscrive:

| Taglio del bersaglio | Riscrivere |
| ---: | --- |
| `< 20231120` | Rimuovi `generator_version`; rimuovere `fields_autoplaced` da simboli e fogli |
| `< 20260326` | Rimuovere i campi `locked` dello schema introdotti dopo la destinazione |
| `< 20260306` | Rimuovi `embedded_fonts`; rimuovi foglio `exclude_from_sim`, `in_bom`, `on_board`, `dnp`; rimuovere i nodi `group` dello schema root |
| `< 20250827` | Rimuovi `body_styles` e `body_style` |
| `< 20250114` | Rimuovi testo/casella di testo `exclude_from_sim` |
| `<= 20230121` | Rimuovi tutti i restanti `exclude_from_sim` |
| `< 20220822` | Rimuovere i campi testo, etichetta e etichetta direttiva `hyperlink` |
| `< 20220914` | Rimuovi i flag `dnp` del simbolo posizionato |
| `< 20220124` | Rinomina i nodi root `directive_label` in `netclass_flag` |
| `< 20251024` | Rimuovi simbolo `in_pos_files` |
| `< 20250324` | Rimuovi `duplicate_pin_numbers_are_jumpers` |
| `< 20250227` | Rimuovi i flag di classe del simbolo `power` |
| `< 20241004` | Converti elenchi booleani `hide` in atomi legacy; appiattisci la visibilità dei pin nascondi gli elenchi |
| `<= 20211123` | Rimuove le definizioni `pin/alternate` dei simboli di libreria |
| `< 20240108` | Converti elenchi booleani di caratteri in grassetto/corsivo in atomi legacy |
| `<= 20250114` | Rimuovi i campi `face` del carattere |
| `<= 20230121` | Togli gli atomi `uuid` per i parser KiCad 6/7 |
| `<= 20211123` | Genera `sheet_instances` e `symbol_instances` a livello root di KiCad 6 quando lo schema root di origine contiene già dati di istanza; ai fogli secondari non vengono fornite tabelle di istanza root |
| `<= 20211123` | Aggiungi gli ID delle proprietà dello schema standard di KiCad 6 e normalizza i nomi/ID delle proprietà del foglio in `Sheet name=0` e `Sheet file=1` |
| `<= 20211123` | Rimuovere i blocchi `instances` interni al simbolo dopo che la tabella dell'istanza root di KiCad 6 è stata generata |
| `< 20241209` | Aggiungi ID proprietà legacy; sposta i flag della proprietà `hide` in `effects` |
| `< 20251028` | Rimuovi proprietà `show_name` e `do_not_autoplace` |

### Regole del tabellone e dell'impronta

Porte parser generiche:

| Introdotto | Teste rimosse | Motivo |
| ---: | --- | --- |
| 20220131 | `gr_text_box`, `fp_text_box`, `text_box`, `textbox` | Caselle di testo PCB |
| 20220621 | `image` | Oggetti immagine PCB |
| 20220818 | `net_tie`, `net_ties` | Stoccaggio in rete di prima classe |
| 20231007 | `generated` | Oggetti generativi |
| 20240108 | `teardrop`, `teardrops` | Parametri a goccia |
| 20240202 | `table` | Tabelle PCB |
| 20240609 | `tenting` | Parola chiave tenda |
| 20240706 | `embedded_files`, `embedded_file`, `embedded_fonts` | File incorporati |
| 20240928 | `component_class`, `component_classes` | Classi dei componenti |
| 20240929 | `padstack` | Padstack complessi |
| 20241006 | `via_stack`, `viastack` | Tramite pile |
| 20241009 | `rule_area` | Aree di posizionamento/regole |
| 20250228 | `via_protection`, `covering`, `plugging`, `filling`, `capping` | IPC-4761 tramite protezione |
| 20250818 | `custom_layer_count`, `custom_layer_counts` | Conteggi dei livelli di impronta personalizzati |
| 20250829 | `rounded_rectangle`, `roundrect` | Rettangoli arrotondati |
| 20250901 | `point` | Oggetti punto PCB |
| 20250914 | `barcode`, `pcb_barcode`, `gr_barcode`, `fp_barcode` | Oggetti codice a barre PCB |
| 20251101 | `backdrill`, `tertiary_drill`, `front_post_machining`, `back_post_machining` | Campi di perforazione backdrill e terziario |
| 20260101 | `variants`, `variant` | Varianti del circuito stampato |
| 20260410 | `extruded` | Modelli di corpi 3D con impronte estruse |
| 20260508 | `gr_ellipse`, `gr_ellipse_arc`, `fp_ellipse`, `fp_ellipse_arc` | Primitive ellittiche native del PCB |
| 20260511 | `spec_frequency`, `dielectric_model` | Campi di stackup dipendenti dalla frequenza dielettrica |
| 20260512 | `net_chains`, `net_chain` | Catene a rete PCB |
| 20260513 | `thieving` | Modalità riempimento zona ladro rame |

Note sulla copertura dello sviluppo attuale da KiCad locale `10.99.0-1273-gd90e32b6a0`:

| Introdotto | Gestione | Note |
| ---: | --- | --- |
| 20260521 | Implementato | Il pad secondario `sim_electrical_type` viene rimosso per i target più vecchi di `20260521`. |
| 20260603 | Implementato | Il figlio della cella di tabella `knockout` viene rimosso contestualmente per le destinazioni precedenti a `20260603`; `knockout` non viene utilizzato come token gate globale perché viene utilizzato anche da altri tipi di oggetto. |

La compatibilità riscrive:

| Taglio del bersaglio | Riscrivere |
| ---: | --- |
| `< 20260410` | Rimuovi i blocchi del modello 3D digitati/estrusi rimuovendo i nodi `model` che contengono `type` |
| `< 20260513` | Sostituisci la modalità di riempimento della zona di furto di rame con il riempimento poligonale |
| `>= 20220225` | Rimuovi i campi `tedit` dell'impronta obsoleti |
| `>= 20200628` | Rimuovi le impostazioni `visible_elements` della scheda obsoleta |
| `< 20260603` | Rimuovere i campi `knockout` della cella della tabella PCB |
| `< 20240703` | Converti i qualificatori di tipo livello utente `front`, `back`, `auxiliary` in `user` |
| `< 20241010` | Rimuovere i campi grafici `solder_mask_margin` |
| `< 20241030` | Converti i campi booleani delle dimensioni in atomi legacy; rimuovi dimensione `arrow_direction` |
| `< 20250210` | Rimuovi testo PCB `render_cache`; rimuovi la casella di testo `knockout`; rimuovere gli atomi `knockout` dagli elenchi dei livelli; aggiungi `filled_areas_thickness no` ai riempimenti della zona memorizzata nella cache dove necessario |
| `< 20241009` | Rimuovi i campi `placement` della zona |
| `<= 20221018` | Rimuovi zona `attr`; rimuovi pad/zona `thermal_bridge_angle`; rinomina pad/zona `thermal_bridge_width` in legacy `thermal_width` |
| `< 20240108` | Rimuovi `setup/allow_soldermask_bridges_in_footprints`; rimuovi il gruppo `locked`; rimuovere tramite campi di connessione livello come `keep_end_layers`, `start_end_only` e `zone_layer_connections` |
| `< 20241007` | Rimuovi i campi `solder_mask_margin` e `solder_mask_layer` della traccia |
| `< 20240617` | Rimuovi la cella della tabella PCB `angle` |
| `< 20260521` | Rimuovi tampone `sim_electrical_type` |
| `< 20250228` | Converti elenchi booleani anteriori/posteriori in atomi legacy; rimuovere i campi di protezione IPC-4761 |
| `< 20231212` | Converti elenchi booleani `locked` e `hide` in atomi di presenza; rimuovi `unlocked`; rimuovi modello `hide` |
| `< 20231014` | Rimuovi `generator_version` |
| `< 20230924` | Converti `pcbplotparams` `yes/no` booleani in `true/false`; converti il ​​riempimento della forma `no` in `none` |
| `< 20230730` | Rimuovi la connettività della forma grafica `net` |
| `< 20240108` | Converti elenchi booleani di caratteri in grassetto/corsivo in atomi legacy |
| `< 20230620` | Converti le proprietà `Reference` e `Value` dell'impronta in `fp_text`; converti `Description` in `ki_description`; mappa `sheetname`/`sheetfile` alle proprietà |
| `< 20231231` | Rinominare i campi con ambito `uuid` in `tstamp`; rinomina il gruppo/generato `uuid` torna a `id` |
| `< 20250324` | Rimuovi i campi del jumper pad dell'impronta: `duplicate_pad_numbers_are_jumpers` e `jumper_pad_groups` |
| `<= 20221018` | Rimuovi gli attributi `dnp` dell'impronta, `net_tie_pad_groups`, `units` e `allow_missing_courtyard`; rimuovi pad/tramite `remove_unused_layers`; convertire le dimensioni in grafica visibile; rimuovi `locked` incompatibile con la versione precedente; downgrade gratuito tramite campi; convertire i blocchi grafici `stroke` PCB in campi `width` legacy |
| `< 20250309` | Rimuovi `component_class` dalle regole di posizionamento |
| `< 20250222` | Converte i riempimenti a forma di tratteggio/tratteggio invertito/tratteggio incrociato PCB in riempimento solido |
| `<= 20241229` | Rimuovere i campi `face` del carattere PCB |
| `< 20251101` | Rimuovere il tampone/tramite campi post-lavorazione |
| `< 20251028` | Ricostruisci i codici di rete della scheda numerica legacy e le dichiarazioni di rete a livello di root |

Correzioni specifiche del parser di KiCad 6 osservate nei test a livello di progetto:

| Zona | Correzione implementata |
| --- | --- |
| Configurazione del PCB | Rimuovi `setup/allow_soldermask_bridges_in_footprints` per i target scheda pre-8. |
| Impronte del PCB | Rimuovere `net_tie_pad_groups`, `units`, gruppi di jumper pad e `allow_missing_courtyard` atom attr per le destinazioni della scheda KiCad 6/7. |
| Zone e pad PCB | Rimuovi la zona `attr`, rimuovi `thermal_bridge_angle` e rinomina `thermal_bridge_width` in `thermal_width` per le destinazioni della scheda KiCad 6/7. |
| Testi e tabelle PCB | Rimuovi il testo `render_cache`, la casella di testo `knockout`, la cella della tabella `knockout` e l'elenco dei livelli `knockout` dove i parser più vecchi li rifiutano. |
| Librerie di simboli | Rimuovi il simbolo `exclude_from_sim` per le destinazioni precedenti a `20230409` e aggiungi gli ID proprietà standard di KiCad 6. |
| Schemi | Rimuovi il pin `alternate`, genera tabelle di istanze root di KiCad 6, normalizza i percorsi delle istanze del foglio root, normalizza i nomi/ID delle proprietà del foglio e rimuovi `instances` interno al simbolo. I blocchi UUID dei pin dei simboli posizionati vengono mantenuti intenzionalmente perché KiCad 6 li usa per l'associazione delle istanze. |
| File laterali del progetto | Genera le impostazioni di visualizzazione dell'ID numerico `.kicad_prl` per V6/V7/V8 e rimuovi i nodi `version` di livello superiore della tabella della libreria per V6. |

### Foglio di lavoro e regole di progettazione

La gestione del foglio di lavoro attualmente ha un cancello parser implementato:

| Taglio del bersaglio | Riscrivere |
| ---: | --- |
| `< 20220228` | Rimuovi i blocchi del foglio di lavoro `font` |

Le regole di progettazione vengono rilevate e hanno alias della versione di destinazione, ma nessun downgrade
le riscritture sono attualmente implementate perché rimane la macro della versione del formato file
`20200610` nelle versioni di KiCad tracciate.

### Semantica di avvisi e report

Ogni rimozione implementata o riscrittura della compatibilità che modifica l'albero aggiunge un file
avvertimento. Le porte delle funzionalità generiche riportano il numero di nodi rimossi e il file
versione introduttiva. I rapporti includono percorso, tipo rilevato, versione di origine,
versione di destinazione, flag modificato e avvisi.

## Requisiti del convertitore

### Leggi il percorso

- Conserva il file sorgente `version`; non interpretare solo come corrente
Formato KiCad.
- Supporta alias di compatibilità per i file più vecchi:
  - da `page` a `paper`
  - Barra precedente precedente da `~...~` a `~{...}`
  - Dal vecchio formato della casella di testo `start/end` al nuovo `at/size`
  - Vecchio `id` a `uuid`
  - Vecchi formati booleani/token di presenza in booleani espliciti
- Rileva formati futuri e restituisce un errore chiaro o una strategia di downgrade definita.

### Scrivi percorso

- `--target-version` deve fare di più che modificare il numero di versione di livello superiore. Esso
deve eliminare o riscrivere la semantica in base all'obiettivo richiesto.
- Ogni versione di destinazione necessita di funzionalità:
  - KiCad 6 non deve scrivere campi del modello di simulazione V7, DNP o caselle di testo successive alla V6
strutture.
  - KiCad 7 non deve scrivere strutture che sono diventate stabili solo dopo la V8
`generator_version` pulizia.
  - KiCad 8 non deve scrivere file incorporati V9, classi di componenti o file complessi
pastiglie.
  - KiCad 9 non deve scrivere varianti V10, codici a barre, backdrill, suddivisione per tipo e
costrutti simili.
  - KiCad 10 non deve scrivere i metadati del corpo estruso dello sviluppo corrente, nativi
ellissi, campi di frequenza dielettrica, catene reticolari, ladro di rame, pad
tipi elettrici di simulazione o flag di eliminazione delle celle della tabella.
- I downgrade con perdita dovrebbero produrre avvisi o metadati collaterali anziché silenziosi
cancellazione.

### Percorso di prova

- Costruisci dispositivi minimi per KiCad 6, 7, 8, 9 e 10:
  - Libreria di simboli
  - Schematico
  - Asse
  - Orma
  - Foglio di lavoro
  - Regole di progettazione
- Ogni conversione tra versioni incrociate dovrebbe verificare:
  - La versione di origine viene letta correttamente
  - Il numero di versione di destinazione è scritto correttamente
  - I token non consentiti vengono rimossi o declassati
  - La semantica chiave viene preservata
  - Gli avvisi riguardano le conversioni con perdita

## Note sulla manutenzione

Quando si aggiungono differenze di versioni future:

1. Aggiungi o aggiorna prima la matrice della versione.
2. Aggiungi una nuova sezione di intervallo come `10.0 to 11.0` o
   `10.99 / current to 11.99 / current`.
3. Mantieni i risultati del ramo di sviluppo separati dai tag stable rilasciati fino al
la versione corrispondente di KiCad è contrassegnata.
4. Aggiorna il riepilogo della destinazione del backport quando viene introdotta una nuova versione di origine
costrutti che influenzano gli obiettivi di downgrade esistenti.
5. Tieni traccia delle migrazioni dello schema JSON `.kicad_pro` in un documento separato.

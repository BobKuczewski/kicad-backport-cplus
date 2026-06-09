# KiCad Backport C++

Implementazione C++17 autonoma della CLI di downgrade di KiCad Backport. Lo strumento
converte i file di progetto KiCad S-expression più recenti nei formati di file KiCad precedenti
preferendo la sintassi legacy equivalente rispetto alla cancellazione.

## Documentazione localizzata

- [简体中文](docs/README.zh-CN.md)
- [日本語](docs/README.ja-JP.md)
- [한국어](docs/README.ko-KR.md)
- [Francese](docs/README.fr-FR.md)
- [Deutsch](docs/README.de-DE.md)
- [Spagnolo](docs/README.es-ES.md)
- [Italiano](docs/README.it-IT.md)

Ulteriori riferimenti:

- [Differenze di versione del formato del file KiCad](docs/kicad-file-format-version-differences.md)
- [Differenze di versione del formato del file localizzato](docs/README.md#kicad-file-format-version-differences)

## Comandi

L'interfaccia della riga di comando rispecchia l'implementazione Go ed è destinata a esserlo
utilizzabile sia direttamente che dal plugin Python:

```text
kicad-backport convert --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

Il convertitore legge i file KiCad S-expression e applica il downgrade basato sulla versione
regole, scrive un percorso di output con il suffisso della versione e può copiare l'intero progetto KiCad
directory prima di normalizzare tutti i file KiCad nella copia. Durante la conversione,
stampa la versione sorgente rilevata e la versione target risolta per ogni file KiCad.
L'output testuale preferisce alias KiCad come `9.0 (20241229)` o
`10.99-dev (20260513)` invece dei numeri grezzi del formato file.
`detect-versions` è una scansione rapida delle directory che legge solo il testo
necessario per riportare tipi e versioni dei file KiCad. L'output testuale usa gli
stessi alias, mentre i report JSON mantengono le versioni grezze. Prima filtra per
estensioni KiCad supportate e omette i file di cui non può identificare la versione.

Esempi:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions E:\tmp\project
```

Gli alias di versione supportati sono `4.0`, `5.0`, `5.1`, `6.0`, `7.0`,
`8.0`, `9.0`, `10.0` e `10.99`. Una versione grezza del formato file KiCad può essere
passata anche per testare uno specifico limite del parser.

## Stato del supporto

L'attuale implementazione è destinata alle famiglie di file KiCad 4 fino a KiCad 10:

| Bersaglio | Stato |
| --- | --- |
| KiCad 10 | Rimuove la sintassi post-10.0/sviluppo corrente, inclusi 20260521 pad `sim_electrical_type` e 20260603 cella di tabella `knockout`. |
| KiCad 10.99 | Target di sviluppo corrente per board/footprint: scrive la versione `20260603`; symbol library e schematic usano ancora le versioni target KiCad 10 (`20251024` / `20260306`). |
| KiCad 9 | Rimuove o esegue il downgrade delle funzionalità di KiCad 10/attuale come varianti, codici a barre, backdrill/post-lavorazione, jumper pad e omissione del netcode. |
| KiCad8 | Rimuove le tabelle di KiCad 9+, i file incorporati, le classi dei componenti, i padstack, gli stack via, le aree delle regole e i moduli arbitrari del livello utente. |
| KiCad7 | Applica le riscritture di compatibilità del parser precedenti per moduli UUID/tstamp, campi di impronte PCB, lacrime, oggetti generati, immagini e caselle di testo. |
| KiCad 6 | Il supporto per il downgrade dei file di base è in gran parte completo. I progetti di test convertiti sono stati aperti manualmente nell'effettiva applicazione KiCad 6 per la convalida. |
| KiCad 5 | Supporta la versione target board/footprint `20171130` e l'import/export di base dei file legacy `.sch`, `.lib`, `.dcm` e `.pro`. Oggetti schematici dettagliati, primitive grafiche dei simboli e pin restano conversioni con perdita e vengono segnalati con avvisi. |
| KiCad 4 | Supporta la versione target board/footprint `4`, la riscrittura degli header legacy V4 per schemi/librerie e suffissi/estensioni di output V4. Le funzionalità PCB solo V5, come i pad personalizzati, vengono semplificate quando possibile. |

## Politica di downgrade

Il convertitore applica la rappresentazione più compatibile disponibile nel file
formato di destinazione:

- Quando possibile, i nuovi oggetti o campi vengono mappati alla sintassi equivalente precedente.
- Le informazioni visibili/di produzione vengono conservate laddove il vecchio formato può esprimerle.
- La sintassi non supportata viene rimossa solo quando i parser KiCad più vecchi non riescono a caricarla o
il formato file precedente non ha una rappresentazione equivalente.
- Ogni rimozione o riscrittura della compatibilità viene segnalata come avviso.

Ad esempio, i codici netti legacy vengono ricostruiti per i vecchi formati PCB, booleani più recenti
i moduli di campo vengono convertiti in atomi di presenza dove necessario, KiCad 7 PCB
le dimensioni vengono conservate come grafica visibile e scheda locale del progetto legacy
i file di visibilità vengono generati per le destinazioni KiCad 6/7/8.

Quando si converte una directory di progetto o `.kicad_pro`, lo strumento copia solo
input KiCad modificabili e file di modelli 3D locali comuni. Produzione generata
output, cartelle di cronologia/backup, Gerber, distinte materiali e file temporanei vengono ignorati.
Quando si attraversa il confine KiCad 5/6, le estensioni cambiano automaticamente dove necessario,
ad esempio `.sch -> .kicad_sch`, `.lib -> .kicad_sym`, `.kicad_sch -> .sch`,
`.kicad_sym -> .lib/.dcm` e `.kicad_pro -> .pro`.

## Disposizione del progetto

Il codice è suddiviso per responsabilità in modo che sia possibile aggiungere versioni successive di KiCad
piccoli cambiamenti localizzati:

- `src/kicad_backport.cpp`: flusso CLI, copia/filtro del progetto, conversione file.
- `src/kicad_backport_document.cpp`: rilevamento del tipo di documento KiCad.
- `src/kicad_backport_legacy.cpp`: helper di lettura/scrittura per file KiCad legacy `.sch`, `.lib`, `.dcm` e `.pro`.
- `src/kicad_backport_pathmap.cpp`: aiutanti per la mappatura dell'estensione del file di destinazione.
- `src/kicad_backport_report.cpp`: formattazione del report JSON.
- `src/kicad_backport_rules.cpp`: gate di versione e ordine delle regole di downgrade.
- `src/kicad_backport_rule_rewriters.cpp`: helper per la riscrittura dell'albero delle espressioni S.
- `src/kicad_backport_upgrade.cpp`: normalizzazione della sintassi limitata per i file sorgente più vecchi.
- `src/kicad_backport_versions.cpp`: alias di rilascio di KiCad e versioni del formato.
- `src/kicad_backport_util.cpp`: stringhe condivise, file e helper JSON.
- `src/sexpr.cpp`: parser/formattatore minimale di espressioni S in stile KiCad.
- `src/internal/`: intestazioni di implementazione private utilizzate solo dai file di origine.
- `include/kicad_backport/`: intestazioni di progetto pubbliche utilizzate dall'eseguibile.

Le regole di downgrade ad azione singola utilizzano un piccolo helper `applyWhen()` invece di
`std::function`, mantenendo le regole compatte senza aggiungere allocazioni heap.
Le regole multi-azione rimangono raggruppate quando si ordinano le questioni.

La struttura di primo livello è volutamente semplice:

```text
kicad-backport-cplus/
  include/kicad_backport/   public headers
  src/                      implementation files
  src/internal/             private headers
  build/                    generated build trees
  dist/                     packaged command-line binaries
```

## Costruire

There are two simple direct build entrypoints:

- `build.ps1` for Windows native MinGW/g++ builds.
- `build.sh` for native Linux, Raspberry Pi, and macOS builds.

Native Linux/RPi/macOS build:

```sh
git clone <repo-url> kicad-backport-cplus
cd kicad-backport-cplus
./build.sh --config Release
```

Windows native MinGW/g++ build:

```powershell
git clone <repo-url> kicad-backport-cplus
cd kicad-backport-cplus
.\build.ps1
```

Useful native options:

```sh
./build.sh --clean
./build.sh --compiler g++-8
./build.sh --static-runtime off
```

Outputs are copied to `dist/`. The current source requires C++17 support for
newer standard-library filesystem, view-string, PMR, and memory-resource facilities; it uses a small project-owned path/directory API plus `std::string`. Direct builds fall back from `-std=c++17` to
`-std=c++1z` when needed. Direct builds also probe for supported section garbage collection and symbol stripping flags, enabling them only when the active toolchain accepts them.

Manual direct GCC build:

```sh
./build.sh --config Release --target native
```

## Ringraziamenti

Un ringraziamento speciale a Hubert per l'aiuto fornito durante lo sviluppo di questo
progetto.

## Validazione

Dopo la conversione, convalida ogni destinazione con la versione KiCad corrispondente. Per
KiCad 8/9/10 questo di solito significa eseguire lo schema ERC e PCB DRC:

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

La CLI di KiCad 7 ha un set di comandi più piccolo, quindi usa netlist e Gerber Export to
verificare che i file dello schema convertito e del PCB vengano caricati:

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

KiCad 6 ha una copertura limitata della convalida CLI. Per i file PCB, un rapido controllo del parser
può essere fatto tramite il modulo Python di KiCad 6:

```powershell
& 'D:\KiCad\6.0\bin\python.exe' -c "import pcbnew; pcbnew.LoadBoard(r'E:\tmp\project_V6\project.kicad_pcb'); print('pcb ok')"
```

Per gli schemi e i simboli di KiCad 6, l'apertura manuale della GUI rimane la più utile
convalida end-to-end. Gli attuali campioni di regressione V6 sono stati controllati in questo modo.

Le violazioni ERC/DRC sono risultati delle regole di progettazione del progetto. Non lo sono
errori di conversione del formato a meno che KiCad non riporti un errore di caricamento o di analisi.

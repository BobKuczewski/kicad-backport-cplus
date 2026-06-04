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
kicad-backport convert --target-version <6.0|7.0|8.0|9.0|10.0|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

Il convertitore legge i file KiCad S-expression e applica il downgrade basato sulla versione
regole, scrive un percorso di output con il suffisso della versione e può copiare l'intero progetto KiCad
directory prima di normalizzare tutti i file KiCad nella copia.

Esempi:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

Gli alias di versione supportati sono `6.0`, `7.0`, `8.0`, `9.0` e `10.0`. Un crudo
Il numero di versione del formato file KiCad può anche essere passato durante il test di uno specifico
taglio del parser.

## Stato del supporto

L'attuale implementazione è destinata al file S-expression da KiCad 6 a KiCad 10
famiglie:

| Bersaglio | Stato |
| --- | --- |
| KiCad 10 | Rimuove la sintassi post-10.0/sviluppo corrente, inclusi 20260521 pad `sim_electrical_type` e 20260603 cella di tabella `knockout`. |
| KiCad 9 | Rimuove o esegue il downgrade delle funzionalità di KiCad 10/attuale come varianti, codici a barre, backdrill/post-lavorazione, jumper pad e omissione del netcode. |
| KiCad8 | Rimuove le tabelle di KiCad 9+, i file incorporati, le classi dei componenti, i padstack, gli stack via, le aree delle regole e i moduli arbitrari del livello utente. |
| KiCad7 | Applica le riscritture di compatibilità del parser precedenti per moduli UUID/tstamp, campi di impronte PCB, lacrime, oggetti generati, immagini e caselle di testo. |
| KiCad 6 | Il supporto per il downgrade dei file di base è in gran parte completo. I progetti di test convertiti sono stati aperti manualmente nell'effettiva applicazione KiCad 6 per la convalida. |
| KiCad 5 e versioni precedenti | Non ancora implementato. Il codice ora separa il rilevamento dei documenti legacy, la mappatura dei percorsi e le regole di upgrade/downgrade per prepararsi al futuro supporto V5. |

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

## Disposizione del progetto

Il codice è suddiviso per responsabilità in modo che sia possibile aggiungere versioni successive di KiCad
piccoli cambiamenti localizzati:

- `src/kicad_backport.cpp`: flusso CLI, copia/filtro del progetto, conversione file.
- `src/kicad_backport_document.cpp`: rilevamento del tipo di documento KiCad.
- `src/kicad_backport_legacy.cpp`: impalcatura di caricamento dei documenti KiCad legacy.
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
  scripts/                  cross-build environment setup
  build/                    generated build trees
  dist/                     packaged command-line binaries
```

## Costruire

Costruisci sulla piattaforma attuale:

```powershell
.\build.ps1
```

```sh
./build.sh
```

Per rilevare e installare automaticamente la più piccola toolchain pratica prima
edificio:

```powershell
.\build.ps1 -SetupMissingTools
```

```sh
./build.sh --setup
```

Entrambi gli script provano le destinazioni di rilascio standard e vi copiano gli output riusciti
`dist/` utilizzando nomi compatibili con il plugin:

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

Utilizza `.\build.ps1 -Clean` o `./build.sh --clean` per rimuovere la build precedente
output prima della ricostruzione.

La compilazione incrociata C++ richiede toolchain della piattaforma. Su Windows, `build.ps1`
costruisce `windows-amd64` e `windows-arm64` con Visual Studio e costruisce
`linux-amd64`/`linux-arm64` tramite WSL quando la toolchain WSL è disponibile.
Su Linux, `build.sh` crea Linux nativo e può creare `linux-arm64` quando
`aarch64-linux-gnu-g++` è installato. Su macOS, `build.sh` crea Darwin
amd64/arm64 con l'SDK di Apple. I file binari di Darwin devono essere generati su macOS.

Per creare un sottoinsieme:

```powershell
.\build.ps1 -Targets windows-amd64,windows-arm64
```

```sh
TARGETS="linux-amd64 linux-arm64" ./build.sh
```

Configurazione dell'ambiente tra build:

```powershell
.\scripts\setup-cross.ps1
.\scripts\setup-cross.ps1 -CheckOnly
```

```sh
./scripts/setup-cross.sh
./scripts/setup-cross.sh --check-only
```

Gli script di installazione installano automaticamente la più piccola toolchain di compilazione pratica
per la piattaforma ospitante. Utilizza `-CheckOnly` o `--check-only` per segnalare solo quelli mancanti
strumenti senza installare nulla.

In Windows, lo script di installazione installa o prepara CMake, Visual Studio C++
Strumenti di creazione, WSL, Ubuntu e i pacchetti WSL minimi necessari per Linux
build amd64/arm64. Su Linux installa CMake, un compilatore C++ nativo, Ninja,
e il compilatore incrociato Linux aarch64 era supportato dal pacchetto host
manager. Su macOS, attiva gli strumenti da riga di comando di Apple e installa CMake/Ninja
tramite Homebrew quando disponibile.

Compilazione manuale di CMake:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

L'implementazione è intenzionalmente priva di dipendenze e segue il C++ in stile KiCad
convenzioni di formattazione.

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

# KiCad Backport C++

KiCad Backport C++ è un'implementazione autonoma in C++17 della CLI di downgrade KiCad Backport. Converte file di progetto KiCad recenti in formati KiCad più vecchi, preferendo una rappresentazione equivalente nel formato di destinazione invece della semplice rimozione dei dati.

## Comandi

```text
kicad-backport convert --target-version <7.0|8.0|9.0|10.0|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

Esempi:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

Gli alias supportati sono `7.0`, `8.0`, `9.0` e `10.0`. È anche possibile passare un numero di versione grezzo del formato KiCad per testare un punto specifico del parser.

## Politica di downgrade

- I nuovi oggetti o campi vengono convertiti in sintassi equivalente più vecchia quando possibile.
- Le informazioni visibili e di produzione vengono conservate se il vecchio formato può rappresentarle.
- La sintassi non supportata viene rimossa solo quando KiCad precedente non può caricarla o quando non esiste una rappresentazione equivalente.
- Ogni rimozione o riscrittura di compatibilità viene segnalata come warning.

Durante la conversione di una cartella di progetto vengono copiati solo i file correlati a KiCad e i modelli 3D locali comuni. Directory `.history`, backup, Gerber, BOM, PDF, README e altri file non correlati non vengono copiati.

## Build

Windows:

```powershell
.\build.ps1
.\build.ps1 -SetupMissingTools
```

Linux/macOS:

```sh
./build.sh
./build.sh --setup
```

I binari vengono generati in `dist/`.

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

Su Windows, `build.ps1` compila Windows amd64/arm64 con Visual Studio e Linux amd64/arm64 tramite WSL quando la toolchain è disponibile. I binari Darwin richiedono macOS e Apple SDK.

## Validazione

Dopo la conversione, verificare con la versione corrispondente di KiCad CLI. Per KiCad 8/9/10 di solito si eseguono ERC e DRC:

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 ha meno comandi CLI; usare l'esportazione netlist e Gerber per confermare che i file vengano caricati:

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

Le violazioni ERC/DRC sono risultati delle regole di progetto. Non indicano un errore di conversione, a meno che KiCad non segnali un errore di caricamento o parsing.

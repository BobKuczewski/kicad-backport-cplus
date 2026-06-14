# KiCad Backport C++

`kicad-backport` è un convertitore command-line standalone implementato in C++ portabile per spostare progetti e file KiCad verso target di formato più vecchi. Mira alla compatibilità dei parser: quando una versione KiCad vecchia ha una rappresentazione equivalente, riscrive verso quella rappresentazione; altrimenti rimuove o approssima la sintassi non supportata e la segnala con warning.

L’implementazione è dependency-free, include un piccolo parser/formatter S-expression in stile KiCad e può essere usata direttamente o da un plugin wrapper.

## Documentazione

- [Indice documentazione](README.md)
- [Differenze di formato del convertitore](kicad-backport-converter-format-differences.it-IT.md)
- [README inglese](../README.md)

## Comandi

```text
kicad-backport convert [--quiet] --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

Esempio:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions --json E:\tmp\project
```

`convert` accetta un documento KiCad o una directory di progetto. Un `.kicad_pro` converte la directory del progetto che lo contiene. L’output riceve un suffisso target come `project_V9`.

`inspect` riporta tipo e versione rilevati. `detect-versions` esegue uno scan leggero e può emettere JSON.

## Target supportati

| Target | Comportamento attuale |
| --- | --- |
| KiCad 10.99 | Alias di sviluppo. Board/footprint scrivono `20260603`; schematic/symbol-library restano sugli anchor KiCad 10. |
| KiCad 10 | Rimuove o riscrive sintassi di sviluppo fuori dagli anchor `10.0`. |
| KiCad 9 | Rimuove o downgrade variants, barcodes, backdrill/post-machining, jumper pad metadata e riferimenti board solo net name. |
| KiCad 8 | Rimuove o riscrive KiCad 9+ tables, embedded files/fonts, component classes, padstacks, via stacks, rule/placement areas, user-layer type qualifiers e font face fields. |
| KiCad 7 | Applica compatibilità per UUID/tstamp, PCB footprint fields, teardrops, generated objects, images, text boxes e stroke/dimension syntax. |
| KiCad 6 | Target della prima famiglia moderna schematic/symbol/project con strutture di compatibilità necessarie. |
| KiCad 5.0/5.1 | Board/footprint usano `20171130`; schematic, symbol-library e project scrivono legacy `.sch`, `.lib/.dcm`, `.pro`. |
| KiCad 4 | Board/footprint usano `4`; header legacy V4 sono riscritti e construct PCB KiCad 5+ sono semplificati quando possibile. |

Vedere le [differenze di formato](kicad-backport-converter-format-differences.it-IT.md).

## Politica di conversione

- Preserva l’intenzione esistente quando il formato target può rappresentarla.
- Riscrive verso sintassi vecchia equivalente quando esiste.
- Rimuove solo ciò che i parser vecchi non possono leggere o che non ha equivalente.
- Riscritture con perdita e rimozioni sono emesse come warning.
- Gli upgrade non creano funzionalità KiCad assenti dalla source.

Il confine KiCad 5/6 cambia famiglie file: `.sch -> .kicad_sch`, `.lib/.dcm -> .kicad_sym`, `.pro -> .kicad_pro`, e inverso `.kicad_sch -> .sch`, `.kicad_sym -> .lib + .dcm`, `.kicad_pro -> .pro`.

## Conversione progetto

Per directory di progetto, il convertitore copia solo input KiCad editabili e comuni modelli 3D locali, poi converte i documenti copiati. Salta output di fabbricazione, backup, history, Gerbers, BOM, directory plot/export e temporanei.

Le riparazioni includono `sym-lib-table` / `fp-lib-table`, `.kicad_prl` per KiCad 6/7/8, durante il downgrade di board/footprint recenti l’estrazione delle risorse 3D embedded in `3D/` e la riscrittura degli URI modello `kicad-embed://...` in `${KIPRJMOD}/3D/...`, simboli locali embedded in `lib_symbols` e ricostruzione delle hierarchy instances.

## Build

```powershell
.\build.ps1
```

```sh
./build.sh
```

Opzioni POSIX utili:

```sh
./build.sh --clean
./build.sh --config Release
./build.sh --compiler g++-8
./build.sh --static-runtime off
./build.sh --zstd off
```

PowerShell accetta anche `-Zstd auto|on|off`. Il default `auto` compila il decompressor zstd vendored quando `src/third_party/zstd` è presente. Usare `off` per toolchain vecchie che non possono compilare quelle sorgenti C; in quel caso le risorse 3D embedded non possono essere estratte e il convertitore emette warning quando deve rimuovere riferimenti embedded model non supportati.

Gli script leggono `kicad_backport_sources.txt`, compilano con `g++` o `clang++` e copiano l’eseguibile in `dist/`. Se necessario, fanno fallback da `-std=c++17` a `-std=c++1z`.

## Validazione

Dopo la conversione, aprire con la versione KiCad target ed eseguire ERC/DRC se applicabile. Controllare i warning prima dell’uso in produzione.

## Ringraziamenti

Un ringraziamento speciale a Hubert per l’aiuto fornito durante lo sviluppo di questo progetto.

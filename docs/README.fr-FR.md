# KiCad Backport C++

KiCad Backport C++ est une implémentation autonome en C++17 de l'outil CLI de rétroportage KiCad Backport. Il convertit les fichiers de projet KiCad récents au format de versions plus anciennes, en privilégiant une représentation équivalente dans l'ancien format plutôt que la suppression.

## Commandes

```text
kicad-backport convert --target-version <7.0|8.0|9.0|10.0|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

Exemples :

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

Les alias pris en charge sont `7.0`, `8.0`, `9.0` et `10.0`. Un numéro brut de version de format KiCad peut aussi être utilisé pour tester une limite précise du parseur.

## Politique de rétrogradation

- Les nouveaux objets et champs sont convertis vers une syntaxe équivalente quand c'est possible.
- Les informations visibles et de fabrication sont conservées si l'ancien format peut les représenter.
- Une syntaxe est supprimée seulement si l'ancien KiCad ne peut pas la charger ou si l'ancien format n'a pas d'équivalent.
- Chaque suppression ou réécriture de compatibilité est signalée comme warning.

Lors de la conversion d'un dossier de projet, seuls les fichiers KiCad utiles et les modèles 3D locaux courants sont copiés. Les dossiers `.history`, sauvegardes, Gerber, BOM, PDF, README et autres fichiers sans rapport ne sont pas copiés.

## Compilation

Windows :

```powershell
.\build.ps1
.\build.ps1 -SetupMissingTools
```

Linux/macOS :

```sh
./build.sh
./build.sh --setup
```

Les sorties sont créées dans `dist/`.

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

Sous Windows, `build.ps1` utilise Visual Studio pour Windows amd64/arm64 et WSL pour Linux amd64/arm64 si la chaîne d'outils est disponible. Les binaires Darwin nécessitent macOS et l'Apple SDK.

## Validation

Après conversion, validez avec la version KiCad CLI correspondante. Pour KiCad 8/9/10, exécutez généralement ERC et DRC :

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 dispose de moins de commandes CLI ; utilisez l'export netlist et Gerber pour vérifier que les fichiers se chargent :

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

Les violations ERC/DRC sont des résultats de règles de conception. Elles ne signifient pas un échec de conversion sauf si KiCad signale une erreur de chargement ou de parsing.

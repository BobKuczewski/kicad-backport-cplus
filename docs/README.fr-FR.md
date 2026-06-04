# Rétroportage KiCad C++

Implémentation autonome C++17 de la CLI de rétrogradation de KiCad Backport. L'outil
convertit les nouveaux fichiers de projet KiCad S-expression en formats de fichiers KiCad plus anciens
tout en préférant une syntaxe héritée équivalente à la suppression.

## Documentation localisée

- [简体中文](docs/README.zh-CN.md)
- [日本語](docs/README.ja-JP.md)
- [한국어](docs/README.ko-KR.md)
- [Français](docs/README.fr-FR.md)
- [Allemand](docs/README.de-DE.md)
- [Espagnol](docs/README.es-ES.md)
- [Italien](docs/README.it-IT.md)

Références supplémentaires :

- [Différences de version du format de fichier KiCad](docs/kicad-file-format-version-differences.md)
- [Différences de version du format de fichier localisé](docs/README.md#kicad-file-format-version-differences)

## Commandes

L'interface de ligne de commande reflète l'implémentation Go et est destinée à être
utilisable à la fois directement et depuis le plugin Python :

```text
kicad-backport convert --target-version <6.0|7.0|8.0|9.0|10.0|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

Le convertisseur lit les fichiers d'expression KiCad S et applique une rétrogradation basée sur la version.
règles, écrit un chemin de sortie avec le suffixe de version et peut copier l'intégralité du projet KiCad
répertoires avant de normaliser tous les fichiers KiCad dans la copie.

Exemples :

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

Les alias de version pris en charge sont `6.0`, `7.0`, `8.0`, `9.0` et `10.0`. Un brut
Le numéro de version du format de fichier KiCad peut également être transmis lors du test d'un format de fichier spécifique.
coupure de l'analyseur.

## Statut d'assistance

L'implémentation actuelle cible KiCad 6 jusqu'au fichier d'expression S KiCad 10.
familles:

| Cible | Statut |
| --- | --- |
| KiCad 10 | Supprime la syntaxe post-10.0/développement actuel, y compris 20260521 pad `sim_electrical_type` et 20260603 table-cell `knockout`. |
| KiCad9 | Supprime ou rétrograde les fonctionnalités actuelles de KiCad 10 telles que les variantes, les codes-barres, le contre-perçage/post-usinage, les cavaliers et l'omission du netcode. |
| KiCad 8 | Supprime les tables KiCad 9+, les fichiers intégrés, les classes de composants, les padstacks, via les piles, les zones de règles et les formulaires de couche utilisateur arbitraires. |
| KiCad 7 | Applique les anciennes réécritures de compatibilité de l'analyseur pour les formulaires UUID/tstamp, les champs d'empreinte PCB, les larmes, les objets générés, les images et les zones de texte. |
| KiCad 6 | La prise en charge de base de la rétrogradation des fichiers est en grande partie terminée. Les projets de test convertis ont été ouverts manuellement dans l'application KiCad 6 actuelle pour validation. |
| KiCad 5 et versions antérieures | Pas encore mis en œuvre. Le code sépare désormais la détection des documents existants, le mappage des chemins et les règles de mise à niveau/rétrogradation pour préparer la future prise en charge de la V5. |

## Politique de rétrogradation

Le convertisseur applique la représentation la plus compatible disponible dans le
format cible :

- Les nouveaux objets ou champs sont mappés à une syntaxe équivalente plus ancienne lorsque cela est possible.
- Les informations visibles/de fabrication sont conservées là où l'ancien format peut les exprimer.
- La syntaxe non prise en charge est supprimée uniquement lorsque les anciens analyseurs KiCad ne peuvent pas la charger ou
l'ancien format de fichier n'a pas de représentation équivalente.
- Chaque suppression ou réécriture de compatibilité est signalée sous forme d’avertissement.

Par exemple, les codes réseau existants sont reconstruits pour les anciens formats de PCB, les nouveaux formats booléens
les formes de champ sont converties en atomes de présence si nécessaire, KiCad 7 PCB
les dimensions sont conservées sous forme de graphiques visibles et d'un tableau local du projet existant
des fichiers de visibilité sont générés pour les cibles KiCad 6/7/8.

Lors de la conversion d'un répertoire de projet ou de `.kicad_pro`, l'outil copie uniquement
entrées KiCad modifiables et fichiers de modèle 3D locaux communs. Fabrication générée
les sorties, les dossiers d'historique/de sauvegarde, les Gerbers, les nomenclatures et les fichiers temporaires sont ignorés.

## Disposition du projet

Le code est divisé par responsabilité afin que les versions ultérieures de KiCad puissent être ajoutées avec
petits changements localisés :

- `src/kicad_backport.cpp` : flux CLI, copie/filtrage de projet, conversion de fichiers.
- `src/kicad_backport_document.cpp` : Détection du type de document KiCad.
- `src/kicad_backport_legacy.cpp` : ancien échafaudage de chargement de documents KiCad.
- `src/kicad_backport_pathmap.cpp` : assistants de mappage d'extension de fichier cible.
- `src/kicad_backport_report.cpp` : formatage du rapport JSON.
- `src/kicad_backport_rules.cpp` : portes de version et ordre des règles de rétrogradation.
- `src/kicad_backport_rule_rewriters.cpp` : aides à la réécriture de l'arbre d'expression S.
- `src/kicad_backport_upgrade.cpp` : normalisation de la syntaxe limitée pour les anciens fichiers sources.
- `src/kicad_backport_versions.cpp` : alias de version KiCad et versions de format.
- `src/kicad_backport_util.cpp` : aides de chaîne, de fichier et JSON partagées.
- `src/sexpr.cpp` : analyseur/formatteur d'expression S minimal de style KiCad.
- `src/internal/` : en-têtes d'implémentation privés utilisés uniquement par les fichiers sources.
- `include/kicad_backport/` : en-têtes de projet publics utilisés par l'exécutable.

Les règles de rétrogradation à action unique utilisent un petit assistant `applyWhen()` au lieu de
`std::function`, en gardant les règles compactes sans ajouter d'allocations de tas.
Les règles multi-actions restent regroupées lors de la commande des sujets.

La structure de niveau supérieur est volontairement simple :

```text
kicad-backport-cplus/
  include/kicad_backport/   public headers
  src/                      implementation files
  src/internal/             private headers
  scripts/                  cross-build environment setup
  build/                    generated build trees
  dist/                     packaged command-line binaries
```

## Construire

Construire sur la plateforme actuelle :

```powershell
.\build.ps1
```

```sh
./build.sh
```

Détecter et installer automatiquement la plus petite chaîne d'outils pratique avant
bâtiment:

```powershell
.\build.ps1 -SetupMissingTools
```

```sh
./build.sh --setup
```

Les deux scripts essaient les cibles de version standard et copient les sorties réussies dans
`dist/` en utilisant des noms compatibles avec les plugins :

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

Utilisez `.\build.ps1 -Clean` ou `./build.sh --clean` pour supprimer la version précédente
sorties avant la reconstruction.

La compilation croisée C++ nécessite des chaînes d'outils de plateforme. Sous Windows, `build.ps1`
construit `windows-amd64` et `windows-arm64` avec Visual Studio, et construit
`linux-amd64`/`linux-arm64` via WSL lorsque la chaîne d’outils WSL est disponible.
Sous Linux, `build.sh` construit Linux natif et peut créer `linux-arm64` lorsque
`aarch64-linux-gnu-g++` est installé. Sur macOS, `build.sh` construit Darwin
amd64/arm64 avec le SDK Apple. Les binaires Darwin doivent être générés sur macOS.

Pour créer un sous-ensemble :

```powershell
.\build.ps1 -Targets windows-amd64,windows-arm64
```

```sh
TARGETS="linux-amd64 linux-arm64" ./build.sh
```

Configuration de l'environnement cross-build :

```powershell
.\scripts\setup-cross.ps1
.\scripts\setup-cross.ps1 -CheckOnly
```

```sh
./scripts/setup-cross.sh
./scripts/setup-cross.sh --check-only
```

Les scripts d'installation installent automatiquement la plus petite chaîne d'outils de construction pratique
pour la plateforme hôte. Utilisez `-CheckOnly` ou `--check-only` pour signaler uniquement les éléments manquants.
outils sans rien installer.

Sous Windows, le script d'installation installe ou prépare CMake, Visual Studio C++
Outils de construction, WSL, Ubuntu et les packages WSL minimaux nécessaires pour Linux
Versions amd64/arm64. Sous Linux, il installe CMake, un compilateur natif C++, Ninja,
et le compilateur croisé Linux aarch64 lorsqu'il est pris en charge par le package hôte
directeur. Sur macOS, il déclenche les outils de ligne de commande Apple et installe CMake/Ninja
via Homebrew lorsqu’il est disponible.

Construction manuelle de CMake :

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

L'implémentation est intentionnellement sans dépendance et suit le C++ de style KiCad.
conventions de formatage.

## Validation

Après la conversion, validez chaque cible avec la version KiCad correspondante. Pour
KiCad 8/9/10, cela signifie généralement exécuter un schéma ERC et PCB DRC :

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 CLI a un jeu de commandes plus petit, utilisez donc netlist et Gerber export vers
vérifiez que les fichiers schématiques et PCB convertis se chargent :

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

KiCad 6 a une couverture de validation CLI limitée. Pour les fichiers PCB, une vérification rapide de l'analyseur
peut être effectué via le module Python de KiCad 6 :

```powershell
& 'D:\KiCad\6.0\bin\python.exe' -c "import pcbnew; pcbnew.LoadBoard(r'E:\tmp\project_V6\project.kicad_pcb'); print('pcb ok')"
```

Pour les schémas et symboles KiCad 6, l'ouverture manuelle de l'interface graphique reste la plus utile
validation de bout en bout. Les échantillons de régression V6 actuels ont été vérifiés de cette façon.

Les violations ERC/DRC sont des conclusions des règles de conception du projet. Ils ne sont pas
échecs de conversion de format, sauf si KiCad signale une erreur de chargement ou d'analyse.

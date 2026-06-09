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
kicad-backport convert --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

Le convertisseur lit les fichiers d'expression KiCad S et applique une rétrogradation basée sur la version.
règles, écrit un chemin de sortie avec le suffixe de version et peut copier l'intégralité du projet KiCad
répertoires avant de normaliser tous les fichiers KiCad dans la copie. Pendant la conversion,
il affiche la version source détectée et la version cible résolue pour chaque fichier KiCad.
La sortie texte privilégie les alias KiCad comme `9.0 (20241229)` ou
`10.99-dev (20260513)` plutôt que les numéros bruts de format de fichier.
`detect-versions` est un scan rapide de répertoire qui ne lit que le texte nécessaire
pour signaler les types et versions de fichiers KiCad. La sortie texte utilise les mêmes
alias, tandis que les rapports JSON conservent les versions brutes. Il filtre d'abord par
extensions KiCad prises en charge et ignore les fichiers dont la version ne peut pas être identifiée.

Exemples :

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions E:\tmp\project
```

Les alias de version pris en charge sont `4.0`, `5.0`, `5.1`, `6.0`, `7.0`,
`8.0`, `9.0`, `10.0` et `10.99`. Une version brute du format de fichier KiCad peut
également être transmise pour tester une coupure précise de l'analyseur.

## Statut d'assistance

L'implémentation actuelle cible les familles de fichiers KiCad 4 à KiCad 10 :

| Cible | Statut |
| --- | --- |
| KiCad 10 | Supprime la syntaxe post-10.0/développement actuel, y compris 20260521 pad `sim_electrical_type` et 20260603 table-cell `knockout`. |
| KiCad 10.99 | Cible de developpement actuelle pour board/footprint : ecrit la version `20260603`; les bibliotheques de symboles et les schemas utilisent encore les versions cibles KiCad 10 (`20251024` / `20260306`). |
| KiCad9 | Supprime ou rétrograde les fonctionnalités actuelles de KiCad 10 telles que les variantes, les codes-barres, le contre-perçage/post-usinage, les cavaliers et l'omission du netcode. |
| KiCad 8 | Supprime les tables KiCad 9+, les fichiers intégrés, les classes de composants, les padstacks, via les piles, les zones de règles et les formulaires de couche utilisateur arbitraires. |
| KiCad 7 | Applique les anciennes réécritures de compatibilité de l'analyseur pour les formulaires UUID/tstamp, les champs d'empreinte PCB, les larmes, les objets générés, les images et les zones de texte. |
| KiCad 6 | La prise en charge de base de la rétrogradation des fichiers est en grande partie terminée. Les projets de test convertis ont été ouverts manuellement dans l'application KiCad 6 actuelle pour validation. |
| KiCad 5 | Prend en charge la version cible board/footprint `20171130` et l'import/export de base des fichiers legacy `.sch`, `.lib`, `.dcm` et `.pro`. Les objets de schéma détaillés, primitives graphiques de symboles et broches restent avec perte et sont signalés par des avertissements. |
| KiCad 4 | Prend en charge la version cible board/footprint `4`, la réécriture des en-têtes legacy V4 pour schémas/bibliothèques, ainsi que les suffixes et extensions de sortie V4. Les fonctionnalités PCB propres à V5, comme les pads personnalisés, sont simplifiées lorsque possible. |

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
Lors du passage de la frontière KiCad 5/6, les extensions sont automatiquement adaptées,
par exemple `.sch -> .kicad_sch`, `.lib -> .kicad_sym`, `.kicad_sch -> .sch`,
`.kicad_sym -> .lib/.dcm` et `.kicad_pro -> .pro`.

## Disposition du projet

Le code est divisé par responsabilité afin que les versions ultérieures de KiCad puissent être ajoutées avec
petits changements localisés :

- `src/kicad_backport.cpp` : flux CLI, copie/filtrage de projet, conversion de fichiers.
- `src/kicad_backport_document.cpp` : Détection du type de document KiCad.
- `src/kicad_backport_legacy.cpp` : aides de lecture/écriture des anciens fichiers KiCad `.sch`, `.lib`, `.dcm` et `.pro`.
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
  build/                    generated build trees
  dist/                     packaged command-line binaries
```

## Construire

Il ne reste que deux points d'entrée de compilation :

- `build.ps1` pour les builds natifs Windows et les builds croisés Linux temporaires depuis Windows via WSL.
- `build.sh` uniquement pour les builds natifs Linux/macOS.

Build natif depuis un nouveau checkout :

```sh
git clone <repo-url> kicad-backport-cplus
cd kicad-backport-cplus
./build.sh --setup
./build.sh --config Release
```

Sous Windows :

```powershell
git clone <repo-url> kicad-backport-cplus
cd kicad-backport-cplus
.\build.ps1 -SetupMissingTools
.\build.ps1 -Targets windows-amd64
```

Cibles Linux depuis Windows via WSL :

```powershell
.\build.ps1 -Targets linux-amd64,linux-arm64,linux-armhf
```

Options natives utiles :

```sh
./build.sh --clean
./build.sh --compiler g++-8
./build.sh --direct
./build.sh --static-runtime off
```

Les sorties sont copiées dans `dist/`. Le code actuel nécessite un support C++17 pour `filesystem`, `pmr` et `string_view`; la couche de compatibilité accepte les implémentations standard ou experimental, et les builds directs reviennent de `-std=c++17` à `-std=c++1z`.

Build CMake manuel :

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Build manuel sans CMake :

```sh
./build.sh --config Release --target native --direct
```
## Remerciements

Merci tout particulièrement à Hubert pour l'aide apportée pendant le développement
de ce projet.

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

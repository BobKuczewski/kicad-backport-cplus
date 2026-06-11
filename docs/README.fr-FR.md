# KiCad Backport C++

`kicad-backport` est un convertisseur en ligne de commande autonome implémenté en C++ portable pour déplacer des projets et fichiers KiCad vers d’anciens formats cibles. Il vise la compatibilité des parseurs : lorsqu’une ancienne version de KiCad possède une représentation équivalente, le convertisseur réécrit vers cette représentation ; sinon la syntaxe non prise en charge est supprimée ou approximée et signalée par warning.

L’implémentation ne dépend d’aucune bibliothèque externe, contient un petit parser/formatter S-expression de style KiCad, et peut être utilisée directement ou via un plugin wrapper.

## Documentation

- [Index de documentation](README.md)
- [Différences de format du convertisseur](kicad-backport-converter-format-differences.fr-FR.md)
- [README anglais](../README.md)

## Commandes

```text
kicad-backport convert [--quiet] --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

Exemple :

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions --json E:\tmp\project
```

`convert` accepte un document KiCad ou un répertoire de projet. Un `.kicad_pro` convertit aussi le répertoire de projet parent. Le chemin de sortie reçoit un suffixe de cible, par exemple `project_V9`.

`inspect` rapporte le type et la version détectés. `detect-versions` effectue un scan léger et peut produire du JSON.

## Cibles prises en charge

| Cible | Comportement actuel |
| --- | --- |
| KiCad 10.99 | Alias de développement. Board/footprint écrivent `20260603`; schematic/symbol-library restent sur les ancres KiCad 10. |
| KiCad 10 | Supprime ou réécrit la syntaxe de développement hors des ancres `10.0`. |
| KiCad 9 | Supprime ou downgrade variants, barcodes, backdrill/post-machining, metadata jumper pad et références board uniquement par nom de net. |
| KiCad 8 | Supprime ou réécrit tables, embedded files/fonts, component classes, padstacks, via stacks, rule/placement areas, user-layer type qualifiers et font face fields. |
| KiCad 7 | Applique la compatibilité UUID/tstamp, PCB footprint fields, teardrops, generated objects, images, text boxes et stroke/dimension syntax. |
| KiCad 6 | Cible la première famille moderne schematic/symbol/project et ajoute les structures de compatibilité nécessaires. |
| KiCad 5.0/5.1 | Board/footprint utilisent `20171130`; schematic, symbol-library et project écrivent `.sch`, `.lib/.dcm`, `.pro` legacy. |
| KiCad 4 | Board/footprint utilisent `4`, les headers legacy V4 sont réécrits et les constructions PCB KiCad 5+ sont simplifiées si possible. |

Voir [différences de format](kicad-backport-converter-format-differences.fr-FR.md) pour les détails.

## Politique de conversion

- Préserver l’intention existante si le format cible peut l’exprimer.
- Réécrire vers une syntaxe ancienne équivalente lorsqu’elle existe.
- Supprimer uniquement ce que les anciens parseurs ne peuvent pas lire ou ce qui n’a pas d’équivalent.
- Signaler les réécritures avec perte et suppressions par warning.
- Ne pas créer de nouvelles fonctionnalités KiCad absentes de la source.

La frontière KiCad 5/6 change les familles de fichiers : `.sch -> .kicad_sch`, `.lib/.dcm -> .kicad_sym`, `.pro -> .kicad_pro`, et inversement `.kicad_sch -> .sch`, `.kicad_sym -> .lib + .dcm`, `.kicad_pro -> .pro`.

## Conversion de projet

La conversion d’un répertoire copie seulement les entrées KiCad éditables et les modèles 3D locaux courants, puis convertit les documents copiés. Les sorties de fabrication, sauvegardes, history, Gerbers, BOM, répertoires plot/export et fichiers temporaires sont ignorés.

Les réparations projet incluent la normalisation de `sym-lib-table` / `fp-lib-table`, les données `.kicad_prl` pour KiCad 6/7/8, l’intégration de symboles locaux dans `lib_symbols` et la reconstruction des hierarchy instances.

## Build

```powershell
.\build.ps1
```

```sh
./build.sh
```

Les scripts lisent `kicad_backport_sources.txt`, compilent avec `g++` ou `clang++`, puis copient l’exécutable dans `dist/`. Ils peuvent basculer de `-std=c++17` vers `-std=c++1z` si nécessaire.

## Validation

Après conversion, ouvrir le résultat avec la version KiCad cible et exécuter ERC/DRC si applicable. Les warnings du convertisseur doivent être examinés avant un usage en production.

## Remerciements

Remerciements particuliers à Hubert pour l’aide apportée pendant le développement de ce projet.

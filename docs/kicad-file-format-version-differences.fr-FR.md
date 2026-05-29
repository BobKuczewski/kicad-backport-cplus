# Différences de versions du format de fichier KiCad

Ce document est la version française du [document de référence en anglais](kicad-file-format-version-differences.md). Il résume les versions de format utilisées par KiCad Backport C++, les différences principales et les règles de rétroportage déjà implémentées. Les tokens KiCad, macros source, extensions et numéros de version restent en anglais pour faciliter la recherche dans le code.

Dernière mise à jour : 2026-05-29.

## Sources

- Tags officiels KiCad et fichiers d'en-tête de versions.
- Checkout local KiCad `master` : `10.99.0-1153-g8c1d374f29`.
- Implémentation `kicad-backport-cplus` : `kicad_backport_versions.cpp`, `kicad_backport_rules.cpp`, `kicad_backport_rule_rewriters.cpp`, `kicad_backport.cpp`.

Les versions proviennent des macros `SEXPR_SYMBOL_LIB_FILE_VERSION`, `SEXPR_SCHEMATIC_FILE_VERSION`, `SEXPR_BOARD_FILE_VERSION`, `SEXPR_WORKSHEET_FILE_VERSION` et `DRC_RULE_FILE_VERSION`.

## Matrice des versions stables

| KiCad tag | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

La version S-expression des boards couvre aussi les footprints `.kicad_mod`. `.kicad_dru` reste à `20200610` dans les versions suivies ; cela ne garantit pas que la sémantique des règles soit inchangée. `.kicad_pro` est un fichier JSON géré par migration de schéma et doit être documenté séparément.

## 10.99 / current

| Type de fichier | Version 10.99/current | Notes |
| --- | ---: | --- |
| Board `.kicad_pcb` | `20260513` | Copper thieving zone fill mode |
| Footprint `.kicad_mod` | `20260513` | Utilise la version S-expression PCB |
| Schematic `.kicad_sch` | `20260512` | Net chains |
| Symbol library `.kicad_sym` | `20260508` | Native ellipse primitive |
| Worksheet `.kicad_wks` | `20231118` | Pas de bump après le nettoyage KiCad 8 |
| Design rules `.kicad_dru` | `20200610` | Aucun bump propre à 10.99 trouvé |

Les ajouts principaux après KiCad 10 sont les métadonnées extruded 3D body, les ellipses natives, les champs dielectric frequency, les net chains et copper thieving.

## Versions cibles dans l'implémentation C++

| Alias | Symbol | Schematic | Board | Footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |

Le convertisseur ne met pas à niveau les anciens fichiers. Si la version source est inférieure à la cible demandée, le fichier est copié sans modification et la version source reste dans le rapport.

## Règles principales de rétroportage

Symbol library / schematic :

- Suppression des noeuds non acceptés par les anciens parseurs : `text_box`, embedded files, private flags, rounded rectangles, native ellipses, schematic variants, net chains.
- Ajout d'IDs de propriétés legacy et déplacement de property `hide` dans `effects`.
- Conversion des listes booléennes `hide` et des champs font `bold` / `italic` vers des presence atoms.
- Suppression selon la cible de `generator_version`, `embedded_fonts`, font `face`, `show_name`, `do_not_autoplace`, power class flags et jumper pin flags.

Board / footprint :

- Suppression ou conversion de text boxes, images, net ties, generated objects, teardrops, tables, tenting, embedded files, component classes, padstacks, via stacks, rule areas, via protection, custom layer counts, rounded rectangles, points, barcodes, backdrill, variants, extruded models, native ellipses, dielectric frequency fields, net chains et copper thieving.
- Conversion de copper thieving fill mode vers polygon fill.
- Conversion des propriétés footprint `Reference` / `Value` vers `fp_text` legacy.
- Renommage de `uuid` vers `tstamp` pour les objets concernés, et de `uuid` vers `id` pour group/generated.
- Conversion des PCB dimensions incompatibles KiCad 7 en annotations texte visibles.
- Reconstruction des numeric netcodes et root-level net declarations pour les anciens boards.

Worksheet supprime les blocs `font` pour les cibles antérieures à `20220228`. Design rules est détecté et résolu en version cible, mais n'a pas encore de rewrite dédié.

## Rapports et maintenance

Chaque suppression ou rewrite compatible qui modifie l'arbre produit un warning. Le rapport contient path, kind, source version, target version, changed et warnings. Pour ajouter une nouvelle version KiCad, mettre à jour la matrice d'abord, puis documenter séparément les différences stable tag et development branch.

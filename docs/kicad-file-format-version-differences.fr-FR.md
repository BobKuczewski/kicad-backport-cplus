# Différences de version du format de fichier KiCad

Ce document suit les différences de version du format de fichier KiCad utilisé par le backport
convertisseur. Il est organisé de manière à ce que des versions stables ou de développement plus récentes puissent être ajoutées.
sans renommer le fichier.

Dernière mise à jour : 2026-06-05.

## Sources et méthode

Sources examinées :

- Balises GitLab et fichiers sources officiels de KiCad.
- Checkout KiCad local à `E:/WORKS/MY/kicadProject/kicad`.
- Références et balises locales : `origin/4.0`, `4.0.0`, `origin/5.0`, `origin/5.1`,
  `5.0.0`, `5.1.0`, `6.0.0`, `7.0.0`, `8.0.0`, `9.0.0`, `10.0.0`,
et `origin/10.0`.
- KiCad local `master`, utilisé uniquement pour identifier la branche de développement post-10.0
frontières.
- Implémentation de `kicad-backport-cplus`, notamment :
  - `src/kicad_backport_versions.cpp`
  - `src/kicad_backport_rules.cpp`
  - `src/kicad_backport_rule_rewriters.cpp`
  - `src/kicad_backport_upgrade.cpp`
  - `src/kicad_backport.cpp`
- Fichiers d'en-tête de version :
  - `pcbnew/kicad_plugin.h` pour les formats PCB KiCad 4/5.
  - `pcbnew/plugins/kicad/pcb_plugin.h` pour les formats PCB KiCad 6/7.
  - `eeschema/sch_file_versions.h`
  - `pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h`
  - `include/drawing_sheet/ds_file_versions.h`
  - `pcbnew/drc/drc_rule_parser.h`
  - `eeschema/general.h` et `eeschema/sch_legacy_plugin.h` pour KiCad 4/5
formats schématiques existants.

Les numéros de version proviennent des macros sources KiCad actives :

- `SEXPR_SYMBOL_LIB_FILE_VERSION`
- `SEXPR_SCHEMATIC_FILE_VERSION`
- `SEXPR_BOARD_FILE_VERSION`
- `SEXPR_WORKSHEET_FILE_VERSION`
- `DRC_RULE_FILE_VERSION`

Remarques :

- Les versions Board S-expression couvrent également les fichiers d'empreinte `.kicad_mod`.
- `.kicad_dru` est resté à `20200610` de KiCad 6.0 jusqu'aux sources 10.99 actuelles.
Cela signifie seulement que la macro de version n'a pas changé ; la sémantique des règles peut encore avoir
modifié.
- `.kicad_pro` est un fichier de projet JSON et utilise à la place la migration des paramètres/schémas
de ces macros de version de date d’expression S. Différences de schéma JSON du projet
doivent être suivis séparément.
- Les schémas et bibliothèques de symboles KiCad 4/5 sont hérités `.sch`, `.lib` et
Fichiers `.dcm`, pas `.kicad_sch` ou `.kicad_sym`.

## Matrice des familles de fichiers principales

| Version majeure de KiCad | Projet | Schématique | Bibliothèque de symboles | PCB/empreinte | Feuille de travail | Règles de conception | Point clé |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 4.0 | Héritage `.pro` | Ancien `.sch`, `EESCHEMA_VERSION=2` | `.lib` `EESchema-LIBRARY Version 2.3`, `.dcm` | `.kicad_pcb` / `.kicad_mod` Expression S, version `4` | Feuille de dessin héritée | Pas de `.kicad_dru` autonome | PCB était déjà une expression S ; les bibliothèques de schémas et de symboles étaient encore héritées. |
| 5.0 / 5.1 | Héritage `.pro` | Ancien `.sch`, `EESCHEMA_VERSION=4` | Généralement `.lib` `Version 2.4`, `.dcm` | `.kicad_pcb` / `.kicad_mod` Expression S, version `20171130` | Feuille de dessin héritée | Pas de `.kicad_dru` autonome | PCB a ajouté des tampons personnalisés, des protections multicouches et des modifications de décalage du modèle 3D ; le schéma est resté un héritage. |
| 6.0 | JSON `.kicad_pro`, `.kicad_prl` | `.kicad_sch` `20211123` | `.kicad_sym` `20211014` | `20211014` | `.kicad_wks` `20210606` | `.kicad_dru` `20200610` | Nouveaux formats d’expression S schématiques et symboliques. |
| 7.0 | JSON `.kicad_pro` | `.kicad_sch` `20230121` | `.kicad_sym` `20220914` | `20221018` | `.kicad_wks` `20220228` | `20200610` | Zones de texte, polices, DNP, modifications du modèle de simulation, liens nets, images, mots-clés en forme de larme. |
| 8.0 | JSON `.kicad_pro` | `.kicad_sch` `20231120` | `.kicad_sym` `20231120` | `20240108` | `.kicad_wks` `20231118` | `20200610` | `generator_version`, nettoyage V8, champs PCB, objets générés, normalisation UUID. |
| 9.0 | JSON `.kicad_pro` | `.kicad_sch` `20250114` | `.kicad_sym` `20241209` | `20241229` | `.kicad_wks` `20231118` | `20200610` | Fichiers intégrés, tables, zones de règles, classes de composants, padstacks, via des piles, couches utilisateur arbitraires. |
| 10.0 | JSON `.kicad_pro` | `.kicad_sch` `20260306` | `.kicad_sym` `20251024` | `20260206` | `.kicad_wks` `20231118` | `20200610` | Variantes, cavaliers, codes-barres, via protection, backdrill, division via types, arrêt de l'écriture des netcodes. |

## Matrice de versions stables

| Balise KiCad | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

## Limite héritée de KiCad 4/5

KiCad 4 et 5 ne sont pas seulement d'anciennes versions d'expression S pour les données schématiques. Ils
utilisez une autre famille de fichiers de schémas et de bibliothèques de symboles :

| Zone | KiCad 4.0 | KiCad 5.0 / 5.1 |
| --- | --- | --- |
| En-tête schématique | `EESchema Schematic File Version 2` | `EESchema Schematic File Version 4` |
| Macro schématique | `EESCHEMA_VERSION 2` | `EESCHEMA_VERSION 4` |
| En-tête de la bibliothèque de symboles | Généralement `EESchema-LIBRARY Version 2.3` | Généralement `EESchema-LIBRARY Version 2.4` |
| Version PCB | `4` | `20171130` |

La version KiCad 5 PCB/empreinte est antérieure à la ligne de développement KiCad 6 :

| Version | Changement |
| ---: | --- |
| 20160815 | Paramètres de paire différentiels par classe de réseau |
| 20170123 | Refactoriser `EDA_TEXT` ; déplacé `hide` |
| 20170920 | Noms de pad longs et forme de pad personnalisée |
| 20170922 | Les zones d'exclusion peuvent exister sur plusieurs couches |
| 20171114 | Enregistrez le décalage du modèle 3D en mm au lieu de pouces |
| 20171125 | Texte d'empreinte verrouillé/déverrouillé |
| 20171130 | Décalage du modèle 3D écrit à l'aide du paramètre `offset` |

Implications du rétroportage :

- Les cibles schématiques KiCad 4/5 nécessitent un graveur `.sch` existant, et pas seulement un
Réécriture de la version `.kicad_sch`.
- Les cibles de symboles KiCad 4/5 nécessitent une sortie héritée `.lib` / `.dcm` ou une sortie explicite.
avertissement avec perte/non implémenté.
- Les cibles de la carte KiCad 4 utilisent la version `4` ; Les cibles de la carte KiCad 5 utilisent `20171130`.
- UUID V6+, zones de texte, fichiers intégrés, variantes, tableaux, zones de règles, composant
les classes, padstacks, via stacks, backdrill et structures similaires ne peuvent pas être
conservés directement dans les fichiers V4/V5.

## Matrice des versions de développement actuelles

La branche KiCad `master` révisée est déjà passée au développement 11.0.
Ces résultats sont des éléments de développement post-10.0 et ne doivent pas être étiquetés comme KiCad.
Prise en charge du format stable 10.0 :

| Type de fichier | Version de développement actuelle | Remarques |
| --- | ---: | --- |
| Tableau `.kicad_pcb` | `20260603` | Drapeau knock-out sur les cellules du tableau |
| Empreinte `.kicad_mod` | `20260603` | Les empreintes utilisent la version PCB S-expression |
| Schéma `.kicad_sch` | `20260512` | Chaînes de filet |
| Bibliothèque de symboles `.kicad_sym` | `20260508` | Primitive d'ellipse native |
| Feuille de travail `.kicad_wks` | `20231118` | Version générateur / Nettoyage KiCad 8 |
| Règles de conception `.kicad_dru` | `20200610` | Aucune modification de version spécifique au développement actuel trouvée |

Étapes de développement de la version post-10.0 trouvées jusqu'à présent :

| Version | Type de fichier | Changement |
| ---: | --- | --- |
| 20260410 | Board / footprint | Corps 3D extrudé |
| 20260508 | Board / footprint | Primitives natives d'ellipse et d'arc d'ellipse de PCB |
| 20260508 | Schéma / symbole | Primitives natives d'ellipse et d'arc d'ellipse de schéma/symbole |
| 20260511 | Conseil | Modèles d'empilement diélectriques dépendants de la fréquence |
| 20260512 | Carte / schéma | Chaînes de filet |
| 20260513 | Conseil | Mode de remplissage de la zone de vol de cuivre |
| 20260521 | Board / footprint | Types électriques de simulation de tampons |
| 20260603 | Board / footprint | Drapeau knock-out sur les cellules du tableau |

## 6,0 à 7,0

### Bibliothèque de symboles

| Version | Changement |
| ---: | --- |
| 20220101 | Drapeaux de classe |
| 20220102 | Polices |
| 20220126 | Zones de texte |
| 20220328 | La zone de texte `start/end` a été remplacée par `at/size` |
| 20220331 | Couleurs du texte |
| 20220914 | Noms d’affichage des unités de symboles |
| 20220914 | Les identifiants de propriété ne sont plus enregistrés |

### Schématique

| Version | Changement |
| ---: | --- |
| 20220101 | Cercles, arcs, rectangles, polys, béziers |
| 20220102 | Tiret-point-point |
| 20220103 | Champs d'étiquette |
| 20220104 | Polices |
| 20220124 | `netclass_flag` renommé `directive_label` |
| 20220126 | Zones de texte |
| 20220328 | La zone de texte `start/end` a été remplacée par `at/size` |
| 20220331 | Couleurs du texte |
| 20220404 | Données d'instance de symbole schématique par défaut |
| 20220622 | Nouveau format de modèle de simulation |
| 20220820 | Correction des données d'instance de symbole par défaut |
| 20220822 | Hyperliens d’objet texte |
| 20220903 | Visibilité du nom de champ |
| 20220904 | Ne pas placer automatiquement l'option de champ |
| 20220914 | Prise en charge du DNP |
| 20220929 | Les identifiants de propriété ne sont plus enregistrés |
| 20221002 | Les données d'instance sont revenues à la définition du symbole |
| 20221004 | Les données d'instance sont revenues à la définition du symbole |
| 20221110 | Données d'instance de feuille déplacées vers la définition de feuille |
| 20221126 | Suppression de la valeur et de l'empreinte des données d'instance |
| 20221206 | Champs du modèle de simulation V6 à V7 |
| 20230121 | Sérialisation du chemin de feuille `SCH_MARKER` |

### PCB / Empreinte

| Version | Changement |
| ---: | --- |
| 20211226 | Cote radiale |
| 20211227 | Remplacements de l'angle des rayons à décharge thermique |
| 20211228 | Attribut d'empreinte `allow_soldermask_bridges` |
| 20211229 | Formatage du trait |
| 20211230 | Dimensions en empreintes |
| 20211231 | Couches d'empreinte privée |
| 20211232 | Polices |
| 20220131 | Zones de texte |
| 20220211 | Fin du support de la stratégie de remplissage de zone V5 |
| 20220225 | TEDIT supprimé |
| 20220308 | Texte masqué et propriété de texte graphique verrouillé |
| 20220331 | Paramètre de sélection du tracé sur tous les calques |
| 20220417 | Précisions de cote automatiques |
| 20220427 | Edge.Cuts et Margin exclus des couches privées d'empreinte |
| 20220609 | Mots-clés en forme de larme |
| 20220621 | Prise en charge des images |
| 20220815 | indicateur `allow-soldermask-bridges-in-FPs` |
| 20220818 | Liens nets de première classe |
| 20220914 | Boîtes numérotées de forme personnalisée |
| 20221018 | Connexions de couche de zone via et pad |

### Feuille de travail

| Version | Changement |
| ---: | --- |
| 20220228 | Prise en charge des polices |

## 7,0 à 8,0

### Bibliothèque de symboles

| Version | Changement |
| ---: | --- |
| 20230620 | `ki_description` a été remplacé par le champ `Description` |
| 20231120 | Nettoyage `generator_version` et V8 |

### Schématique

| Version | Changement |
| ---: | --- |
| 20230221 | Symboles de pouvoir modernes, valeur modifiable = net |
| 20230409 | Balisage `exclude_from_sim` |
| 20230620 | `ki_description` a été remplacé par le champ `Description` |
| 20230808 | Champ `Sim.Enable` déplacé vers l'attribut `exclude_from_sim` |
| 20230819 | Plusieurs niveaux d'héritage des symboles de bibliothèque |
| 20231120 | Nettoyage `generator_version` et V8 |

### PCB / Empreinte

| Version | Changement |
| ---: | --- |
| 20230410 | Attribut DNP propagé du schéma vers `attr` |
| 20230517 | Paramètres Pad et via Teardrop |
| 20230620 | Champs de PCB |
| 20230730 | Connectivité des formes graphiques |
| 20230825 | Drapeau de bordure explicite de zone de texte |
| 20230906 | Prise en charge de plusieurs types d'images |
| 20230913 | Modèles de rayons à plaquettes de forme personnalisée |
| 20231007 | Objets génératifs |
| 20231014 | Normalisation du format de fichier V8 |
| 20231212 | Verrouillage d'image de référence / UUID, format booléen d'empreinte |
| 20231231 | Les générateurs et les groupes utilisent `uuid` au lieu de `id` |
| 20240108 | Paramètres de Teardrop modifiés en booléens explicites |

### Feuille de travail

| Version | Changement |
| ---: | --- |
| 20230607 | Images enregistrées en base64 |
| 20231118 | Nettoyage des formats de fichiers `generator_version` et V8 |

## 8,0 à 9,0

### Bibliothèque de symboles

| Version | Changement |
| ---: | --- |
| 20240529 | Fichiers intégrés |
| 20240819 | L'algorithme de hachage de fichier intégré est devenu Murmur3 |
| 20241209 | `SCH_FIELD` drapeaux privés |

### Schématique

| Version | Changement |
| ---: | --- |
| 20240101 | Tableaux |
| 20240417 | Zones de règles |
| 20240602 | Attributs de feuille |
| 20240620 | Fichiers intégrés |
| 20240716 | Plusieurs affectations de classes de réseau |
| 20240812 | Mise en évidence des couleurs Netclass |
| 20240819 | L'algorithme de hachage de fichier intégré est devenu Murmur3 |
| 20241004 | Le symbole `hide` utilise des booléens |
| 20241209 | `SCH_FIELD` drapeaux privés |
| 20250114 | Les références croisées de variables de texte utilisent des chemins complets |

### PCB / Empreinte

| Version | Changement |
| ---: | --- |
| 20240201 | Les remplacements utilisent des propriétés nullables |
| 20240202 | Tableaux |
| 20240225 | `solder_paste_margin` rationalisation |
| 20240609 | Mot-clé `tenting` |
| 20240617 | Angles de table |
| 20240703 | Types de couches utilisateur |
| 20240706 | Fichiers intégrés |
| 20240819 | L'algorithme de hachage de fichier intégré est devenu Murmur3 |
| 20240928 | Classes de composants |
| 20240929 | Padstacks complexes |
| 20241006 | Via des piles |
| 20241007 | Les pistes peuvent porter une couche et une marge de masque de soudure |
| 20241009 | Evolution du format de la zone de règle de placement |
| 20241010 | Les formes graphiques peuvent porter une couche et une marge de masque de soudure |
| 20241030 | Directions des flèches de dimension, normalisation `suppress_zeroes` |
| 20241129 | `keep_text_aligned` normalisé et propriétés de remplissage |
| 20241228 | Les points de la courbe en forme de larme ont été modifiés en booléens |
| 20241229 | Couches utilisateur étendues à un nombre arbitraire |

### Feuille de travail

Aucune modification de version de la feuille de calcul ; reste `20231118`.

## 9,0 à 10,0

### Bibliothèque de symboles

| Version | Changement |
| ---: | --- |
| 20250318 | `~` ne signifie plus un texte vide |
| 20250324 | Groupes de broches de cavalier |
| 20250829 | Rectangles arrondis |
| 20250901 | Notation des broches empilées |
| 20250925 | Alias ​​de bus dans le fichier de projet |
| 20251024 | Mises à jour du formatage des propriétés : `do_not_autoplace`, `show_name` |

### Schématique

| Version | Changement |
| ---: | --- |
| 20250222 | Remplissages hachurés pour les formes |
| 20250227 | Symboles du pouvoir local |
| 20250318 | `~` ne signifie plus un texte vide |
| 20250425 | UUID pour les tables |
| 20250513 | Les groupes peuvent porter le bloc de conception `lib_id` |
| 20250610 | Les zones de règles prennent en charge DNP et d'autres indicateurs |
| 20250827 | Styles de carrosserie personnalisés |
| 20250829 | Rectangles arrondis |
| 20250901 | Notation des broches empilées |
| 20250922 | Variantes schématiques |
| 20251012 | Prise en charge de la hiérarchie schématique plate |
| 20251028 | Mises à jour du formatage des propriétés : `do_not_autoplace`, `show_name` |
| 20260101 | Variantes de PCB |
| 20260306 | Sémantique de la variante `in_bom` corrigée |

### PCB / Empreinte

| Version | Changement |
| ---: | --- |
| 20250210 | Suppression de la zone de texte |
| 20250222 | Éclosion de formes de PCB |
| 20250228 | IPC-4761 via les fonctions de protection |
| 20250302 | Décalages de hachures de zone |
| 20250309 | Règles d'affectation dynamique des classes de composants |
| 20250324 | Coussinets de cavalier |
| 20250401 | Réglage de la longueur du domaine temporel |
| 20250513 | Les groupes peuvent porter le bloc de conception `lib_id` |
| 20250801 | `(island)` remplacé par `(island yes/no)` |
| 20250811 | Propriété de fabrication du tampon à ajustement serré |
| 20250818 | Les empreintes prennent en charge le nombre de couches personnalisées |
| 20250829 | Rectangles arrondis |
| 20250901 | Points PCB |
| 20250907 | UUID pour les tables |
| 20250909 | Métadonnées de l'unité d'empreinte : unités / broches |
| 20250914 | Objets `PCB_BARCODE` |
| 20250926 | Types de via divisés en aveugles / enterrés / traversants |
| 20251027 | Correction de la mise à l'échelle des retards Pad-to-die |
| 20251028 | Arrêté d'écrire des netcodes ; ce sont des détails de mise en œuvre internes |
| 20251101 | Support de forage arrière et de forage tertiaire |
| 20260101 | Variantes de PCB avec remplacements par empreinte |
| 20260206 | Corrections de sérialisation des codes-barres et des attributs de variantes |

### Feuille de travail

Aucune modification de version de la feuille de calcul ; reste `20231118`.

## 10.0 au développement actuel

Par rapport aux fichiers cibles KiCad 10, la branche de développement actuelle examinée
ajoute ces nouvelles étapes de format :

| Version | Type de fichier | Différence |
| ---: | --- | --- |
| 20260410 | Board / footprint | Métadonnées de corps 3D extrudées dans les blocs de modèle d'empreinte |
| 20260508 | Board / footprint | Primitives natives d'ellipse et d'arc d'ellipse de PCB |
| 20260508 | Schéma / symbole | Primitives natives d'ellipse et d'arc d'ellipse de schéma/symbole |
| 20260511 | Conseil | Champs du modèle d'empilement dépendant de la fréquence diélectrique |
| 20260512 | Conseil | Bloc d'agrégation de chaîne nette |
| 20260512 | Schématique | Records de chaîne nette |
| 20260513 | Conseil | Mode de remplissage de la zone de vol de cuivre |
| 20260521 | Board / footprint | Type électrique de simulation de pad, numéroté en tant que `sim_electrical_type` sur les pads |
| 20260603 | Board / footprint | Indicateur `knockout` de cellule de tableau |

## Résumé de la cible de rétroportage à partir des fichiers de développement actuels

Par rapport aux anciennes cibles prises en charge, la version 10.99 introduit ou conserve des cibles plus récentes.
constructions qui doivent être supprimées, simplifiées ou renommées lors du rétroportage :

| Cible | Cible de carte/empreinte | Cible schématique | Cible du symbole | Principales zones de déclassement du développement actuel |
| --- | ---: | ---: | ---: | --- |
| KiCad10 | `20260206` | `20260306` | `20251024` | Supprimez les métadonnées de corps extrudées réservées au développement, les ellipses natives, les champs de fréquence diélectrique, les chaînes de filets, le vol de cuivre, les types électriques de simulation de plots et les indicateurs d'inactivation des cellules de table. |
| KiCad 9 | `20241229` | `20250114` | `20241209` | KiCad 10 éléments plus hachures de forme de PCB, via protection, décalages de hachures de zone, cavaliers, identifiants de blocs de conception de groupe, nombre de couches personnalisées, rectangles arrondis, points de PCB, UUID de table, codes à barres, types de division, omission de code net, contre-perçage/post-usinage, variantes de PCB, variantes schématiques/styles de corps/rectangles arrondis/broches empilées/formatage des propriétés |
| KiCad 8 | `20240108` | `20231120` | `20231120` | Éléments KiCad 9 plus tableaux, fichiers intégrés, classes de composants, padstacks, via piles, zones de règles, tentes, extension de couche utilisateur, attributs de feuille, affectations de classes de réseau multiples, surbrillance des couleurs de classe de réseau |
| KiCad 7 | `20221018` | `20230121` | `20220914` | KiCad 8 éléments plus champs PCB, propagation d'attributs DNP, larmes modernes, modèles de rayons de tampons personnalisés, générateurs, normalisation UUID/id, zones de texte, images, liens nets, formatage de police/champ, zones de règles, simulation schématique/indicateurs d'exclusion modernes |

## Couverture de l’implémentation du backport C++

La CLI `kicad-backport-cplus` implémente des réécritures d’expression S basées sur la version.
Il résout les alias de version par type de document, applique les règles de rétrogradation, puis
écrit le champ cible `version`. Il accepte également un format de fichier numérique brut
version pour tester un seuil d'analyseur spécifique.

Mappages d'alias pris en charge dans le code :

| Alias | Symbole | Schématique | Conseil | Empreinte | Feuille de travail | Règles de conception |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |

Si le fichier source possède déjà exactement la version numérique demandée, le
le convertisseur le copie inchangé. Si la version source est inférieure à la version cible,
l'implémentation C++ applique désormais des mises à niveau de compatibilité limitées avant
écrire le champ `version` demandé :

| Gentil | Normalisation de la mise à niveau implémentée |
| --- | --- |
| Bibliothèque de symboles | Développez les atomes de style de police hérités pour les cibles modernes ; étendre les atomes de visibilité des broches ; déplacer la propriété `hide` hors de `effects` ; supprimer les ID de propriété hérités. |
| Schématique | Renommez `tstamp` en `uuid` ; renommez `netclass_flag` en `directive_label` ; convertir l'ancienne zone de texte `start/end` en `at/size` ; développer les polices héritées et les atomes de visibilité des broches ; déplacer la propriété `hide` hors de `effects` ; supprimer les ID de propriété hérités. |
| Board / footprint | Renommez `tstamp` en `uuid` pour les cibles modernes ; développer les atomes de style de police ; étendre l'empreinte des atomes `dnp` ; normaliser les booléens en `yes/no` de style KiCad 7 ; supprimer le `tedit` obsolète ; convertissez éventuellement les références réseau numériques héritées en noms de réseau. |

Il ne s’agit pas d’un moteur de mise à niveau sémantique complet ; il normalise uniquement la syntaxe
le convertisseur sait déjà exprimer.

### Couverture de la version cible implémentée

Les règles C++ sont basées sur les coupures, donc chaque alias de version active les règles dont
les seuils sont plus récents que la version cible de cette famille de fichiers. Le résumé suivant
répertorie la couverture pratique pour les cibles stables non V6.

#### Cible KiCad 10

Les cibles KiCad 10 suppriment pour la plupart les constructions post-10.0/actuelles :

| Gentil | Gestion mise en œuvre |
| --- | --- |
| Bibliothèque de symboles | Supprimez les primitives natives d'ellipse et d'arc d'ellipse introduites après la cible des symboles 10.0. |
| Schématique | Supprimez les champs `locked` post-10.0, les primitives d'ellipse natives et `net_chain` / `net_chains`. |
| Board / footprint | Supprimez ou rétrogradez les blocs de modèle typés/extrudés post-10.0, les primitives d'ellipse natives, les champs d'empilement de fréquence diélectrique, les chaînes de filet, le mode de remplissage de vol de cuivre, le tampon `sim_electrical_type` et la cellule de table `knockout`. |
| Fichiers annexes du projet | Aucune réécriture de compatibilité `.kicad_prl` ou table de bibliothèque héritée n'est générée pour le suffixe V10. |

#### Cible KiCad 9

Les cibles KiCad 9 suppriment KiCad 10 et la syntaxe de développement actuelle tout en conservant
fonctionnalités valables dans les versions de fichiers KiCad 9 :

| Gentil | Gestion mise en œuvre |
| --- | --- |
| Bibliothèque de symboles | Supprimez les groupes de broches de cavalier, les rectangles arrondis, les ellipses natives, les symboles `in_pos_files`, `duplicate_pin_numbers_are_jumpers`, les indicateurs de classe `power`, la propriété `show_name` / `do_not_autoplace` et la police `face`. |
| Schématique | Supprimez les rectangles arrondis, les variantes schématiques, les ellipses natives, les chaînes de réseau, les `locked` post-cible, `embedded_fonts`, les styles de corps personnalisés, les indicateurs d'assemblage/simulation de feuille, le symbole `in_pos_files`, les indicateurs de cavalier/classe de puissance, la police `face`, les champs de formatage de propriété et les nœuds racine `group`. |
| Board / footprint | Supprimez ou rétrogradez l'IPC-4761 via la protection, les champs de cavaliers, les sources de placement de classes de composants, les remplissages de hachures de PCB, le nombre de couches personnalisées, les rectangles arrondis, les objets ponctuels de PCB, les codes-barres, les champs de contre-perçage/post-usinage, les variantes de PCB, les fonctionnalités de développement actuel et la police `face` ; reconstruire les netcodes du tableau numérique existant. Tenting est rétrogradé des listes booléennes avant/arrière aux atomes hérités pour cette plage cible. |
| Fichiers annexes du projet | Aucune réécriture `.kicad_prl` héritée n’est générée pour la V9. |

#### Cible KiCad 8

Les cibles KiCad 8 suppriment la syntaxe KiCad 9/10/actuelle et normalisent également plusieurs
Le développement tardif de KiCad-8 revient aux versions de fichier 8.0.0 :

| Gentil | Gestion mise en œuvre |
| --- | --- |
| Bibliothèque de symboles | Supprimez les fichiers/champs privés intégrés V9+ et la syntaxe des cavaliers, rectangles arrondis et ellipses V10+ ; supprimez `embedded_fonts`, la police `face`, les champs de formatage des symboles/propriétés ; ajoutez des ID de propriété hérités et déplacez la visibilité de la propriété vers `effects` ; convertir le style de police et les booléens de visibilité des broches en une ancienne syntaxe atomique. |
| Schématique | Supprimez les tables V9+, les zones de règles, les fichiers intégrés/champs privés et la syntaxe V10+ de rectangle arrondi, de variante, de style de corps et d'ellipse/chaîne nette ; supprimer les indicateurs de simulation/assemblage de texte et de feuille, les champs de formatage de symbole/propriété, la police `face` ; ajoutez des ID de propriété hérités et déplacez la visibilité de la propriété vers `effects` ; convertir les booléens de visibilité des polices et des broches en une syntaxe atomique plus ancienne ; supprimez les nœuds racine `group`. |
| Board / footprint | Supprimez les tables V9+, les tentes, les fichiers/polices intégrés, les classes de composants, les padstacks complexes, via les piles, les zones de règles, via la protection, les qualificatifs de couche utilisateur arbitraires, le nombre de couches personnalisées, les rectangles arrondis, les points PCB, les codes-barres, le backdrill/post-usinage, les variantes et les fonctionnalités de développement en cours. Supprimez également les champs de marge/couche de masque de soudure graphique/piste, l'angle de cellule de tableau, les caches de rendu de texte, la zone de texte/cellule de tableau/couche défoncée, le modèle `hide`, la police `face` et ajoutez les netcodes numériques hérités. `solder_paste_margin_ratio` est renommé `solder_paste_ratio`. |
| Fichiers annexes du projet | Générez les anciens paramètres d'affichage de l'ID numérique `.kicad_prl` pour les cartes V8. |

#### Cible KiCad 7

Les cibles KiCad 7 suppriment la syntaxe KiCad 8/9/10/actuelle et appliquent un analyseur supplémentaire
réécriture de compatibilité autour des champs PCB, des UUID et des données d'empreinte :

| Gentil | Gestion mise en œuvre |
| --- | --- |
| Bibliothèque de symboles | Supprimez V8+ `generator_version`, les polices/fichiers intégrés, les champs privés V9, la syntaxe de cavalier/arrondi/ellipse V10, le symbole `exclude_from_sim`, les champs de formatage de fichier de position et de propriété, les indicateurs de cavalier/classe de puissance et la police `face` ; ajouter des ID de propriété hérités ; déplacer la visibilité de la propriété vers `effects` ; convertir les booléens de police et de visibilité en syntaxe atomique. |
| Schématique | Supprimez V8+ `generator_version` et `fields_autoplaced`, les tables/zones de règles/champs intégrés/privés V9+, la syntaxe arrondie/variante/style de corps V10+, les champs d'exclusion de simulation post-cible, les indicateurs d'assemblage/simulation de feuille, les champs de formatage de symbole/propriété, la police `face` et les nœuds racine `group`. Les atomes UUID ne sont pas cités pour les analyseurs KiCad 6/7, et la visibilité/ID des propriétés sont rétrogradés à l'emplacement hérité. |
| Board / footprint | Supprimez les objets générés par V8+, les larmes, les tableaux, les fichiers/polices intégrés, les classes de composants, les piles pad/via, les zones de règles, la protection via et la syntaxe cible plus récente. Convertir les qualificateurs de type de couche utilisateur en `user` ; supprimez les champs de masque de soudure graphique/piste, les angles de table, les caches de rendu, les drapeaux knock-out, le modèle `hide`, la connectivité du réseau graphique, les champs verrouillés par groupe, via les champs de connexion de couche, les champs de cavalier d'empreinte/lien de réseau/unité, la police `face` et les atomes d'empreinte d'empreinte incompatibles avec l'héritage. Convertissez les propriétés de l'empreinte du PCB en `fp_text`, renommez `uuid`/`id` en formes héritées `tstamp`/`id`, renommez la pâte à souder et les champs thermiques, convertissez les traits en anciens `width`, convertissez les dimensions en graphiques visibles, rétrogradez les booléens/atomes de présence et reconstruisez les netcodes numériques. |
| Fichiers annexes du projet | Générez les anciens paramètres d'affichage de l'ID numérique `.kicad_prl` pour les cartes V7. |

### Détection de documents et gestion de projet

L'implémentation C++ détecte le type de document KiCad principalement à partir de la racine
Tête d'expression S :

| Tête de racine | Gentil |
| --- | --- |
| `kicad_symbol_lib` | Bibliothèque de symboles |
| `kicad_sch` | Schématique |
| `kicad_pcb` | Conseil |
| `footprint` | Empreinte |
| `kicad_dru` | Règles de conception |
| `kicad_wks`, `drawing_sheet` | Feuille de travail |

Si le titre racine est manquant ou inconnu, il revient à l'extension de fichier :
`.kicad_sym`, `.kicad_sch`, `.kicad_pcb`, `.kicad_mod`, `.kicad_dru` et
`.kicad_wks`. Les anciens `.sch`, `.lib`, `.dcm` et `.pro` sont également détectés comme
types de fichiers KiCad hérités, mais la conversion directe à partir de ces familles de fichiers hérités n'est pas
mis en œuvre dans la phase actuelle.

Lors de la conversion d'un répertoire de projet ou de `.kicad_pro`, il copie uniquement les éléments modifiables
Entrées du projet KiCad et fichiers de modèle 3D locaux courants. Sorties générées,
dossiers d'historique/sauvegarde, Gerbers, résultats de fabrication, nomenclatures et fichiers temporaires
sont ignorés. Pour les cartes cibles KiCad 6, 7 et 8, cela crée également un héritage
Paramètres d'affichage de la carte locale `.kicad_prl` avec `visible_items` numérique, complet
`visible_layers` et l'ancienne version méta des paramètres locaux afin de convertir les objets
restent visibles dans les anciennes interfaces graphiques. Pour le projet KiCad 6, il est également ciblé
supprime les nœuds `version` de niveau supérieur de `sym-lib-table` / `fp-lib-table` et
reconstruit les tables d'instances de hiérarchie schématique au niveau racine sur les feuilles enfants.

### Règles de la bibliothèque de symboles

Les portes d'analyseur génériques suppriment ces nœuds introduits lorsque le format de fichier cible
est plus ancienne que la version d'introduction :

| Introduit | Têtes supprimées | Raison |
| ---: | --- | --- |
| 20220126 | `text_box`, `textbox` | Zones de texte des symboles |
| 20240529 | `embedded_files`, `embedded_file` | Fichiers intégrés |
| 20241209 | `private` | Indicateurs SCH_FIELD privés |
| 20250324 | `pin_group`, `pin_groups` | Groupes de broches de cavalier |
| 20250829 | `rounded_rectangle`, `roundrect` | Rectangles arrondis |
| 20260508 | `ellipse`, `ellipse_arc` | Primitives d'ellipse natives |

Réécritures de compatibilité :

| Seuil cible | Récrire |
| ---: | --- |
| `< 20231120` | Supprimer les champs racine `generator_version` |
| `< 20241209` | Supprimez `embedded_fonts` ; ajouter des ID de propriété hérités ; déplacer les indicateurs de la propriété `hide` dans `effects` |
| `< 20230409` | Supprimer les indicateurs d'exclusion de simulation de la bibliothèque de symboles `symbol/exclude_from_sim` |
| `< 20240108` | Convertir les listes de polices `(bold yes/no)` et `(italic yes/no)` en atomes de présence hérités |
| `<= 20241209` | Supprimer les champs de police `face` |
| `< 20241004` | Convertir les listes booléennes `hide` en atomes hérités ; aplatir `pin_names` / `pin_numbers` masquer les listes |
| `<= 20211014` | Ajoutez les ID de propriété standard KiCad 6 : `Reference=0`, `Value=1`, `Footprint=2`, `Datasheet=3`, `ki_keywords=4`, `ki_description=5`, `ki_fp_filters=6` |
| `< 20251024` | Supprimez le symbole `in_pos_files` ; supprimer les propriétés `show_name` et `do_not_autoplace` |
| `< 20250324` | Supprimer `duplicate_pin_numbers_are_jumpers` |
| `< 20250227` | Supprimer les indicateurs de classe du symbole `power` |

### Règles schématiques

Portes d'analyseur génériques :

| Introduit | Têtes supprimées | Raison |
| ---: | --- | --- |
| 20220126 | `text_box`, `textbox` | Zones de texte schématiques |
| 20220622 | `simulation_model`, `sim_model` | Nouveau format de modèle de simulation |
| 20240101 | `table` | Tableaux schématiques |
| 20240417 | `rule_area` | Zones de règles schématiques |
| 20240620 | `embedded_files`, `embedded_file` | Fichiers intégrés |
| 20241209 | `private` | Indicateurs SCH_FIELD privés |
| 20250829 | `rounded_rectangle`, `roundrect` | Rectangles arrondis |
| 20250922 | `variants`, `variant` | Variantes schématiques |
| 20260508 | `ellipse`, `ellipse_arc` | Primitives d'ellipse natives |
| 20260512 | `net_chain`, `net_chains` | Chaînes de réseaux schématiques |

Réécritures de compatibilité :

| Seuil cible | Récrire |
| ---: | --- |
| `< 20231120` | Supprimez `generator_version` ; supprimer `fields_autoplaced` des symboles et des feuilles |
| `< 20260326` | Supprimer les champs schématiques `locked` introduits après la cible |
| `< 20260306` | Supprimez `embedded_fonts` ; retirer la feuille `exclude_from_sim`, `in_bom`, `on_board`, `dnp` ; supprimer les nœuds `group` du schéma racine |
| `< 20250827` | Supprimez `body_styles` et `body_style` |
| `< 20250114` | Supprimer le texte/la zone de texte `exclude_from_sim` |
| `<= 20230121` | Supprimez tous les `exclude_from_sim` restants |
| `< 20220822` | Supprimez les champs `hyperlink` de texte, d'étiquette et d'étiquette de directive |
| `< 20220914` | Supprimer les indicateurs `dnp` du symbole placé |
| `< 20220124` | Renommez les nœuds racine `directive_label` en `netclass_flag` |
| `< 20251024` | Supprimer le symbole `in_pos_files` |
| `< 20250324` | Supprimer `duplicate_pin_numbers_are_jumpers` |
| `< 20250227` | Supprimer les indicateurs de classe du symbole `power` |
| `< 20241004` | Convertir les listes booléennes `hide` en atomes hérités ; aplatir la visibilité des épingles masquer les listes |
| `<= 20211123` | Supprimer les définitions du symbole de bibliothèque `pin/alternate` |
| `< 20240108` | Convertir les listes booléennes de polices grasses/italiques en atomes hérités |
| `<= 20250114` | Supprimer les champs de police `face` |
| `<= 20230121` | Supprimez les atomes `uuid` pour les analyseurs KiCad 6/7 |
| `<= 20211123` | Générez `sheet_instances` et `symbol_instances` au niveau racine KiCad 6 lorsque le schéma racine source contient déjà des données d'instance ; les feuilles enfants ne reçoivent pas de tables d'instance racine |
| `<= 20211123` | Ajoutez les ID de propriété schématique standard KiCad 6 et normalisez les noms/ID de propriété de feuille en `Sheet name=0` et `Sheet file=1`. |
| `<= 20211123` | Supprimez les blocs `instances` internes aux symboles une fois la table d'instance racine de KiCad 6 générée. |
| `< 20241209` | Ajoutez des ID de propriété hérités ; déplacer les indicateurs de la propriété `hide` dans `effects` |
| `< 20251028` | Supprimer les propriétés `show_name` et `do_not_autoplace` |

### Règles du conseil d'administration et de l'empreinte

Portes d'analyseur génériques :

| Introduit | Têtes supprimées | Raison |
| ---: | --- | --- |
| 20220131 | `gr_text_box`, `fp_text_box`, `text_box`, `textbox` | Zones de texte PCB |
| 20220621 | `image` | Objets image PCB |
| 20220818 | `net_tie`, `net_ties` | Rangement en filet de première classe |
| 20231007 | `generated` | Objets génératifs |
| 20240108 | `teardrop`, `teardrops` | Paramètres de larme |
| 20240202 | `table` | Tableaux de circuits imprimés |
| 20240609 | `tenting` | Mot-clé tente |
| 20240706 | `embedded_files`, `embedded_file`, `embedded_fonts` | Fichiers intégrés |
| 20240928 | `component_class`, `component_classes` | Classes de composants |
| 20240929 | `padstack` | Padstacks complexes |
| 20241006 | `via_stack`, `viastack` | Via des piles |
| 20241009 | `rule_area` | Zones de placement/règles |
| 20250228 | `via_protection`, `covering`, `plugging`, `filling`, `capping` | IPC-4761 via protection |
| 20250818 | `custom_layer_count`, `custom_layer_counts` | Nombre de couches d'empreinte personnalisées |
| 20250829 | `rounded_rectangle`, `roundrect` | Rectangles arrondis |
| 20250901 | `point` | Objets ponctuels PCB |
| 20250914 | `barcode`, `pcb_barcode`, `gr_barcode`, `fp_barcode` | Objets de codes-barres PCB |
| 20251101 | `backdrill`, `tertiary_drill`, `front_post_machining`, `back_post_machining` | Champs de backdrill et de forage tertiaire |
| 20260101 | `variants`, `variant` | Variantes de PCB |
| 20260410 | `extruded` | Modèles de corps 3D à empreinte extrudée |
| 20260508 | `gr_ellipse`, `gr_ellipse_arc`, `fp_ellipse`, `fp_ellipse_arc` | Primitives d'ellipse PCB natives |
| 20260511 | `spec_frequency`, `dielectric_model` | Champs d'empilement diélectriques dépendants de la fréquence |
| 20260512 | `net_chains`, `net_chain` | Chaînes de filets PCB |
| 20260513 | `thieving` | Mode de remplissage de la zone de vol de cuivre |

Notes de couverture du développement actuel du KiCad local `10.99.0-1273-gd90e32b6a0` :

| Introduit | Manutention | Remarques |
| ---: | --- | --- |
| 20260521 | Mis en œuvre | Le pad enfant `sim_electrical_type` est supprimé pour les cibles plus anciennes que `20260521`. |
| 20260603 | Mis en œuvre | L'enfant de cellule de tableau `knockout` est supprimé contextuellement pour les cibles plus anciennes que `20260603` ; `knockout` n'est pas utilisé comme porte de jeton globale car d'autres types d'objets l'utilisent également. |

Réécritures de compatibilité :

| Seuil cible | Récrire |
| ---: | --- |
| `< 20260410` | Supprimez les blocs de modèle 3D typés/extrudés en supprimant les nœuds `model` qui contiennent `type` |
| `< 20260513` | Remplacer le mode de remplissage de la zone de vol de cuivre par un remplissage de polygone |
| `>= 20220225` | Supprimer les champs `tedit` d'empreinte obsolètes |
| `>= 20200628` | Supprimer les paramètres `visible_elements` de la carte obsolète |
| `< 20260603` | Supprimer les champs `knockout` de la cellule du tableau PCB |
| `< 20240703` | Convertir les qualificateurs de type de couche utilisateur `front`, `back`, `auxiliary` en `user` |
| `< 20241010` | Supprimer les champs graphiques `solder_mask_margin` |
| `< 20241030` | Convertir les champs booléens de dimension en atomes hérités ; supprimer la dimension `arrow_direction` |
| `< 20250210` | Supprimez le texte du PCB `render_cache` ; supprimer la zone de texte `knockout` ; supprimez les atomes `knockout` des listes de couches ; ajoutez `filled_areas_thickness no` aux remplissages de zones mises en cache si nécessaire |
| `< 20241009` | Supprimer les champs de la zone `placement` |
| `<= 20221018` | Supprimer la zone `attr` ; supprimer le pad/zone `thermal_bridge_angle` ; renommer le pad/zone `thermal_bridge_width` en ancien `thermal_width` |
| `< 20240108` | Supprimez `setup/allow_soldermask_bridges_in_footprints` ; supprimer le groupe `locked` ; supprimer via des champs de connexion de couche tels que `keep_end_layers`, `start_end_only` et `zone_layer_connections` |
| `< 20241007` | Supprimer les champs `solder_mask_margin` et `solder_mask_layer` de la piste |
| `< 20240617` | Supprimer la cellule du tableau PCB `angle` |
| `< 20260521` | Supprimer le tampon `sim_electrical_type` |
| `< 20250228` | Convertir les listes booléennes avant/arrière en tentes en atomes hérités ; supprimer les champs de protection IPC-4761 |
| `< 20231212` | Convertissez les listes booléennes `locked` et `hide` en atomes de présence ; supprimer `unlocked` ; supprimer le modèle `hide` |
| `< 20231014` | Supprimer `generator_version` |
| `< 20230924` | Convertissez les booléens `pcbplotparams` `yes/no` en `true/false` ; convertir le remplissage de forme `no` en `none` |
| `< 20230730` | Supprimer la forme graphique `net` connectivité |
| `< 20240108` | Convertir les listes booléennes de polices grasses/italiques en atomes hérités |
| `< 20230620` | Convertissez les propriétés d'empreinte `Reference` et `Value` en `fp_text` ; convertir `Description` en `ki_description` ; mapper `sheetname`/`sheetfile` aux propriétés |
| `< 20231231` | Renommez les champs `uuid` de portée en `tstamp` ; renommer le groupe/généré `uuid` en `id` |
| `< 20250324` | Supprimez les champs du cavalier d'empreinte : `duplicate_pad_numbers_are_jumpers` et `jumper_pad_groups` |
| `<= 20221018` | Supprimez les attributs d'empreinte `dnp`, `net_tie_pad_groups`, `units` et `allow_missing_courtyard` ; supprimer le pad/via `remove_unused_layers` ; convertir les dimensions en graphiques visibles ; supprimer `locked` incompatible avec l'héritage ; rétrograder gratuitement via les champs ; convertir les blocs graphiques `stroke` du PCB en champs `width` hérités |
| `< 20250309` | Supprimer `component_class` des règles de placement |
| `< 20250222` | Convertir les remplissages de forme de hachures de PCB/hachures inversées/hachures croisées en remplissage solide |
| `<= 20241229` | Supprimer les champs `face` de la police PCB |
| `< 20251101` | Supprimer le tampon/via les champs de post-usinage |
| `< 20251028` | Reconstruire les netcodes du tableau numérique hérité et les déclarations nettes au niveau racine |

Correctifs spécifiques à l'analyseur KiCad 6 observés dans les tests au niveau du projet :

| Zone | Correctif implémenté |
| --- | --- |
| Configuration du PCB | Supprimez `setup/allow_soldermask_bridges_in_footprints` pour les cibles de carte pré-8. |
| Empreintes de PCB | Supprimez `net_tie_pad_groups`, `units`, les groupes de cavaliers et les atomes d'attr `allow_missing_courtyard` pour les cibles de la carte KiCad 6/7. |
| Zones et plots PCB | Supprimez la zone `attr`, supprimez `thermal_bridge_angle` et renommez `thermal_bridge_width` en `thermal_width` pour les cibles de la carte KiCad 6/7. |
| Texte et tableaux des PCB | Supprimez le texte `render_cache`, la zone de texte `knockout`, la cellule de tableau `knockout` et la liste de couches `knockout` là où les anciens analyseurs les rejettent. |
| Bibliothèques de symboles | Supprimez le symbole `exclude_from_sim` pour les cibles antérieures à `20230409` et ajoutez les ID de propriété standard KiCad 6. |
| Schémas | Supprimez la broche `alternate`, générez des tables d'instance racine KiCad 6, normalisez les chemins d'instance de feuille racine, normalisez les noms/ID de propriété de feuille et supprimez `instances` interne au symbole. Les blocs UUID de broche de symbole placés sont intentionnellement conservés car KiCad 6 les utilise pour l'association d'instances. |
| Fichiers annexes du projet | Générez les paramètres d'affichage de l'ID numérique `.kicad_prl` pour V6/V7/V8 et supprimez les nœuds `version` de niveau supérieur de la table de bibliothèque pour V6. |

### Feuille de travail et règles de conception

La gestion des feuilles de calcul a actuellement une porte d'analyse implémentée :

| Seuil cible | Récrire |
| ---: | --- |
| `< 20220228` | Supprimer les blocs de la feuille de calcul `font` |

Les règles de conception sont détectées et ont des alias de version cible, mais pas de rétrogradation
les réécritures sont actuellement implémentées car la macro de version du format de fichier reste
`20200610` dans les versions KiCad suivies.

### Sémantique des avertissements et des rapports

Chaque suppression ou réécriture de compatibilité implémentée qui modifie l'arborescence ajoute un
avertissement. Les portes de fonctionnalités génériques signalent le nombre de nœuds supprimés et le
version introductive. Les rapports incluent le chemin, le type détecté, la version source,
version cible, indicateur modifié et avertissements.

## Exigences du convertisseur

### Lire le chemin

- Conservez le fichier source `version` ; ne pas interpréter uniquement comme le courant
Format KiCad.
- Prise en charge des alias de compatibilité pour les fichiers plus anciens :
  - `page` à `paper`
  - Barre supérieure héritée `~...~` à `~{...}`
  - Ancien format de zone de texte `start/end` vers le nouveau `at/size`
  - Ancien `id` à `uuid`
  - Anciens formats booléens/jetons de présence vers des booléens explicites
- Détectez les futurs formats et renvoyez une erreur claire ou une stratégie de rétrogradation définie.

### Chemin d'écriture

- `--target-version` doit faire plus que changer le numéro de version de niveau supérieur. Il
doit élaguer ou réécrire la sémantique en fonction de la cible demandée.
- Chaque version cible nécessite des portes de fonctionnalités :
  - KiCad 6 ne doit pas écrire de champs de modèle de simulation V7, de DNP ou de zone de texte post-V6
structures.
  - KiCad 7 ne doit pas écrire des structures qui ne sont devenues stables qu'après la V8
Nettoyage `generator_version`.
  - KiCad 8 ne doit pas écrire de fichiers embarqués V9, de classes de composants ou de composants complexes.
piles de tampons.
  - KiCad 9 ne doit pas écrire de variantes V10, de codes-barres, de backdrill, de type split via et
des constructions similaires.
  - KiCad 10 ne doit pas écrire de métadonnées de corps extrudé en cours de développement, natives
ellipses, champs de fréquence diélectrique, chaînes de filets, vol de cuivre, tampon
types électriques de simulation ou indicateurs d'inactivation des cellules de table.
- Les rétrogradations avec perte devraient produire des avertissements ou des métadonnées annexes au lieu d'être silencieuses
effacement.

### Chemin de test

- Créez des appareils minimaux pour KiCad 6, 7, 8, 9 et 10 :
  - Bibliothèque de symboles
  - Schématique
  - Conseil
  - Empreinte
  - Feuille de travail
  - Règles de conception
- Chaque conversion multiversion doit vérifier :
  - La version source est lue correctement
  - Le numéro de version cible est écrit correctement
  - Les jetons non autorisés sont supprimés ou déclassés
  - La sémantique clé est préservée
  - Les avertissements couvrent les conversions avec perte

## Notes d'entretien

Lors de l'ajout de différences de versions futures :

1. Ajoutez ou mettez à jour la matrice de versions en premier.
2. Ajoutez une nouvelle section d'intervalle telle que `10.0 to 11.0` ou
`10.99 / current to 11.99 / current`.
3. Gardez les résultats de la branche de développement séparés des balises stables publiées jusqu'à ce que
La version KiCad correspondante est étiquetée.
4. Mettre à jour le résumé de la cible de rétroportage lorsqu'une nouvelle version source est introduite
constructions qui affectent les objectifs de déclassement existants.
5. Suivez les migrations de schémas JSON `.kicad_pro` dans un document séparé.

# Différences de format et comportement du convertisseur KiCad Backport

Ce document décrit les différences de formats KiCad réellement prises en charge par l’implémentation actuelle de `kicad-backport` : familles de fichiers, ancres de version, dispatch de conversion, règles de réécriture et chemins avec perte. La référence normative des formats KiCad reste la documentation développeur KiCad.

- https://dev-docs.kicad.org/en/file-formats/index.html

## Lecture rapide pour les développeurs KiCad

| Sujet | Comportement du convertisseur |
| --- | --- |
| Détection du format | Les fichiers modernes sont identifiés d’abord par le root token S-expression : `kicad_sch`, `kicad_symbol_lib`, `kicad_pcb`, `footprint`, `kicad_wks` / `drawing_sheet`. L’extension est un fallback. |
| Détection de version | Les fichiers S-expression modernes lisent le champ top-level `(version ...)`. `.kicad_pro` est rapporté comme `kicad-project-json`. Les fichiers legacy `.sch/.lib/.dcm/.pro` utilisent des alias legacy. |
| Frontière KiCad 5/6 | KiCad 6 est la frontière de famille pour les schémas, bibliothèques de symboles et projets. Les `.sch/.lib/.pro` KiCad 4/5 et les `.kicad_sch/.kicad_sym/.kicad_pro` KiCad 6+ ne partagent pas la même syntaxe. |
| PCB et empreintes | Les boards et footprints KiCad 4-10 sont traités comme des S-expressions. Les différences importantes sont les ancres de version, les ensembles de nœuds et la syntaxe des champs. |
| `.kicad_pro` | Le JSON projet moderne n’est pas réécrit par version majeure cible. Pour KiCad 6+ il est normalement copié brut ; pour KiCad 5/4 un `.pro` minimal est généré. |
| `.kicad_wks` | Les worksheets sont détectées et leur version peut être réécrite. Il existe seulement une petite règle de downgrade worksheet, sans writer legacy KiCad 4/5. |
| `.kicad_dru` | Le code peut le détecter avec l’ancre fixe `20200610`, mais il n’est pas listé comme famille principale visible utilisateur dans ce document. |

## Modèle d’implémentation

| Étape | Implémentation | Signification |
| --- | --- | --- |
| Lecture | `loadDocumentImpl()` lit le texte, route `.kicad_pro` et les fichiers legacy par extension, puis parse les autres fichiers comme S-expressions. | Évite de parser JSON ou texte legacy comme S-expression. |
| Détection du type | `DetectKind()` privilégie le root token et utilise l’extension comme fallback. | Un fichier S-expression correctement raciné peut être accepté même avec un nom atypique. |
| Résolution cible | `ResolveTargetVersion()` mappe chaque alias KiCad vers une version propre à chaque type de fichier. | Une release KiCad n’a pas une version de format unique pour tous les fichiers. |
| Extension de sortie | `withTargetFamilyExtension()` bascule `.sch/.lib/.pro` et `.kicad_sch/.kicad_sym/.kicad_pro` à la frontière KiCad 5/6. | La conversion 5/6 n’est pas une simple modification de `(version ...)`. |
| Même version | Si les versions S-expression source et cible sont identiques, le fichier est copié ou laissé inchangé. | Évite les réécritures inutiles. |
| Upgrade | Si la source est plus ancienne que la cible, `ApplyUpgradeRules()` effectue une normalisation conservatrice. | Aucune intention de conception nouvelle n’est inventée. |
| Downgrade | Si la source est plus récente que la cible, `ApplyDowngradeRules()` supprime, renomme, aplatit ou approxime. | Les anciens parseurs KiCad ne rencontrent pas de nœuds inconnus. |

## Ancres de versions cibles

| Cible | Symbol library | Schematic | Board | Footprint | Worksheet | Notes |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `4.0` | legacy `.lib` 2.3 | legacy `.sch` V2 | `4` | `4` | non défini | Schematic/symbol passent par les writers legacy. |
| `5.0` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | non défini | Même ancre PCB/footprint que `5.1`. |
| `5.1` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | non défini | Identique à `5.0`. |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | Début des familles modernes schematic/symbol. |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | Extensions S-expression modernes. |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | Frontière `generator_version`, UUID/id et PCB fields. |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | Données embarquées, tables, rule areas et objets PCB complexes. |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | Plus haut alias cible régulier de l’implémentation. |
| `10.99` | `20251024` | `20260306` | `20260603` | `20260603` | `20231118` | Alias de développement ; seuls board/footprint avancent au-delà de `10.0`. |

## Conversion par famille

| Fichier | Comportement | Limite |
| --- | --- | --- |
| `.kicad_pro` | Copie JSON brute pour KiCad 6+, `.pro` minimal pour KiCad 5/4. | Pas de réécriture JSON par version majeure cible. |
| legacy `.pro` | Génère un `.kicad_pro` JSON minimal pour KiCad 6+. | Préserve seulement les settings legacy et noms de bibliothèques reconnus. |
| `.kicad_sch` | Writer legacy `.sch` pour KiCad 5/4 ; règles S-expression pour KiCad 6+. | Les propriétés, instances et objets modernes complexes sont avec perte en legacy. |
| legacy `.sch` | Conversion vers `.kicad_sch` pour KiCad 6+ ; réécriture d’en-tête pour KiCad 5/4. | Les dessins legacy non-wire ne sont pas entièrement mappés. |
| `.kicad_sym` | Écrit `.lib` et `.dcm` pour KiCad 5/4. | Propriétés, graphismes et symboles imbriqués modernes sont approximés. |
| legacy `.lib/.dcm` | Génère `.kicad_sym` pour KiCad 6+. | `.dcm` seul devient un squelette de symboles, metadata documentaire incomplète. |
| `.kicad_pcb/.kicad_mod` | Reste S-expression ; réécrit version, nœuds et champs. | Champs géométriques, électriques, fabrication ou cache non supportés sont supprimés ou approximés. |
| `.kicad_wks` | Version rewrite et règle worksheet limitée pour KiCad 6+. | Pas de writer worksheet legacy KiCad 4/5. |

## Politique de downgrade

| Cas | Choix d’implémentation | Exemples |
| --- | --- | --- |
| Le parseur cible ne lit pas un nœud | Suppression du nœud/champ avec warning. | `embedded_files`, `variants`, `barcodes`, `net_chains`, ellipse native. |
| Une représentation ancienne existe | Renommage, aplatissement ou champ legacy. | `directive_label -> netclass_flag`, `stroke -> width`, `uuid/tstamp/id`. |
| Géométrie cible plus faible | Conversion en primitives plus simples. | Rectangles PCB en lignes, arcs de pistes en segments, custom pads en pads rectangulaires. |
| Layout de propriétés ancien | Déplacement de propriété, ajout ou suppression d’ID. | Position de property hide, standard property id. |
| Pas de sémantique nouvelle dans la source | Aucun nouvel objet fonctionnel n’est créé. | Pas de padstack, variants, component classes ni barcodes automatiques. |

## Conversion de projet

| Traitement | Implémentation |
| --- | --- |
| Entrée | Un dossier ou un `.kicad_pro` est traité comme project tree. |
| Fichiers copiés | Documents KiCad, documents legacy, library tables, `.kicad_prl`, modèles 3D. |
| Dossiers ignorés | VCS, history, backup, archive, gerber/fabrication/output/plot/BOM/assembly/vendor output. |
| Extensions | KiCad 5/4 mappe `.kicad_sch/.kicad_sym/.kicad_pro` vers `.sch/.lib/.pro`; KiCad 6+ fait l’inverse. |
| `.dcm` | Si un `.lib` associé existe pour une cible KiCad 6+, `.dcm` n’est pas converti séparément. |
| Library tables | `sym-lib-table` / `fp-lib-table` sont normalisées pour la famille cible. |
| Support schematic | Pour KiCad 6+, les symboles locaux sont intégrés dans `lib_symbols` et les hierarchy instances sont reconstruites. |


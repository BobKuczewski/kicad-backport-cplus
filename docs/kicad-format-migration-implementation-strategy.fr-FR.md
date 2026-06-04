# Stratégie d'implémentation pour les migrations de format KiCad

Ce texte est synchronisé structurellement avec la référence anglaise ; les termes
techniques KiCad, tokens et noms de fichiers restent volontairement inchangés.

Ce document transforme les différences de format listées dans
`kicad-file-format-version-differences.md` en recommandations d'implémentation pour les
migrations entre versions majeures adjacentes de KiCad.

La frontière principale est KiCad 5 vers KiCad 6. KiCad 4 et 5 utilisent des fichiers
legacy pour les schémas et bibliothèques de symboles (`.sch`, `.lib`, `.dcm`), tandis que
KiCad 6 et les versions plus récentes utilisent des fichiers S-expression (`.kicad_sch`,
`.kicad_sym`). Les fichiers PCB et footprint sont déjà des fichiers S-expression sur ces
versions, mais leurs ensembles de fonctionnalités et numéros de version nécessitent quand
même des règles de réécriture explicites.

Dernière mise à jour : 2026-06-04.

## Périmètre

Ce document est une feuille de route d'implémentation, pas une promesse de support de
release. Il décrit comment implémenter :

- Les migrations vers l'avant comme KiCad 6 vers 7 et KiCad 5 vers 6.
- Les migrations arrière comme KiCad 7 vers 6, KiCad 6 vers 5 et KiCad 5 vers 4.
- Les points d'extension pour les versions adjacentes ultérieures comme 7 vers 8,
  8 vers 9, 9 vers 10 et 10 vers la branche de développement actuelle.

Le convertisseur C++ actuel est principalement un moteur de downgrade. Si la version du
format source est plus ancienne que la cible demandée, il copie actuellement le fichier
sans le modifier au lieu de l'upgrader. Le support d'upgrade doit donc être ajouté comme
chemin de migration séparé, sans affaiblir les règles de downgrade.

## Carte des versions cibles

| Cible KiCad | Bibliothèque de symboles | Schéma | PCB / footprint | Worksheet | Règles de conception |
| --- | ---: | ---: | ---: | ---: | ---: |
| 4.0 | Legacy `.lib` 2.3 | Legacy `.sch` V2 | `4` | Legacy drawing sheet | Aucune |
| 5.0 / 5.1 | Legacy `.lib` 2.4 | Legacy `.sch` V4 | `20171130` | Legacy drawing sheet | Aucune |
| 6.0 | `20211014` | `20211123` | `20211014` | `20210606` | `20200610` |
| 7.0 | `20220914` | `20230121` | `20221018` | `20220228` | `20200610` |
| 8.0 | `20231120` | `20231120` | `20240108` | `20231118` | `20200610` |
| 9.0 | `20241209` | `20250114` | `20241229` | `20231118` | `20200610` |
| 10.0 | `20251024` | `20260306` | `20260206` | `20231118` | `20200610` |

## Forme du moteur de migration

Implémenter les migrations comme un pipeline avec des adaptateurs de famille de fichiers :

1. Détecter le type de document par le root S-expression head ou par extension/header legacy.
2. Parser vers un arbre de document mutable ou un typed intermediate model.
3. Choisir une route de migration de la famille/version source vers la famille/version cible.
4. Appliquer les étapes de migration dans l'ordre. Chaque étape doit être gardée par le
   type de document, la version source et la version cible.
5. Écrire la famille de fichiers cible avec la version cible et l'extension cible.
6. Émettre des avertissements pour chaque suppression avec perte, valeur par défaut de
   repli ou conversion sémantique non implémentée.

Ne pas implémenter la migration comme une simple réécriture du numéro de version racine.
Chaque frontière de version peut introduire ou retirer des tokens, structures de mise en
page, attributs d'objets, voire des familles de fichiers entières.

## Règles entre familles

### Famille legacy KiCad 4/5

KiCad 4 et 5 exigent des writers legacy pour les sorties schéma et bibliothèque de symboles :

- Schéma : `.sch`, avec `EESchema Schematic File Version 2` pour KiCad 4 et
  `Version 4` pour KiCad 5.
- Bibliothèque de symboles : `.lib` plus `.dcm` optionnel, généralement
  `EESchema-LIBRARY Version 2.3` pour KiCad 4 et `Version 2.4` pour KiCad 5.
- Projet : `.pro` legacy.

Les données schéma et symbole V6+ ne peuvent pas être downgradées vers KiCad 4/5 en
supprimant quelques nodes S-expression. L'implémentation a besoin d'un vrai serializer
legacy ou d'un avertissement ferme indiquant que la conversion n'est pas supportée.

### Famille S-expression KiCad 6+

KiCad 6 et plus récent utilisent des fichiers S-expression pour schéma, bibliothèque de
symboles, PCB, footprint, worksheet et règles de conception. La plupart des migrations
adjacentes peuvent être gérées comme des réécritures d'arbre pilotées par version :

- Ajouter ou supprimer des feature nodes selon la version cible.
- Renommer les champs quand le format a changé.
- Convertir les formats boolean-list vers des legacy presence atoms, ou l'inverse.
- Normaliser les structures UUID, ID, font, text et property pour la cible.

## Stratégie de migration vers l'avant

La migration vers l'avant doit préserver la sémantique source et synthétiser uniquement des
valeurs par défaut sûres pour la cible. Elle ne doit pas inventer d'intention de conception.

Comportement recommandé :

- Si la source et la cible sont dans la même famille, parser puis écrire le format cible
  plus récent en appliquant les réécritures de compatibilité connues.
- Si un champ est absent dans la source, l'omettre lorsque le nouveau format le permet ;
  sinon écrire des valeurs par défaut proches de celles de KiCad.
- Si la migration traverse la frontière KiCad 4/5 legacy vers KiCad 6+ S-expression,
  utiliser des importers dédiés pour `.sch`, `.lib`, `.dcm` et `.pro`.
- Pour les imports legacy difficiles, utiliser KiCad lui-même comme oracle de référence :
  charger l'ancien projet dans la version KiCad correspondante ou dans KiCad 6+, sauvegarder,
  puis comparer la structure générée avec la sortie du convertisseur.

### Upgrade KiCad 6 vers KiCad 7

C'est un upgrade S-expression dans la même famille. Il peut être implémenté comme une
réécriture structurée suivie d'une mise à jour de version cible.

Actions clés :

- Mettre les versions cibles à symbol `20220914`, schematic `20230121`, board /
  footprint `20221018` et worksheet `20220228`.
- Préserver les objets V6 existants de schéma, symbole, board, footprint, worksheet et DRC.
- Ajouter des valeurs par défaut uniquement là où KiCad 7 exige une valeur que KiCad 6
  n'écrivait pas.
- Convertir les anciens champs compatibles vers la nouvelle orthographe si nécessaire,
  par exemple `netclass_flag` vers `directive_label`.
- Normaliser la géométrie des text boxes vers la représentation KiCad 7 `at` / `size`
  si une ancienne représentation `start` / `end` est rencontrée.
- Garder les informations de simulation V6 valides, mais ne pas fabriquer de données
  complètes de simulation model KiCad 7 sans champs suffisants.
- Préserver ou synthétiser les valeurs par défaut liées à DNP uniquement lorsque le type
  d'objet cible les prend en charge.
- Pour PCB et footprints, préserver les objets V6 et activer des formes compatibles V7
  pour stroke formatting, footprint private layers, dimensions, teardrop keywords,
  net ties et pad/via zone-layer connections lorsque les données suffisent.
- Pour les worksheets, écrire la version V7 et préserver les font data seulement si elles
  peuvent être représentées de manière compatible KiCad 7.

Fixtures de validation :

- Un schéma V6 avec labels, fields, hierarchical sheets et simulation fields.
- Un board V6 avec vias, pads, zones, dimensions et footprint text.
- Une bibliothèque de symboles V6 avec properties et primitives simples.

### Upgrade KiCad 5 vers KiCad 6

C'est la frontière de migration vers l'avant la plus importante, car les fichiers schéma et
symbole changent de famille.

Adaptateurs requis :

- Migration de projet `.pro` vers `.kicad_pro` JSON.
- Legacy `.sch` V4 vers `.kicad_sch` `20211123`.
- Legacy `.lib` / `.dcm` 2.4 vers `.kicad_sym` `20211014`.
- `.kicad_pcb` / `.kicad_mod` `20171130` vers `20211014`.
- Legacy drawing sheet vers `.kicad_wks` `20210606`, si la conversion worksheet est supportée.

Actions clés :

- Parser les records de schéma legacy dans un typed model avant d'écrire des
  S-expressions KiCad 6. Éviter les remplacements texte ligne par ligne pour `.sch`.
- Mapper les symboles de schéma legacy vers des symbol instances KiCad 6 avec UUIDs,
  properties, paths, sheet instances et library identifiers.
- Générer des UUIDs stables là où KiCad 6 les exige. Utiliser des UUIDs déterministes
  dérivés des chemins source et de l'identité objet quand la reproductibilité compte.
- Convertir les legacy library aliases, fields, drawing primitives, pins et documentation
  records en symboles `.kicad_sym`.
- Préserver les descriptions, keywords et documentation links `.dcm` comme métadonnées de
  symbole KiCad 6 lorsque c'est possible.
- Convertir les legacy project settings vers `.kicad_pro` et `.kicad_prl` uniquement pour
  les réglages ayant des équivalents clairs. Avertir pour les réglages UI ou cache ignorés.
- Upgrader la version PCB à `20211014` tout en préservant les fonctionnalités KiCad 5 :
  custom pads, multi-layer keepouts, 3D model offsets et footprint text lock state.

Zones de perte attendues :

- Certains legacy project settings peuvent ne pas avoir d'équivalent JSON KiCad 6.
- Le comportement legacy library rescue/cache peut nécessiter un remapping explicite des symboles.
- Les constructions de schéma V5 dépendant des anciennes règles de lookup de bibliothèques
  peuvent nécessiter des avertissements ou des données sidecar de bibliothèque de symboles.

## Stratégie de migration arrière

La migration arrière doit supprimer ou réécrire les constructions non supportées et signaler
la perte. Le fichier cible doit être chargeable par la version KiCad demandée même si une
sémantique plus récente ne peut pas être préservée.

Comportement recommandé :

- Appliquer les règles de downgrade de la plus récente à la plus ancienne, afin de retirer
  les features tardives avant les réécritures de compatibilité plus anciennes.
- Garder les avertissements précis : nommer la feature retirée, compter les nodes affectés
  et inclure la version d'introduction.
- Pour les downgrades S-expression dans la même famille, préserver l'identité des objets
  quand la cible la supporte.
- Pour les downgrades schéma ou symbole de V6+ vers V5/V4, utiliser un writer legacy dédié
  et traiter les objets V6+ non supportés comme des pertes.

### Downgrade KiCad 7 vers KiCad 6

C'est un downgrade S-expression dans la même famille. Il doit être implémenté comme une
suppression ciblée et une réécriture de compatibilité.

Versions cibles :

- Bibliothèque de symboles : `20211014`.
- Schéma : `20211123`.
- Board / footprint : `20211014`.
- Worksheet : `20210606`.
- Design rules : `20200610`.

Règles clés :

- Supprimer symbol class flags, symbol fonts, text boxes, text colors, unit display names
  et le comportement KiCad 7-only property-ID.
- Supprimer les schematic graphic primitives introduites après V6 sans équivalent V6,
  y compris text boxes et nouveaux label fields.
- Réécrire `directive_label` vers la forme V6-compatible `netclass_flag` si une
  correspondance fidèle existe ; sinon avertir.
- Supprimer ou downgrader dash-dot-dot line style, text hyperlinks, field name visibility,
  do-not-autoplace options, DNP support et V7 simulation model fields.
- Déplacer ou simplifier les symbol and sheet instance data vers la structure attendue par KiCad 6.
- Supprimer PCB text boxes, image objects, first-class net ties, V7 teardrop keywords,
  footprint private-layer changes que V6 ne peut pas parser, et pad/via
  zone-layer-connection additions.
- Convertir les formats V7 stroke, font, boolean, lock et hide vers des formes compatibles V6.
- Supprimer le worksheet font support ajouté en `20220228`.

Zones de perte attendues :

- Text boxes, images, net ties, DNP flags, hyperlinks et modern simulation model data peuvent
  ne pas avoir d'équivalent V6 fidèle.
- Certains V7 PCB dimensions et teardrop settings doivent être supprimés ou aplatis.

### Downgrade KiCad 6 vers KiCad 5

C'est un downgrade entre familles pour les fichiers schéma et symbole.

Writers requis :

- `.kicad_sch` `20211123` vers legacy `.sch` V4.
- `.kicad_sym` `20211014` vers legacy `.lib` 2.4 plus `.dcm`.
- `.kicad_pro` JSON vers legacy `.pro`.
- `.kicad_pcb` / `.kicad_mod` `20211014` vers `20171130`.

Règles clés :

- Sérialiser les KiCad 6 schematic symbols, wires, buses, labels, junctions, sheets, fields
  et page settings dans le format de records legacy `.sch`.
- Convertir les UUIDs et paths KiCad 6 en timestamps legacy ou identifiers déterministes
  lorsque la cible l'exige.
- Séparer les KiCad 6 symbol metadata en `.lib` symbol definitions et `.dcm`
  documentation records.
- Supprimer les structures schéma et symbole KiCad 6-only que les fichiers legacy ne peuvent
  pas représenter directement. Avertir pour chaque suppression.
- Convertir les settings `.kicad_pro` vers `.pro` seulement s'il existe un équivalent legacy
  connu. Ignorer ou avertir pour les settings JSON-only.
- Downgrader les versions board et footprint à `20171130`.
- Supprimer les features V6 board/footprint que KiCad 5 ne peut pas parser, y compris V6+
  UUID-only structures, newer zone behavior, rule file assumptions et tout objet introduit
  après le format PCB V5.
- Préserver les features PCB compatibles KiCad 5 : custom pads, multi-layer keepouts,
  3D model offset en mm avec le paramètre `offset`, et locked footprint text.

Zones de perte attendues :

- Les détails KiCad 6 schéma et symbole S-expression ne se mappent pas toujours aux champs
  de records legacy.
- Les settings JSON de projet, l'identité UUID moderne et certains détails de linkage de
  bibliothèque peuvent être réduits ou supprimés.
- Les fichiers `.kicad_dru` n'ont pas d'équivalent standalone KiCad 5.

### Downgrade KiCad 5 vers KiCad 4

C'est principalement un downgrade dans la famille legacy, mais le PCB nécessite encore la
suppression de features S-expression.

Formats cibles :

- Schéma : `.sch` V2.
- Bibliothèque de symboles : `.lib` 2.3 plus `.dcm`.
- PCB / footprint : version `4`.
- Projet : legacy `.pro`.

Règles clés :

- Réécrire le header de schéma de `EESchema Schematic File Version 4` vers `Version 2`.
- Supprimer ou réécrire les records schéma KiCad 5 que KiCad 4 ne peut pas parser.
- Downgrader les headers de bibliothèque de symboles d'une sortie style 2.4 vers style 2.3.
- Supprimer les symbol fields ou attributes absents des bibliothèques KiCad 4.
- Downgrader les fichiers board et footprint de `20171130` vers version `4`.
- Supprimer les features PCB KiCad 5 introduites après KiCad 4, y compris differential pair
  settings per net class, long pad names, custom pad shape details, multi-layer keepouts,
  3D model offset changes non interprétables par KiCad 4, et locked/unlocked footprint text.
- Convertir les 3D model offsets vers la représentation KiCad 4 lorsqu'une conversion
  d'unité réversible est connue ; sinon avertir et retirer les champs offset non supportés.

Zones de perte attendues :

- Custom pads et multi-layer keepouts peuvent devoir être simplifiés ou supprimés.
- Long pad names peuvent nécessiter une troncature ou un renommage déterministe.
- Certaines métadonnées net-class et footprint text-lock ne peuvent pas être préservées.

## Modèles de downgrade adjacents ultérieurs

Le même framework de downgrade doit couvrir les versions adjacentes ultérieures :

| Route | Focus principal d'implémentation |
| --- | --- |
| KiCad 8 vers 7 | Supprimer V8 generator cleanup fields, PCB fields, generated objects, UUID normalization, custom pad spoke templates, modern teardrops, images, rule-area changes, simulation/exclude flags et worksheet embedded images. |
| KiCad 9 vers 8 | Supprimer embedded files, tables, rule areas, component classes, complex padstacks, via stacks, arbitrary user layers, tenting, multiple netclass assignments et netclass color highlighting. |
| KiCad 10 vers 9 | Supprimer variants, jumper pads, barcodes, via protection, zone hatch offsets, backdrill, split via types, stopped-netcode assumptions, rounded rectangles, stacked pins, PCB points et property-formatting updates. |
| Développement actuel vers KiCad 10 | Supprimer les features post-10.0 : extruded 3D body metadata, native ellipses and ellipse arcs, dielectric frequency-dependent stackup fields, net chains, copper thieving, pad `sim_electrical_type` et PCB table-cell `knockout`. |

Les migrations vers l'avant pour ces mêmes routes sont généralement moins destructrices que
les downgrades, mais nécessitent toujours des réécritures de compatibilité et des valeurs
par défaut cibles. Par exemple, KiCad 7 vers 8 ne doit introduire le traitement
V8-normalized `generator_version` que là où le format cible l'attend, et KiCad 8 vers 9 ne
doit pas inventer embedded files, component classes ou padstacks sans sémantique source.

## Stratégie de test

Créer un jeu de fixtures par route adjacente et par type de document :

- Fichiers projet : `.pro` et `.kicad_pro`.
- Fichiers schéma : legacy `.sch` et `.kicad_sch`.
- Bibliothèques de symboles : `.lib`, `.dcm` et `.kicad_sym`.
- Fichiers PCB : `.kicad_pcb`.
- Footprints : `.kicad_mod`.
- Worksheets : legacy drawing sheet et `.kicad_wks`.
- Design rules : `.kicad_dru` uniquement pour KiCad 6+.

Chaque test doit vérifier :

- Le type source et la version source sont correctement détectés.
- La famille de fichier cible, l'extension et la version sont correctes.
- Les tokens interdits par la version cible sont absents.
- Les structures compatibles connues sont préservées.
- Les changements avec perte produisent des avertissements.
- Réexécuter la même migration est stable et ne modifie pas encore le fichier.

Pour les routes V5/V6 et V6/V5, ajouter si possible des golden-file tests générés par KiCad
lui-même. Ces routes traversent une frontière de famille de fichiers et présentent le plus
grand risque de dérive sémantique.

## Ordre d'implémentation

Ordre recommandé :

1. Terminer les downgrades S-expression same-family de V7 vers V6.
2. Ajouter le downgrade PCB / footprint V5 vers V4, car il reste dans la famille PCB
   S-expression.
3. Implémenter les parsers legacy schéma et symbole pour l'upgrade V5 vers V6.
4. Implémenter les writers legacy schéma et symbole pour le downgrade V6 vers V5.
5. Ajouter le downgrade legacy schéma et symbole V5 vers V4.
6. Ajouter les upgrades same-family comme V6 vers V7.
7. Étendre le même framework aux routes V8, V9, V10 et développement actuel.

Cet ordre livre tôt une couverture de downgrade utile tout en isolant le travail beaucoup
plus difficile de conversion legacy des schémas et symboles.

# KiCad フォーマット移行の実装方針

この文書は英語版リファレンスと同じ構造で同期しています。KiCad の技術用語、
token、ファイル名は誤訳を避けるため意図的にそのまま残しています。

この文書は `kicad-file-format-version-differences.md` の format difference を、
隣接する KiCad major version 間の migration 実装方針に落とし込むものです。

最も重要な境界は KiCad 5 から KiCad 6 です。KiCad 4 と 5 は schematic と
symbol library に legacy file (`.sch`, `.lib`, `.dcm`) を使います。一方、KiCad 6
以降は S-expression file (`.kicad_sch`, `.kicad_sym`) を使います。PCB と footprint
file はこれらの version 全体で S-expression file ですが、feature set と version
number には明示的な rewrite rule が必要です。

最終更新: 2026-06-04。

## 範囲

これは実装ロードマップであり、release support の約束ではありません。対象は次の通りです。

- KiCad 6 から 7、KiCad 5 から 6 のような forward migration。
- KiCad 7 から 6、KiCad 6 から 5、KiCad 5 から 4 のような backward migration。
- 7 から 8、8 から 9、9 から 10、10 から current development branch のような
  後続の隣接 version に対する extension point。

現在の C++ converter は主に downgrade engine です。source format version が要求された
target より古い場合、現在は upgrade せずに file をそのまま copy します。そのため upgrade
support は downgrade rule を弱めるのではなく、別の migration path として追加すべきです。

## Target Version Map

| KiCad target | Symbol library | Schematic | Board / footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: |
| 4.0 | Legacy `.lib` 2.3 | Legacy `.sch` V2 | `4` | Legacy drawing sheet | なし |
| 5.0 / 5.1 | Legacy `.lib` 2.4 | Legacy `.sch` V4 | `20171130` | Legacy drawing sheet | なし |
| 6.0 | `20211014` | `20211123` | `20211014` | `20210606` | `20200610` |
| 7.0 | `20220914` | `20230121` | `20221018` | `20220228` | `20200610` |
| 8.0 | `20231120` | `20231120` | `20240108` | `20231118` | `20200610` |
| 9.0 | `20241209` | `20250114` | `20241229` | `20231118` | `20200610` |
| 10.0 | `20251024` | `20260306` | `20260206` | `20231118` | `20200610` |

## Migration Engine の形

migration は file-family adapter を持つ pipeline として実装します。

1. root S-expression head、または legacy extension/header で document kind を検出する。
2. mutable document tree または typed intermediate model に parse する。
3. source family/version から target family/version への migration route を選ぶ。
4. 順序付き migration step を適用する。各 step は document kind、source version、
   target version で guard する。
5. target file family、target version、target extension で書き出す。
6. lossy removal、fallback default、未実装の semantic conversion ごとに warning を出す。

migration を top-level version number の書き換えだけで実装してはいけません。各 version
boundary は token、layout structure、object attribute、または file family 全体を追加・削除
する可能性があります。

## Cross-Family Rules

### KiCad 4/5 Legacy Family

KiCad 4 と 5 の schematic と symbol library output には legacy writer が必要です。

- Schematic: `.sch`。KiCad 4 は `EESchema Schematic File Version 2`、KiCad 5 は
  `Version 4`。
- Symbol library: `.lib` と optional `.dcm`。一般的には KiCad 4 が
  `EESchema-LIBRARY Version 2.3`、KiCad 5 が `Version 2.4`。
- Project: legacy `.pro`。

V6+ schematic と symbol data は、いくつかの S-expression node を削除するだけでは KiCad
4/5 に downgrade できません。実装には本物の legacy serializer が必要です。未対応の場合は、
conversion が support されていないことを明確な warning として出すべきです。

### KiCad 6+ S-Expression Family

KiCad 6 以降は schematic、symbol library、PCB、footprint、worksheet、design rules に
S-expression file を使います。ほとんどの隣接 migration は version-gated tree rewrite として
扱えます。

- target version に応じて feature node を追加または削除する。
- format が変更された field を rename する。
- boolean-list format と legacy presence atom を相互変換する。
- target に合わせて UUID、ID、font、text、property structure を normalize する。

## Forward Migration Strategy

forward migration は source semantics を保持し、安全な target default だけを合成すべきです。
新しい design intent を作り出してはいけません。

推奨動作:

- source と target が同じ file family の場合、parse して既知の compatibility rewrite を適用し、
  より新しい target format で書き出す。
- source に format field がない場合、新 format が省略を許すなら省略する。そうでなければ
  KiCad-like default を書く。
- KiCad 4/5 legacy file から KiCad 6+ S-expression file へ跨ぐ場合、`.sch`、`.lib`、
  `.dcm`、`.pro` 用の dedicated importer を使う。
- 難しい legacy import では KiCad 自身を reference oracle として使う。古い project を該当
  KiCad version または KiCad 6+ で開いて保存し、生成構造を converter output と比較する。

### KiCad 6 から KiCad 7 への Upgrade

これは同一 family 内の S-expression upgrade です。structured rewrite と target version update
として実装できます。

主な処理:

- target version を symbol `20220914`、schematic `20230121`、board / footprint
  `20221018`、worksheet `20220228` に更新する。
- 既存の V6 schematic、symbol、board、footprint、worksheet、DRC object を保持する。
- KiCad 7 が必要とし、KiCad 6 が書かなかった値に限って default を追加する。
- 必要に応じて古い互換 field を新しい spelling に変換する。例: `netclass_flag` から
  `directive_label`。
- 古い `start` / `end` 表現の text box がある場合、KiCad 7 の `at` / `size` 表現へ
  normalize する。
- V6 simulation information は有効に保つ。ただし十分な field がない場合は完全な KiCad 7
  simulation model data を作らない。
- DNP 関連 default は target object type が対応する場合のみ保持または合成する。
- PCB と footprint では V6 object を保持し、十分な data がある場合に stroke formatting、
  footprint private layers、dimensions、teardrop keywords、net ties、pad/via
  zone-layer connections を V7-compatible form にする。
- worksheet では V7 worksheet version を書き、font data は KiCad 7-compatible に表現できる
  場合だけ保持する。

Validation fixtures:

- labels、fields、hierarchical sheets、simulation fields を含む V6 schematic。
- vias、pads、zones、dimensions、footprint text を含む V6 board。
- properties と simple primitives を含む V6 symbol library。

### KiCad 5 から KiCad 6 への Upgrade

これは forward migration で最も重要な境界です。schematic と symbol file が file family を
変更するためです。

必要な adapter:

- `.pro` から `.kicad_pro` JSON project migration。
- Legacy `.sch` V4 から `.kicad_sch` `20211123`。
- Legacy `.lib` / `.dcm` 2.4 から `.kicad_sym` `20211014`。
- `.kicad_pcb` / `.kicad_mod` `20171130` から `20211014`。
- worksheet conversion を support する場合、legacy drawing sheet から `.kicad_wks` `20210606`。

主な処理:

- KiCad 6 S-expression を書く前に、legacy schematic records を typed model に parse する。
  `.sch` file で line-oriented text replacement を使わない。
- legacy schematic symbol を UUIDs、properties、paths、sheet instances、
  library identifiers を持つ KiCad 6 symbol instances に map する。
- KiCad 6 が要求する場所では stable UUID を生成する。reproducibility が重要な場合は source
  path と object identity から deterministic UUID を導出する。
- legacy library aliases、fields、drawing primitives、pins、documentation records を
  `.kicad_sym` symbols に変換する。
- `.dcm` descriptions、keywords、documentation links を可能な限り KiCad 6 symbol metadata として保持する。
- legacy project settings は明確な対応がある場合だけ `.kicad_pro` と `.kicad_prl` に変換する。
  ignored UI/cache settings には warning を出す。
- PCB version を `20211014` に upgrade しつつ、custom pads、multi-layer keepouts、
  3D model offsets、footprint text lock state など KiCad 5 board features を保持する。

想定される lossy area:

- 一部の legacy project settings には KiCad 6 JSON equivalent がない可能性がある。
- Legacy library rescue/cache behavior には explicit symbol remapping が必要な場合がある。
- 古い library lookup rule に依存する V5 schematic constructs は warning または symbol-library
  sidecar data が必要になる可能性がある。

## Backward Migration Strategy

backward migration は unsupported constructs を削除または rewrite し、loss を報告すべきです。
一部の新しい semantics を保持できない場合でも、target file は要求された KiCad version で
load できる必要があります。

推奨動作:

- downgrade rule は newest から oldest の順に適用し、後の feature を古い compatibility rewrite
  より先に削除する。
- warning は具体的にする。削除した feature 名、affected nodes 数、introduction version を含める。
- same-family S-expression downgrade では、target が support する範囲で object identity を保持する。
- V6+ から V5/V4 への schematic または symbol downgrade では dedicated legacy writer を使い、
  unsupported V6+ object を lossy として扱う。

### KiCad 7 から KiCad 6 への Downgrade

これは同一 family 内の S-expression downgrade です。targeted removal と compatibility rewrite
として実装します。

Target versions:

- Symbol library: `20211014`。
- Schematic: `20211123`。
- Board / footprint: `20211014`。
- Worksheet: `20210606`。
- Design rules: `20200610`。

主な downgrade rule:

- symbol class flags、symbol fonts、text boxes、text colors、unit display names、
  KiCad 7-only property-ID behavior を削除する。
- V6 以降に導入され V6 equivalent がない schematic graphic primitives を削除する。text boxes
  と newer label fields を含む。
- faithful mapping がある場合は `directive_label` を V6-compatible `netclass_flag` form に戻す。
  ない場合は warning を出す。
- dash-dot-dot line style、text hyperlinks、field name visibility、do-not-autoplace options、
  DNP support、V7 simulation model fields を削除または downgrade する。
- symbol and sheet instance data を KiCad 6 が期待する layout に移動または単純化する。
- PCB text boxes、image objects、first-class net ties、V7 teardrop keywords、V6 が parse できない
  footprint private-layer changes、pad/via zone-layer-connection additions を削除する。
- V7 stroke、font、boolean、lock、hide format を V6-compatible form に変換する。
- `20220228` で追加された worksheet font support を削除する。

想定される lossy area:

- Text boxes、images、net ties、DNP flags、hyperlinks、modern simulation model data には
  faithful V6 equivalent がない可能性がある。
- 一部の V7 PCB dimensions と teardrop settings は削除または flatten する必要がある。

### KiCad 6 から KiCad 5 への Downgrade

これは schematic と symbol file の cross-family downgrade です。

必要な writer:

- `.kicad_sch` `20211123` から legacy `.sch` V4。
- `.kicad_sym` `20211014` から legacy `.lib` 2.4 plus `.dcm`。
- `.kicad_pro` JSON から legacy `.pro`。
- `.kicad_pcb` / `.kicad_mod` `20211014` から `20171130`。

主な downgrade rule:

- KiCad 6 schematic symbols、wires、buses、labels、junctions、sheets、fields、
  page settings を legacy `.sch` record format に serialize する。
- KiCad 6 UUIDs と paths を、target が必要とする legacy timestamps または deterministic
  identifiers に変換する。
- KiCad 6 symbol metadata を `.lib` symbol definitions と `.dcm` documentation records に分割する。
- legacy file が直接表現できない KiCad 6-only schematic と symbol structures を削除し、各削除に
  warning を出す。
- `.kicad_pro` settings は既知の legacy equivalent がある場合のみ `.pro` に変換する。
  JSON-only settings は ignore または warning。
- board と footprint version を `20171130` に downgrade する。
- KiCad 5 が parse できない V6 board/footprint features を削除する。V6+ UUID-only structures、
  newer zone behavior、rule file assumptions、V5 PCB format 以降に導入された object を含む。
- KiCad 5-compatible PCB features を保持する。custom pads、multi-layer keepouts、
  `offset` parameter を使う mm 単位の 3D model offset、locked footprint text。

想定される lossy area:

- KiCad 6 schematic と symbol S-expression details は legacy record fields に常に map できるとは限らない。
- Project JSON settings、modern UUID identity、一部の library linkage details は削減または削除される可能性がある。
- `.kicad_dru` file には KiCad 5 standalone equivalent がない。

### KiCad 5 から KiCad 4 への Downgrade

これは主に legacy-family downgrade ですが、PCB では引き続き S-expression feature removal が必要です。

Target formats:

- Schematic: `.sch` V2。
- Symbol library: `.lib` 2.3 plus `.dcm`。
- PCB / footprint: version `4`。
- Project: legacy `.pro`。

主な downgrade rule:

- schematic header を `EESchema Schematic File Version 4` から `Version 2` に rewrite する。
- KiCad 4 が parse できない KiCad 5 schematic records を削除または rewrite する。
- symbol library headers を 2.4-style output から 2.3-style output に downgrade する。
- KiCad 4 libraries に存在しない symbol fields または attributes を削除する。
- board と footprint files を `20171130` から version `4` に downgrade する。
- KiCad 4 以降に導入された KiCad 5 PCB features を削除する。differential pair settings per net class、
  long pad names、custom pad shape details、multi-layer keepouts、KiCad 4 が解釈できない
  3D model offset changes、locked/unlocked footprint text を含む。
- reversible unit conversion が分かっている場合は 3D model offsets を KiCad 4 representation に
  変換する。それ以外は warning を出し、unsupported offset fields を削除する。

想定される lossy area:

- Custom pads と multi-layer keepouts は単純化または削除が必要になる場合がある。
- Long pad names は truncation または deterministic renaming が必要になる場合がある。
- 一部の net-class と footprint text-lock metadata は保持できない。

## 後続の隣接 Downgrade Pattern

同じ downgrade framework で後続の隣接 version も扱うべきです。

| Route | Main implementation focus |
| --- | --- |
| KiCad 8 to 7 | V8 generator cleanup fields, PCB fields, generated objects, UUID normalization, custom pad spoke templates, modern teardrops, images, rule-area changes, simulation/exclude flags, worksheet embedded images を削除する。 |
| KiCad 9 to 8 | Embedded files, tables, rule areas, component classes, complex padstacks, via stacks, arbitrary user layers, tenting, multiple netclass assignments, netclass color highlighting を削除する。 |
| KiCad 10 to 9 | Variants, jumper pads, barcodes, via protection, zone hatch offsets, backdrill, split via types, stopped-netcode assumptions, rounded rectangles, stacked pins, PCB points, property-formatting updates を削除する。 |
| Current development to KiCad 10 | Post-10.0 development features を削除する: extruded 3D body metadata, native ellipses and ellipse arcs, dielectric frequency-dependent stackup fields, net chains, copper thieving, pad `sim_electrical_type`, PCB table-cell `knockout`。 |

同じ route の forward migration は通常 downgrade より lossy ではありませんが、compatibility rewrite と
target defaults は引き続き必要です。例えば KiCad 7 から 8 では、target format が期待する場所にだけ
V8-normalized `generator_version` handling を導入すべきです。KiCad 8 から 9 では、source semantics に
存在しない embedded files、component classes、padstacks を作り出してはいけません。

## Test Strategy

隣接 route と document kind ごとに fixture set を作成します。

- Project files: `.pro` と `.kicad_pro`。
- Schematic files: legacy `.sch` と `.kicad_sch`。
- Symbol libraries: `.lib`、`.dcm`、`.kicad_sym`。
- PCB files: `.kicad_pcb`。
- Footprints: `.kicad_mod`。
- Worksheets: legacy drawing sheet と `.kicad_wks`。
- Design rules: KiCad 6+ のみ `.kicad_dru`。

各 test では次を検証します。

- source kind と source version が正しく検出される。
- target file family、extension、version が正しい。
- target version で禁止された token が存在しない。
- 既知の compatible structures が保持される。
- lossy changes が warning を生成する。
- 同じ migration を再実行しても stable で、file が変わり続けない。

V5/V6 と V6/V5 route では、可能な限り KiCad 自身が生成した golden-file tests を追加します。
これらの route は file-family boundary を跨ぎ、semantic drift のリスクが最も高いです。

## Implementation Order

推奨順序:

1. V7 から V6 の same-family S-expression downgrades を完了する。
2. V5 PCB / footprint downgrade to V4 を追加する。これは PCB S-expression family に留まるため。
3. V5 から V6 upgrade 用の legacy schematic と symbol parsers を実装する。
4. V6 から V5 downgrade 用の legacy schematic と symbol writers を実装する。
5. V5 から V4 の legacy schematic と symbol downgrade を追加する。
6. V6 から V7 のような forward same-family upgrades を追加する。
7. 同じ framework を V8、V9、V10、current-development routes に拡張する。

この順序なら有用な downgrade coverage を早めに提供しつつ、より難しい legacy schematic と
symbol conversion work を切り離せます。

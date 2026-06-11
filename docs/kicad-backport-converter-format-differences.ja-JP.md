# KiCad Backport コンバーターのフォーマット差分と変換動作

この文書は、現在の `kicad-backport` 実装が実際に処理する KiCad ファイルフォーマット差分を説明します。対象はファイル種別、バージョンアンカー、変換分岐、書き換え規則、損失を伴う変換です。KiCad フォーマット自体の正本は KiCad 開発者向けドキュメントです。

- https://dev-docs.kicad.org/en/file-formats/index.html

## KiCad 開発者向け要点

| 項目 | 実装上の扱い |
| --- | --- |
| フォーマット判定 | 近代フォーマットは主に S-expression の root token で判定します: `kicad_sch`, `kicad_symbol_lib`, `kicad_pcb`, `footprint`, `kicad_wks` / `drawing_sheet`。拡張子は fallback です。 |
| バージョン判定 | 近代 S-expression はトップレベルの `(version ...)` を読みます。`.kicad_pro` は `kicad-project-json`、legacy `.sch/.lib/.dcm/.pro` は legacy alias として報告されます。 |
| KiCad 5/6 境界 | schematic、symbol library、project は KiCad 6 でファイルファミリーが切り替わります。KiCad 4/5 の `.sch/.lib/.pro` と KiCad 6+ の `.kicad_sch/.kicad_sym/.kicad_pro` は同じ構文ではありません。 |
| PCB / footprint | KiCad 4-10 の board と footprint は S-expression として処理されます。差分は version anchor、ノード集合、フィールド構文です。 |
| `.kicad_pro` | 近代 project JSON は target KiCad major ごとには書き換えません。KiCad 6+ では通常そのままコピーし、KiCad 5/4 では最小限の `.pro` を生成します。 |
| `.kicad_wks` | worksheet は検出と version 書き換えが可能ですが、現在の専用 downgrade 規則は限定的で、KiCad 4/5 legacy writer はありません。 |
| `.kicad_dru` | コード上は検出でき、固定 anchor `20200610` を持ちますが、この文書の主要なユーザー可視フォーマット表には含めません。 |

## 実装モデル

| 段階 | 実装 | 意味 |
| --- | --- | --- |
| 読み込み | `loadDocumentImpl()` がテキストを読み、`.kicad_pro` と legacy ファイルを拡張子で分岐し、それ以外を S-expression として parse します。 | JSON と legacy text を S-expression と誤認しません。 |
| 種別判定 | `DetectKind()` は root token を優先し、拡張子を fallback とします。 | 正しい root を持つファイルは名前が通常と違っても扱えます。 |
| target 解決 | `ResolveTargetVersion()` は KiCad alias をファイル種別ごとの format version に変換します。 | KiCad の 1 つの release が全ファイルで同じ version を使うわけではありません。 |
| 出力拡張子 | `withTargetFamilyExtension()` は KiCad 5/6 境界で `.sch/.lib/.pro` と `.kicad_sch/.kicad_sym/.kicad_pro` を切り替えます。 | 5/6 変換は単なる `(version ...)` 編集ではありません。 |
| 同一 version | S-expression の source と target が同じ場合はコピーまたは無変更です。 | 余分な整形や規則適用を避けます。 |
| upgrade | source version が target より古い場合、`ApplyUpgradeRules()` が保守的な構文正規化を行います。 | 新しい設計意図は自動生成しません。 |
| downgrade | source version が target より新しい場合、`ApplyDowngradeRules()` が削除、改名、平坦化、近似を行います。 | 古い KiCad parser が未知ノードを読まないようにします。 |
| 書き出し | `ensureVersion()` が version を設定し、S-expression は `SEXPR::Format()` で出力します。 | JSON と legacy text は専用 writer または raw path で扱います。 |

## target version anchor

| Target | Symbol library | Schematic | Board | Footprint | Worksheet | 備考 |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `4.0` | legacy `.lib` 2.3 | legacy `.sch` V2 | `4` | `4` | 未定義 | schematic/symbol は legacy writer。 |
| `5.0` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | 未定義 | `5.1` と同じ PCB/footprint anchor。 |
| `5.1` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | 未定義 | `5.0` と同じ。 |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | 近代 schematic/symbol の開始点。 |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | 近代 S-expression 拡張。 |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `generator_version`、UUID/id、PCB fields の境界。 |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | embedded data、tables、rule areas、複雑な PCB object の境界。 |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | コードが持つ通常 target alias の最大値。 |
| `10.99` | `20251024` | `20260306` | `20260603` | `20260603` | `20231118` | 開発用 alias。board/footprint だけが `10.0` より進みます。 |

## ファイルファミリー別変換

| ファイル | 変換動作 | 注意点 |
| --- | --- | --- |
| `.kicad_pro` | KiCad 6+ target では raw JSON コピー、KiCad 5/4 では最小 `.pro` を生成。 | target major ごとの JSON 再構築はしません。 |
| legacy `.pro` | KiCad 6+ では最小 `.kicad_pro` JSON を生成。 | 認識できる legacy setting と library 名だけを保持します。 |
| `.kicad_sch` | KiCad 5/4 では legacy `.sch` writer。KiCad 6+ では S-expression rules。 | 現代の property、instance、複雑な object は legacy で損失します。 |
| legacy `.sch` | KiCad 6+ では `.kicad_sch` に変換。KiCad 5/4 では header を書き換えます。 | legacy の非 wire drawing は完全には mapping されません。 |
| `.kicad_sym` | KiCad 5/4 では `.lib` と `.dcm` を出力。 | 現代 symbol property、graphics、nested symbol は近似されます。 |
| legacy `.lib/.dcm` | KiCad 6+ では `.kicad_sym` を生成。 | `.dcm` 単体は symbol skeleton になり、文書 metadata は完全ではありません。 |
| `.kicad_pcb/.kicad_mod` | すべての定義済み target で S-expression のまま version と node/field を rewrite。 | 対象にない geometry/electrical/manufacturing/cache field は削除または近似されます。 |
| `.kicad_wks` | KiCad 6+ では version rewrite と限定的な worksheet rule。 | KiCad 4/5 legacy worksheet writer はありません。 |

## 主な downgrade 方針

| 条件 | 実装の選択 | 例 |
| --- | --- | --- |
| target parser が読めない node | node または field を削除し warning を出します。 | `embedded_files`, `variants`, `barcodes`, `net_chains`, native ellipse。 |
| 古い表現に近似できる | 改名、平坦化、legacy field への変換を行います。 | `directive_label -> netclass_flag`, `stroke -> width`, `uuid/tstamp/id`。 |
| 古い geometry しかない | 単純な図形へ変換します。 | PCB rectangle を line、track arc を segment、custom pad を rectangular pad。 |
| 古い property layout | property を移動し、ID を追加または削除します。 | property hide の位置、standard property id。 |
| source に新しい意味がない | 新機能 object は生成しません。 | padstack、variants、component classes、barcodes は自動生成されません。 |

## project directory 変換

| 処理 | 実装 |
| --- | --- |
| 入力 | directory または `.kicad_pro` は project tree として扱います。 |
| コピー対象 | KiCad document、legacy document、library table、`.kicad_prl`、3D model file。 |
| skip 対象 | VCS、history、backup、archive、gerber/fabrication/output/plot/BOM/assembly/vendor output。 |
| 拡張子変換 | KiCad 5/4 target では `.kicad_sch/.kicad_sym/.kicad_pro` を `.sch/.lib/.pro` へ、KiCad 6+ では逆方向へ変換します。 |
| `.dcm` | KiCad 6+ target で対応する `.lib` がある場合、`.dcm` は単独変換しません。 |
| library table | `sym-lib-table` / `fp-lib-table` を target family に合わせて正規化します。 |
| schematic support | KiCad 6+ target では project-local symbol を schematic `lib_symbols` に embed し、hierarchy instances を再構築します。 |


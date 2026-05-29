# KiCad ファイル形式バージョン差分

これは [英語の基準ドキュメント](kicad-file-format-version-differences.md) の日本語版です。KiCad Backport C++ が参照するファイル形式バージョン、主な差分、実装済みのダウングレード規則をまとめます。KiCad の token、ソースマクロ、拡張子、バージョン番号は検索しやすいように原文のまま残します。

最終更新: 2026-05-29。

## ソース

- KiCad 公式 tag とソースの version header。
- ローカル KiCad `master`: `10.99.0-1153-g8c1d374f29`。
- `kicad-backport-cplus` 実装: `kicad_backport_versions.cpp`、`kicad_backport_rules.cpp`、`kicad_backport_rule_rewriters.cpp`、`kicad_backport.cpp`。

対象マクロは `SEXPR_SYMBOL_LIB_FILE_VERSION`、`SEXPR_SCHEMATIC_FILE_VERSION`、`SEXPR_BOARD_FILE_VERSION`、`SEXPR_WORKSHEET_FILE_VERSION`、`DRC_RULE_FILE_VERSION` です。

## 安定版の形式バージョン

| KiCad tag | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

Board の S-expression 版は `.kicad_pcb` と `.kicad_mod` の両方に使われます。`.kicad_dru` は追跡対象の範囲では `20200610` のままですが、ルールの意味が変わっていないことを意味するものではありません。`.kicad_pro` は JSON schema migration 系で、これらの日付形式 macro とは別管理です。

## 10.99 / current

| ファイル種別 | 10.99/current version | メモ |
| --- | ---: | --- |
| Board `.kicad_pcb` | `20260513` | Copper thieving zone fill mode |
| Footprint `.kicad_mod` | `20260513` | PCB S-expression version を使用 |
| Schematic `.kicad_sch` | `20260512` | Net chains |
| Symbol library `.kicad_sym` | `20260508` | Native ellipse primitive |
| Worksheet `.kicad_wks` | `20231118` | KiCad 8 cleanup 以降 bump なし |
| Design rules `.kicad_dru` | `20200610` | 10.99 固有の bump は未確認 |

KiCad 10 以降の主な追加は、extruded 3D body metadata、native ellipse / ellipse-arc、dielectric frequency fields、net chains、copper thieving です。

## C++ 実装の対象バージョン

| Alias | Symbol | Schematic | Board | Footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |

変換器は古いファイルをアップグレードしません。source version が target より小さい場合はファイルをそのままコピーし、report には元の version を保持します。

## 主なダウングレード規則

Symbol library / schematic:

- 古い parser が読めない `text_box`、embedded files、private flags、rounded rectangles、native ellipses、schematic variants、net chains などを削除します。
- legacy property IDs を補い、property `hide` を `effects` 内へ戻します。
- boolean `hide` や font `bold` / `italic` を list 形式から presence atom へ戻します。
- `generator_version`、`embedded_fonts`、font `face`、`show_name`、`do_not_autoplace`、power class flags、jumper pin flags を必要に応じて削除します。

Board / footprint:

- text boxes、images、net ties、generated objects、teardrops、tables、tenting、embedded files、component classes、padstacks、via stacks、rule areas、via protection、custom layer counts、rounded rectangles、points、barcodes、backdrill、variants、extruded models、native ellipses、dielectric frequency fields、net chains、copper thieving を削除または降格します。
- copper thieving fill mode は polygon fill に戻します。
- footprint の `Reference` / `Value` property は legacy `fp_text` に戻します。
- scoped `uuid` は `tstamp` に、group/generated `uuid` は `id` に戻します。
- KiCad 7 非対応の PCB dimensions は表示用 text annotation に変換します。
- 古い board 用に numeric netcodes と root-level net declarations を再構築します。

Worksheet は target が `20220228` より古い場合に `font` blocks を削除します。Design rules は検出と target version 解決のみで、専用 rewrite はまだありません。

## Report と保守

削除または互換 rewrite が発生すると warning が記録されます。report には path、kind、source version、target version、changed、warnings が入ります。新しい KiCad 版を追加するときは、まず version matrix を更新し、stable tag と development branch の差分を分けて記録してください。

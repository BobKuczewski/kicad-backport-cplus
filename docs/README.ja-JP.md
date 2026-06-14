# KiCad Backport C++

`kicad-backport` は、portable C++ で実装された独立した CLI converter で、KiCad のプロジェクトやファイルを古い KiCad ファイルフォーマットターゲットへ変換します。目的は parser compatibility です。古い KiCad に等価表現がある場合はその表現へ書き換え、等価表現がない場合は未対応構文を削除または近似し、warning として報告します。

実装は dependency-free で、小さな KiCad 風 S-expression parser/formatter を内蔵しており、直接実行または plugin wrapper から利用できます。

## ドキュメント

- [Documentation index](README.md)
- [KiCad backport converter format differences](kicad-backport-converter-format-differences.ja-JP.md)
- [English README](../README.md)

## コマンド

```text
kicad-backport convert [--quiet] --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

例:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions --json E:\tmp\project
```

`convert` は単一 KiCad document または project directory を受け取ります。`.kicad_pro` を渡すと、その親 project directory を変換します。出力 path には `project_V9` のように target suffix が追加されます。

`inspect` は検出された file kind と version を報告します。`detect-versions` は file/directory を軽量 scan し、対応 KiCad document kind を報告し、JSON も出力できます。

## 対応ターゲット

| Target | 現在の動作 |
| --- | --- |
| KiCad 10.99 | 開発 alias。board/footprint は `20260603`、schematic/symbol-library は KiCad 10 anchor のままです。 |
| KiCad 10 | `10.0` target anchor に含まれない開発構文を削除または書き換えます。 |
| KiCad 9 | variants、barcodes、backdrill/post-machining、jumper pad metadata、net-name-only board references など KiCad 10 時代の機能を削除または downgrade します。 |
| KiCad 8 | KiCad 9+ tables、embedded files/fonts、component classes、padstacks、via stacks、rule/placement areas、user-layer type qualifiers、font face fields を削除または書き換えます。 |
| KiCad 7 | UUID/tstamp、PCB footprint fields、teardrops、generated objects、images、text boxes、stroke/dimension syntax の互換処理を行います。 |
| KiCad 6 | 最初の modern schematic/symbol/project family を target とし、必要な KiCad 6 parser compatibility structures を追加します。 |
| KiCad 5.0/5.1 | board/footprint は `20171130`。schematic、symbol-library、project は legacy `.sch`、`.lib/.dcm`、`.pro` を出力します。 |
| KiCad 4 | board/footprint は `4`。V4 legacy schematic/library header を書き換え、KiCad 5+ PCB construct を可能な範囲で簡略化します。 |

詳細は [format differences](kicad-backport-converter-format-differences.ja-JP.md) を参照してください。

## 変換方針

- target format が表現できる既存の設計意図を保持します。
- 等価な古い構文がある場合は、その構文へ書き換えます。
- 古い KiCad parser が読めない、または旧 format に等価表現がない場合のみ削除します。
- 損失を伴う rewrite と削除は warning として出力します。
- upgrade は保守的で、source に存在しない KiCad feature を作りません。

KiCad 5/6 境界では file family が切り替わります: `.sch -> .kicad_sch`、`.lib/.dcm -> .kicad_sym`、`.pro -> .kicad_pro`、および逆方向の `.kicad_sch -> .sch`、`.kicad_sym -> .lib + .dcm`、`.kicad_pro -> .pro`。

## Project conversion

project directory 変換では、編集可能な KiCad input と一般的な local 3D model file のみをコピーし、コピー内の KiCad document を変換します。manufacturing output、backup、history、Gerbers、BOM、plot/export directory、temporary file は skip します。

project-level repair には `sym-lib-table` / `fp-lib-table` の正規化、KiCad 6/7/8 向け `.kicad_prl` visibility data、新しい board/footprint から downgrade する際の embedded 3D model resource の `3D/` への抽出と `kicad-embed://...` model URI の `${KIPRJMOD}/3D/...` への書き換え、KiCad 6+ 向け project-local schematic symbols の embed、schematic hierarchy instances の再構築が含まれます。

## Build

```powershell
.\build.ps1
```

```sh
./build.sh
```

Useful POSIX options:

```sh
./build.sh --clean
./build.sh --config Release
./build.sh --compiler g++-8
./build.sh --static-runtime off
./build.sh --zstd off
```

PowerShell も `-Zstd auto|on|off` を受け付けます。default の `auto` は `src/third_party/zstd` が存在する場合に vendored zstd decompressor を compile します。古い toolchain が vendored C source を build できない場合は `off` を使えますが、その場合 embedded 3D model resource は抽出できず、unsupported embedded model reference を削除する時に warning が出ます。

scripts は `kicad_backport_sources.txt` を読み、`g++` または `clang++` で compile し、実行ファイルを `dist/` にコピーします。必要に応じて `-std=c++17` から `-std=c++1z` へ fallback します。

## Validation

変換後は target KiCad version で開き、必要な ERC/DRC を実行してください。converter warning は production use 前に確認してください。

## 謝辞

本プロジェクトの開発にあたり支援をいただいた Hubert に深く感謝します。

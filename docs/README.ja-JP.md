# KiCad バックポート C++

KiCad バックポート ダウングレード CLI のスタンドアロン C++17 実装。ツール
新しい KiCad S-expression プロジェクト ファイルを古い KiCad ファイル形式に変換します
ただし、削除よりも同等の従来の構文が優先されます。

## ローカライズされたドキュメント

- [简体中文](docs/README.zh-CN.md)
- [日本語](docs/README.ja-JP.md)
- [한국어](docs/README.ko-KR.md)
- [フランス語](docs/README.fr-FR.md)
- [ドイツ語](docs/README.de-DE.md)
- [スペイン語](docs/README.es-ES.md)
- [イタリア語](docs/README.it-IT.md)

追加の参考資料:

- [KiCad ファイル形式のバージョンの違い](docs/kicad-file-format-version-differences.md)
- [ローカライズされたファイル形式のバージョンの違い](docs/README.md#kicad-file-format-version-differences)

## コマンド

コマンド ライン インターフェイスは Go の実装を反映しており、
直接と Python プラグインからの両方で使用できます。

```text
kicad-backport convert --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

コンバーターは KiCad S-expression ファイルを読み取り、バージョン主導のダウングレードを適用します
ルールを作成し、バージョン接尾辞付きの出力パスを書き込み、KiCad プロジェクト全体をコピーできます
コピー内のすべての KiCad ファイルを正規化する前に、ディレクトリを削除します。

例:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

サポートされているリリース エイリアスは、`4.0`、`5.0`、`5.1`、`6.0`、`7.0`、
`8.0`、`9.0`、`10.0`、および `10.99` です。特定のパーサー境界をテストする場合は、
KiCad ファイル形式の生のバージョン番号も渡せます。

## サポート状況

現在の実装は、KiCad 4 から KiCad 10 までのファイル ファミリを対象としています。

| ターゲット | 状態 |
| --- | --- |
| KiCad 10 | 20260521 パッド `sim_electrical_type` および 20260603 テーブルセル `knockout` を含む、10.0 後/現在の開発構文を削除します。 |
| KiCad 10.99 | 現在の開発版 board/footprint ターゲットです。board と footprint は `20260603` を書き、symbol library と schematic は KiCad 10 ターゲット版（`20251024` / `20260306`）を引き続き使用します。 |
| KiCad9 | バリアント、バーコード、バックドリル/ポストマシニング、ジャンパー パッド、ネットコード省略などの KiCad 10/現在の​​機能を削除またはダウングレードします。 |
| KiCad8 | KiCad 9+ テーブル、埋め込みファイル、コンポーネント クラス、パッドスタック、ビア スタック、ルール領域、および任意のユーザー層フォームを削除します。 |
| KiCad7 | UUID/tstamp フォーム、PCB フットプリント フィールド、ティアドロップ、生成されたオブジェクト、イメージ、およびテキスト ボックスに古いパーサー互換性の書き換えを適用します。 |
| KiCad 6 | 基本的なファイルのダウングレードのサポートはほぼ完了しました。変換されたテスト プロジェクトは、検証のために実際の KiCad 6 アプリケーションで手動で開かれました。 |
| KiCad 5 | board/footprint のターゲット version `20171130` と、legacy `.sch`、`.lib`、`.dcm`、`.pro` の基本的な import/export をサポートします。詳細な schematic オブジェクト、symbol 描画プリミティブ、pin はまだロッシーで、warning として報告されます。 |
| KiCad 4 | board/footprint のターゲット version `4`、V4 legacy schematic/library ヘッダーの書き換え、V4 出力 suffix/extension をサポートします。custom pad などの V5-only PCB 機能は可能な範囲で簡略化されます。 |

## ダウングレードポリシー

コンバーターは、使用可能な最も互換性のある表現を適用します。
ターゲットフォーマット:

- 新しいオブジェクトまたはフィールドは、可能な場合は古い同等の構文にマップされます。
- 可視/製造情報は、古い形式で表現できる場所に保持されます。
- サポートされていない構文は、古い KiCad パーサーがそれをロードできない場合、または
古いファイル形式には同等の表現がありません。
- 削除または互換性の書き換えはそれぞれ、警告として報告されます。

たとえば、レガシー ネット コードは古い PCB フォーマット、新しいブール値用に再構築されます。
フィールド フォームは必要に応じてプレゼンス アトムに変換されます。KiCad 7 PCB
寸法は表示可能なグラフィックスとして保存され、レガシー プロジェクト ローカル ボードとして保存されます。
可視性ファイルは KiCad 6/7/8 ターゲット用に生成されます。

プロジェクト ディレクトリまたは `.kicad_pro` を変換する場合、ツールはコピーのみを行います。
編集可能な KiCad 入力と共通のローカル 3D モデル ファイル。生成されたものづくり
出力、履歴/バックアップ フォルダー、ガーバー、BOM、および一時ファイルはスキップされます。

## プロジェクトのレイアウト

コードは責任ごとに分割されているため、新しい KiCad バージョンを追加できます。
小規模で局所的な変更:

- `src/kicad_backport.cpp`: CLI フロー、プロジェクトのコピー/フィルタリング、ファイル変換。
- `src/kicad_backport_document.cpp`: KiCad ドキュメントの種類の検出。
- `src/kicad_backport_legacy.cpp`: 従来の KiCad ドキュメント読み込みスキャフォールディング。
- `src/kicad_backport_pathmap.cpp`: ターゲット ファイル拡張子マッピング ヘルパー。
- `src/kicad_backport_report.cpp`: JSON レポートの形式。
- `src/kicad_backport_rules.cpp`: バージョン ゲートとダウングレード ルールの順序。
- `src/kicad_backport_rule_rewriters.cpp`: S 式ツリー書き換えヘルパー。
- `src/kicad_backport_upgrade.cpp`: 古いソース ファイルの構文正規化が制限されています。
- `src/kicad_backport_versions.cpp`: KiCad リリースのエイリアスと形式のバージョン。
- `src/kicad_backport_util.cpp`: 共有文字列、ファイル、および JSON ヘルパー。
- `src/sexpr.cpp`: 最小限の KiCad スタイルの S 式パーサー/フォーマッタ。
- `src/internal/`: ソース ファイルによってのみ使用されるプライベート実装ヘッダー。
- `include/kicad_backport/`: 実行可能ファイルによって使用されるパブリック プロジェクト ヘッダー。

シングルアクションのダウングレード ルールは、代わりに小さな `applyWhen()` ヘルパーを使用します。
`std::function`、ヒープ割り当てを追加せずにルールをコンパクトに保ちます。
マルチアクション ルールは、順序付け時にグループ化されたままになります。

トップレベルの構造は意図的に単純になっています。

```text
kicad-backport-cplus/
  include/kicad_backport/   public headers
  src/                      implementation files
  src/internal/             private headers
  scripts/                  cross-build environment setup
  build/                    generated build trees
  dist/                     packaged command-line binaries
```

## 建てる

現在のプラットフォーム上に構築します。

```powershell
.\build.ps1
```

```sh
./build.sh
```

事前に最小の実用的なツールチェーンを自動的に検出してインストールするには
建物：

```powershell
.\build.ps1 -SetupMissingTools
```

```sh
./build.sh --setup
```

どちらのスクリプトも標準のリリース ターゲットを試行し、成功した出力を次の場所にコピーします。
`dist/` プラグイン互換の名前を使用:

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

`.\build.ps1 -Clean` または `./build.sh --clean` を使用して以前のビルドを削除します
リビルド前の出力。

C++ クロスコンパイルにはプラットフォーム ツールチェーンが必要です。 Windows の場合、`build.ps1`
Visual Studio で `windows-amd64` と `windows-arm64` をビルドし、ビルドします
WSL ツールチェーンが利用可能な場合は、WSL 経由の `linux-amd64`/`linux-arm64`。
Linux では、`build.sh` はネイティブ Linux をビルドし、次の場合に `linux-arm64` をビルドできます。
`aarch64-linux-gnu-g++` がインストールされています。 macOS では、`build.sh` が Darwin をビルドします
Apple SDK を使用した amd64/arm64。 Darwin バイナリは macOS 上で生成する必要があります。

サブセットを構築するには:

```powershell
.\build.ps1 -Targets windows-amd64,windows-arm64
```

```sh
TARGETS="linux-amd64 linux-arm64" ./build.sh
```

クロスビルド環境のセットアップ:

```powershell
.\scripts\setup-cross.ps1
.\scripts\setup-cross.ps1 -CheckOnly
```

```sh
./scripts/setup-cross.sh
./scripts/setup-cross.sh --check-only
```

セットアップ スクリプトは、実用的な最小のビルド ツールチェーンを自動的にインストールします。
ホストプラットフォーム用。欠落を報告する場合のみ、`-CheckOnly` または `--check-only` を使用してください
何もインストールせずにツールを使用できます。

Windows では、セットアップ スクリプトにより CMake、Visual Studio C++ がインストールまたは準備されます。
ビルド ツール、WSL、Ubuntu、および Linux に必要な最小限の WSL パッケージ
amd64/arm64 ビルド。 Linux では、CMake、ネイティブ C++ コンパイラー、Ninja、
ホスト パッケージでサポートされる aarch64 Linux クロス コンパイラー
マネージャー。 macOS では、Apple コマンド ライン ツールをトリガーし、CMake/Ninja をインストールします。
利用可能な場合は Homebrew を通じて。

手動 CMake ビルド:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

実装は意図的に依存関係がなく、KiCad スタイルの C++ に従っています。
書式設定規則。

## 検証

変換後、一致する KiCad バージョンで各ターゲットを検証します。のために
KiCad 8/9/10 これは通常、回路図 ERC および PCB DRC を実行することを意味します。

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 CLI のコマンド セットは小さいため、ネットリストとガーバー エクスポートを使用して
変換された回路図ファイルと PCB ファイルが読み込まれることを確認します。

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

KiCad 6 の CLI 検証範囲は限られています。 PCB ファイルの場合、簡単なパーサー チェック
KiCad 6 の Python モジュールを通じて実行できます。

```powershell
& 'D:\KiCad\6.0\bin\python.exe' -c "import pcbnew; pcbnew.LoadBoard(r'E:\tmp\project_V6\project.kicad_pcb'); print('pcb ok')"
```

KiCad 6 の回路図とシンボルの場合、手動で GUI を開くことが依然として最も便利です
エンドツーエンドの検証。現在の V6 回帰サンプルはこの方法でチェックされています。

ERC/DRC 違反は、プロジェクトからの設計ルールの発見です。そうではありません
KiCad がロードまたは解析エラーを報告しない限り、フォーマット変換は失敗します。

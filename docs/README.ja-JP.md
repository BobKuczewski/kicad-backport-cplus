# KiCad Backport C++

KiCad Backport C++ は、KiCad Backport ダウングレード CLI の独立した C++17 実装です。新しい KiCad の S 式プロジェクトファイルを古い KiCad ファイル形式へ変換し、削除よりも古い形式で表現できる等価な構文への変換を優先します。

## コマンド

```text
kicad-backport convert --target-version <7.0|8.0|9.0|10.0|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

例：

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

サポートされるリリース別名は `7.0`、`8.0`、`9.0`、`10.0` です。特定のパーサ境界をテストする場合は、KiCad の生のファイル形式バージョン番号も指定できます。

## ダウングレード方針

- 新しいオブジェクトやフィールドは、可能な限り古い等価構文へ変換します。
- 古い形式で表現できる表示情報と製造情報は保持します。
- 古い KiCad が読み込めない構文、または古い形式に等価表現がない構文だけを削除します。
- 削除や互換変換は warning として報告されます。

プロジェクトディレクトリ変換では、KiCad プロジェクト関連ファイルと一般的なローカル 3D モデルのみをコピーします。`.history`、バックアップ、Gerber、BOM、PDF、README などの不要なファイルはコピーしません。

## ビルド

Windows：

```powershell
.\build.ps1
.\build.ps1 -SetupMissingTools
```

Linux/macOS：

```sh
./build.sh
./build.sh --setup
```

出力は `dist/` に作成されます。

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

Windows では Visual Studio で Windows amd64/arm64 をビルドし、WSL ツールチェーンがある場合は Linux amd64/arm64 もビルドします。Darwin バイナリは macOS と Apple SDK が必要です。

## 検証

変換後は対象バージョンの KiCad CLI で検証してください。KiCad 8/9/10 では通常 ERC と DRC を実行します。

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 ではネットリスト出力と Gerber 出力で、ファイルが読み込めることを確認します。

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

ERC/DRC の違反は設計ルールの結果です。KiCad が読み込みエラーや解析エラーを出さない限り、形式変換の失敗ではありません。

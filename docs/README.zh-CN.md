# KiCad Backport C++

KiCad Backport C++ 是 KiCad Backport 降级命令行工具的独立 C++17 实现。它把较新版 KiCad 的 S 表达式工程文件转换为旧版 KiCad 文件格式，并优先使用旧格式中可等效表达的语法，而不是简单删除对象。

## 命令

```text
kicad-backport convert --target-version <7.0|8.0|9.0|10.0|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

示例：

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

支持的版本别名为 `7.0`、`8.0`、`9.0`、`10.0`。也可以传入原始 KiCad 文件格式版本号，用于测试特定解析器分界点。

## 降级策略

- 能映射为旧版等效语法的新对象或字段，会优先映射。
- 旧格式能够表达的可见信息和生产制造信息会尽量保留。
- 只有旧版 KiCad 无法解析，或旧格式完全没有等效表达时，才删除不兼容语法。
- 每个删除或兼容重写都会在转换报告中记录为 warning。

工程目录转换时只复制 KiCad 工程相关文件和常见本地 3D 模型文件，不复制 `.history`、备份、Gerber、BOM、PDF、README 等无关文件。

## 构建

Windows：

```powershell
.\build.ps1
.\build.ps1 -SetupMissingTools
.\build.ps1 -Targets windows-amd64,windows-arm64
```

Linux/macOS：

```sh
./build.sh
./build.sh --setup
TARGETS="linux-amd64 linux-arm64" ./build.sh
```

输出文件位于 `dist/`：

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

在 Windows 上，`build.ps1` 使用 Visual Studio 构建 Windows amd64/arm64，并在 WSL 工具链可用时构建 Linux amd64/arm64。Darwin 目标需要 macOS 和 Apple SDK，必须在 macOS 上生成。

## 验证

转换后应使用对应版本的 KiCad CLI 验证。KiCad 8/9/10 通常运行 ERC 和 DRC：

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 CLI 命令较少，可使用网表和 Gerber 导出来验证文件能否加载：

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

ERC/DRC 违规是工程本身的设计规则结果。除非 KiCad 报告加载或解析错误，否则它们不代表格式转换失败。

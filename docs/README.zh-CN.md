# KiCad backport C++

KiCad Backport 降级 CLI 的独立 C++17 实现。工具
将较新的 KiCad S-express 项目文件转换为较旧的 KiCad 文件格式
同时更喜欢等效的遗留语法而不是删除。

## 本地化文档

- [简体中文](docs/README.zh-CN.md)
- [日本语](docs/README.ja-JP.md)
- [한국어](docs/README.ko-KR.md)
- [法语](docs/README.fr-FR.md)
- [德语](docs/README.de-DE.md)
- [西班牙语](docs/README.es-ES.md)
- [意大利语](docs/README.it-IT.md)

附加参考：

- [KiCad 文件格式版本差异](docs/kicad-file-format-version-differences.md)
- [本地化文件格式版本差异](docs/README.md#kicad-file-format-version-differences)

## 命令

命令行界面反映了 Go 的实现，旨在
可以直接使用，也可以通过 Python 插件使用：

```text
kicad-backport convert --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

converter读取 KiCad S-表达式文件，应用版本驱动的降级
规则，写入版本后缀的输出路径，并且可以复制整个 KiCad 项目
规范化副本中的所有 KiCad 文件之前的目录。执行转换时会输出每个 KiCad
文件检测到的源文件版本号和解析后的目标文件版本号；面向人的文本输出会优先显示
`9.0 (20241229)` 或 `10.99-dev (20260513)` 这类 KiCad 版本别名，
而不是只显示原始文件格式版本号。`detect-versions` 是极速目录扫描命令，
只读取足够的文件文本来报告 KiCad 相关文件类型和版本；普通文本输出同样使用版本别名，
JSON report 仍保留原始文件格式版本号。它会先按支持的 KiCad 文件后缀过滤，
无法识别版本的文件不会出现在结果中。

示例：

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions E:\tmp\project
```

支持的版本别名为 `4.0`、`5.0`、`5.1`、`6.0`、`7.0`、`8.0`、`9.0`、`10.0` 和 `10.99`。测试特定解析器截止点时，也可以传入原始 KiCad 文件格式版本号。

## 支持状态

当前实现覆盖 KiCad 4 到 KiCad 10 文件族：

| 目标 | 地位 |
| --- | --- |
| KiCad 10 | 删除 10.0 后/当前开发语法，包括 20260521 焊盘 `sim_electrical_type` 和 20260603 表格单元格 `knockout`。 |
| KiCad 10.99 | 当前开发版 board/footprint 目标：写出 board 和 footprint 版本 `20260603`；symbol library 和 schematic 仍使用 KiCad 10 目标版本（`20251024` / `20260306`）。 |
| KiCad9 | 删除或降级 KiCad 10/当前功能，例如变体、条形码、背钻/后加工、跳线垫和网络代码省略。 |
| KiCad 8 | 删除 KiCad 9+ 表、嵌入文件、组件类、padstack、过孔堆栈、规则区域和任意用户层表单。 |
| KiCad7 | 对 UUID/tstamp 表单、PCB 封装字段、泪滴、生成的对象、图像和文本框应用较旧的解析器兼容性重写。 |
| KiCad 6 | 基本的文件降级支持已基本完成。转换后的测试项目已在实际的 KiCad 6 应用程序中手动打开以进行验证。 |
| KiCad 5 | 支持 board/footprint 目标版本 `20171130`，并提供 legacy `.sch`、`.lib`、`.dcm`、`.pro` 的基础导入/导出和确定性输出路径。复杂原理图对象、符号绘图图元和引脚仍然是有损转换，并会输出 warning。 |
| KiCad 4 | 支持 board/footprint 目标版本 `4`、V4 legacy 原理图/符号库文件头重写，以及 V4 输出后缀和扩展名。V5-only PCB 特性会尽可能简化，例如 custom pad 会降级为矩形 pad。 |

## 降级政策

converter应用最兼容的表示形式
目标格式：

- 如果可能，新对象或字段将映射到较旧的等效语法。
- 可见/制造信息保留在旧格式可以表达的地方。
- 仅当较旧的 KiCad 解析器无法加载不支持的语法时才会删除它，或者
旧的文件格式没有等效的表示形式。
- 每次删除或兼容性重写都会报告为警告。

例如，旧的网络代码针对旧的 PCB 格式、新的布尔值进行了重建
场形式在需要时转换为存在原子，KiCad 7 PCB
尺寸保留为可见图形，并且遗留项目本地板
为 KiCad 6/7/8 目标生成可见性文件。

转换项目目录或 `.kicad_pro` 时，该工具仅复制
可编辑的 KiCad 输入和常见的本地 3D 模型文件。生成制造
输出、历史/备份文件夹、Gerber、BOM 和临时文件将被跳过。
跨越 KiCad 5/6 边界时会按目标自动切换扩展名，例如
`.sch -> .kicad_sch`、`.lib -> .kicad_sym`、`.kicad_sch -> .sch`、
`.kicad_sym -> .lib/.dcm`、`.kicad_pro -> .pro`。

## 项目布局

代码按职责划分，因此可以添加更高版本的 KiCad
小的、局部的变化：

- `src/kicad_backport.cpp`：CLI 流程、项目复制/过滤、文件转换。
- `src/kicad_backport_document.cpp`：KiCad 文档类型检测。
- `src/kicad_backport_legacy.cpp`：legacy KiCad `.sch`、`.lib`、`.dcm`、`.pro` 解析/写出辅助逻辑。
- `src/kicad_backport_pathmap.cpp`：目标文件扩展名映射助手。
- `src/kicad_backport_report.cpp`：JSON 报告格式。
- `src/kicad_backport_rules.cpp`：版本门和降级规则排序。
- `src/kicad_backport_rule_rewriters.cpp`：S 表达式树重写助手。
- `src/kicad_backport_upgrade.cpp`：旧源文件的有限语法规范化。
- `src/kicad_backport_versions.cpp`：KiCad 发布别名和格式版本。
- `src/kicad_backport_util.cpp`：共享字符串、文件和 JSON 帮助程序。
- `src/sexpr.cpp`：最小的 KiCad 风格的 S 表达式解析器/格式化程序。
- `src/internal/`：仅由源文件使用的私有实现标头。
- `include/kicad_backport/`：可执行文件使用的公共项目头。

单操作降级规则使用一个小的 `applyWhen()` 帮助器而不是
`std::function`，保持规则紧凑而不添加堆分配。
当排序很重要时，多操作规则保持分组。

顶层结构故意简单：

```text
kicad-backport-cplus/
  include/kicad_backport/   public headers
  src/                      implementation files
  src/internal/             private headers
  build/                    generated build trees
  dist/                     packaged command-line binaries
```

## 构建

现在只保留两个简单的直接构建入口：

- `build.ps1`：Windows 原生 MinGW/g++ 构建。
- `build.sh`：Linux、Raspberry Pi 和 macOS 原生构建。

从全新仓库拉取到原生编译：

```sh
git clone <repo-url> kicad-backport-cplus
cd kicad-backport-cplus
./build.sh --config Release
```

Windows 原生编译：

```powershell
git clone <repo-url> kicad-backport-cplus
cd kicad-backport-cplus
.\build.ps1
```


Linux/Raspberry Pi/macOS 原生构建：

```sh
./build.sh
```

POSIX 入口刻意保持直接：它读取 `kicad_backport_sources.txt`，用
`g++`/`clang++` 逐个编译源码，再链接生成可执行文件。它不调用项目生成器，也不安装
工具。要构建 Linux/RPi/macOS 目标，请在对应系统上运行 `build.sh`，或通过
`--compiler` 传入匹配的编译器。

常用原生选项：

```sh
./build.sh --clean
./build.sh --config Release
./build.sh --compiler g++-8
./build.sh --static-runtime off
```

构建输出会复制到 `dist/`，使用插件兼容名称。直接脚本当前生成运行它的主机目标：

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-linux-armhf`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

直接编译路径在 Linux 上默认使用 `--static-runtime auto`。它会先尝试静态链接
`libstdc++` / `libgcc`；如果系统没有静态运行库，再回退到系统动态运行库。

当前源码避免依赖较新的标准库文件系统、字符串视图、PMR 和 memory-resource
设施；路径/目录访问使用项目自有轻量 API，文本处理使用 `std::string`。直接编译
路径仍会从 `-std=c++17` 回退到老编译器常见的 `-std=c++1z`。直接编译也会探测当前工具链是否支持段级垃圾回收和符号剥离参数，只在支持时启用。
项目转换会按顺序处理复制后的文档，以便在低内存 Linux/RPi 系统上保持可预期的
峰值内存占用。

手动直接 GCC 构建：

```sh
./build.sh --config Release --target native
```

该实现有意无依赖并遵循 KiCad 风格的 C++
格式约定。

## 致谢

特别感谢 Hubert 在开发本项目的过程中提供的帮助。

## 验证

转换后，使用匹配的 KiCad 版本验证每个目标。为了
KiCad 8/9/10 这通常意味着运行原理图 ERC 和 PCB DRC：

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 CLI 有一个较小的命令集，因此使用网表和 Gerber 导出
验证转换后的原理图和 PCB 文件是否已加载：

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

KiCad 6 的 CLI 验证覆盖范围有限。对于 PCB 文件，快速解析器检查
可以通过 KiCad 6 的 Python 模块完成：

```powershell
& 'D:\KiCad\6.0\bin\python.exe' -c "import pcbnew; pcbnew.LoadBoard(r'E:\tmp\project_V6\project.kicad_pcb'); print('pcb ok')"
```

对于 KiCad 6 原理图和符号，手动 GUI 打开仍然是最有用的
端到端验证。当前的 V6 回归样本已通过这种方式进行检查。

ERC/DRC 违规是该项目的设计规则发现结果。他们不是
格式转换失败，除非 KiCad 报告加载或解析错误。

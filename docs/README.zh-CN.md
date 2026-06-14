# KiCad Backport C++

`kicad-backport` 是一个以可移植 C++ 实现的独立命令行转换器，用于把 KiCad 工程和文件转换到较旧的 KiCad 文件格式目标。它关注解析器兼容性：如果旧版 KiCad 有等价表达，转换器会重写为该表达；如果没有等价表达，则删除或近似处理不支持的语法，并以 warning 报告。

实现不依赖第三方库，内置一个小型 KiCad 风格 S-expression parser/formatter，可直接使用，也可由插件包装调用。

## 文档

- [文档索引](README.md)
- [KiCad backport 转换器格式差异](kicad-backport-converter-format-differences.zh-CN.md)
- [English README](../README.md)

## 命令

```text
kicad-backport convert [--quiet] --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

示例：

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions --json E:\tmp\project
```

`convert` 接受单个 KiCad 文档或工程目录。传入 `.kicad_pro` 时，会转换其所在工程目录。输出路径会自动追加目标后缀，例如 `project_V9`，除非该后缀已经存在。

`inspect` 报告检测到的文件类型和版本。`detect-versions` 对文件或目录执行轻量扫描，报告支持的 KiCad 文档类型，并可输出 JSON。

## 支持目标

| 目标 | 当前行为 |
| --- | --- |
| KiCad 10.99 | 开发别名。board 和 footprint 写 `20260603`；schematic 和 symbol-library 仍使用 KiCad 10 锚点。 |
| KiCad 10 | 删除或重写不属于 `10.0` 目标锚点的更新开发语法。 |
| KiCad 9 | 删除或降级 KiCad 10 时代特性，例如 variants、barcodes、backdrill/post-machining、jumper pad 元数据和仅 net-name 的 board 引用。 |
| KiCad 8 | 删除或重写 KiCad 9+ tables、embedded files/fonts、component classes、padstacks、via stacks、rule/placement areas、任意 user-layer type qualifiers 和 font face 字段。 |
| KiCad 7 | 处理 UUID/tstamp、PCB footprint fields、teardrops、generated objects、images、text boxes、stroke/dimension 语法的 KiCad 7 解析器兼容性。 |
| KiCad 6 | 面向第一代现代 schematic/symbol/project 文件族，并在需要时补 KiCad 6 parser 兼容结构。 |
| KiCad 5.0/5.1 | board/footprint 使用 `20171130`；schematic、symbol-library、project 写 legacy `.sch`、`.lib/.dcm`、`.pro`。 |
| KiCad 4 | board/footprint 使用 `4`，重写 V4 legacy schematic/library 文件头，并尽量简化 KiCad 5+ PCB 结构。 |

每类文件的版本锚点和转换细节见 [KiCad backport 转换器格式差异](kicad-backport-converter-format-differences.zh-CN.md)。

## 转换策略

- 目标格式能表达时，保留已有设计意图。
- 存在旧版等价语法时，将新版语法重写为旧版语法。
- 只有旧版 KiCad parser 无法读取或旧格式没有等价表达时，才删除不支持的节点和字段。
- 有损重写和删除会输出 warning。
- 升级是保守的：转换器不会凭空生成源文件中不存在的新 KiCad 设计特性。

跨 KiCad 5/6 边界时会切换文件族：`.sch -> .kicad_sch`、`.lib/.dcm -> .kicad_sym`、`.pro -> .kicad_pro`，以及反向的 `.kicad_sch -> .sch`、`.kicad_sym -> .lib + .dcm`、`.kicad_pro -> .pro`。

## 工程转换

转换工程目录时，工具只复制可编辑 KiCad 输入和常见本地 3D 模型文件，然后在副本中转换 KiCad 文档。它会跳过制造输出、备份、history、Gerbers、BOM、plot/export 目录和临时文件。

工程级修复包括：

- 按目标文件族规范化 `sym-lib-table` 和 `fp-lib-table`；
- 为 KiCad 6/7/8 生成或规范化 `.kicad_prl` 可见性数据；
- 降级新版 board/footprint 时，将嵌入式 3D 模型资源提取到工程本地 `3D/`
  目录，并把 `kicad-embed://...` 模型 URI 改写为 `${KIPRJMOD}/3D/...`；
- 为 KiCad 6+ 嵌入生成的工程本地 schematic symbols；
- 重建 KiCad 6 风格 schematic hierarchy instances。

## 构建

```powershell
.\build.ps1
```

```sh
./build.sh
```

常用 POSIX 参数：

```sh
./build.sh --clean
./build.sh --config Release
./build.sh --compiler g++-8
./build.sh --static-runtime off
./build.sh --zstd off
```

PowerShell 也支持 `-Zstd auto|on|off`。默认 `auto` 会在存在
`src/third_party/zstd` 时编译内置 zstd 解压支持。若旧工具链无法编译
vendored C 源码，可使用 `off`；此时转换器无法从新版 KiCad 文件中提取嵌入式
3D 模型资源，并会在必须移除不受支持的 embedded model 引用时输出 warning。

脚本读取 `kicad_backport_sources.txt`，使用 `g++` 或 `clang++` 编译，并把可执行文件复制到 `dist/`。实现避免使用旧部署工具链常缺失的新标准库设施，并在需要时从 `-std=c++17` fallback 到 `-std=c++1z`。

## 验证

转换后应使用目标 KiCad 版本打开结果，并运行相关检查。对于 KiCad 8/9/10 工程，通常应运行 schematic ERC 和 PCB DRC：

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

在将降级工程用于生产前，应审查转换器输出的 warning。

## 特别鸣谢

特别感谢 Hubert 在本项目开发过程中提供的帮助。

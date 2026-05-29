# KiCad 文件格式版本差异

本文是 [英文基准文档](kicad-file-format-version-differences.md) 的简体中文版本，用于说明 KiCad Backport C++ 转换器依赖的文件格式版本、主要差异和已实现的降级规则。KiCad 的 S-expression token、源码宏、文件扩展名和版本号保持英文原样，便于检索和对照源码。

最后更新：2026-05-29。

## 来源

- KiCad 官方 tag 和源码版本头文件。
- 本地 KiCad `master`：`10.99.0-1153-g8c1d374f29`。
- `kicad-backport-cplus` 实现：
  - `src/kicad_backport_versions.cpp`
  - `src/kicad_backport_rules.cpp`
  - `src/kicad_backport_rule_rewriters.cpp`
  - `src/kicad_backport.cpp`

版本号来自 KiCad 源码中的活动宏：`SEXPR_SYMBOL_LIB_FILE_VERSION`、`SEXPR_SCHEMATIC_FILE_VERSION`、`SEXPR_BOARD_FILE_VERSION`、`SEXPR_WORKSHEET_FILE_VERSION` 和 `DRC_RULE_FILE_VERSION`。

## 稳定版格式矩阵

| KiCad tag | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

说明：

- board S-expression 版本同时覆盖 `.kicad_pcb` 和 `.kicad_mod`。
- `.kicad_dru` 从 KiCad 6.0 到当前 10.99 源码都保持 `20200610`，这只表示版本宏未变，不表示规则语义没有变化。
- `.kicad_pro` 是 JSON 项目文件，走 settings/schema migration，不使用这些 S-expression 日期版本宏。

## 当前开发版 10.99

| 文件类型 | 10.99/current 版本 | 说明 |
| --- | ---: | --- |
| Board `.kicad_pcb` | `20260513` | Copper thieving zone fill mode |
| Footprint `.kicad_mod` | `20260513` | 使用 PCB S-expression 版本 |
| Schematic `.kicad_sch` | `20260512` | Net chains |
| Symbol library `.kicad_sym` | `20260508` | Native ellipse primitive |
| Worksheet `.kicad_wks` | `20231118` | KiCad 8 cleanup 后未再 bump |
| Design rules `.kicad_dru` | `20200610` | 未发现 10.99 专属 bump |

10.99 相比 KiCad 10 新增的主要格式步骤：

| 版本 | 文件类型 | 差异 |
| ---: | --- | --- |
| 20260410 | Board / footprint | Footprint model block 中的 extruded 3D body metadata |
| 20260508 | Board / footprint | Native PCB ellipse 和 ellipse-arc primitives |
| 20260508 | Schematic / symbol | Native schematic/symbol ellipse 和 ellipse-arc primitives |
| 20260511 | Board | Dielectric frequency-dependent stackup model fields |
| 20260512 | Board | Net chain aggregation block |
| 20260512 | Schematic | Net chain records |
| 20260513 | Board | Copper thieving zone fill mode |

## C++ 转换器实现覆盖

`kicad-backport-cplus` 会按文件类型解析目标版本别名，应用降级规则，再写出目标 `version` 字段。它也接受原始数字格式版本，用于测试某个 parser cutoff。

| Alias | Symbol | Schematic | Board | Footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |

转换器不会升级旧文件。如果源文件数字版本低于目标版本，会原样复制并在报告中保留源版本。

## 文档类型检测

优先使用根 S-expression head：`kicad_symbol_lib`、`kicad_sch`、`kicad_pcb`、`footprint`、`kicad_dru`、`kicad_wks` / `drawing_sheet`。如果 root head 不可识别，再按扩展名 `.kicad_sym`、`.kicad_sch`、`.kicad_pcb`、`.kicad_mod`、`.kicad_dru`、`.kicad_wks` 判断。

转换项目目录或 `.kicad_pro` 时，只复制可编辑的 KiCad 项目输入和常见本地 3D 模型文件；跳过 history、backup、Gerber、fabrication output、BOM 和临时文件。目标为 KiCad 7/8 board 时，还会生成 legacy `.kicad_prl` 可见性设置。

## 已实现的主要降级策略

Symbol library 和 schematic：

- 移除旧 parser 不接受的 `text_box`、embedded files、private flags、rounded rectangles、native ellipses、schematic variants、net chains 等节点。
- 对旧版本回填 legacy property IDs。
- 将 property `hide` 移回 `effects`。
- 将 boolean `hide`、font `bold` / `italic` 从列表形式降级为 presence atom。
- 移除 `generator_version`、`embedded_fonts`、font `face`、`show_name`、`do_not_autoplace`、power class flags、jumper pin flags 等新字段。

Board 和 footprint：

- 移除或降级 text boxes、images、net ties、generated objects、teardrops、tables、tenting、embedded files、component classes、padstacks、via stacks、rule areas、via protection、custom layer counts、rounded rectangles、points、barcodes、backdrill、variants、extruded models、native ellipses、dielectric frequency fields、net chains 和 copper thieving。
- 将 copper thieving fill mode 回退为 polygon fill。
- 将 user layer type qualifier 回退为 `user`。
- 将 footprint `Reference` / `Value` property 回退为 legacy `fp_text`。
- 将 scoped `uuid` 回退为 `tstamp`，将 group/generated `uuid` 回退为 `id`。
- 将 KiCad 7 不支持的 PCB dimensions 转换为可见文本注释。
- 为旧 board 重建数字 netcodes 和 root-level net declarations。

Worksheet：

- 目标早于 `20220228` 时移除 worksheet `font` blocks。

Design rules：

- 当前只检测文件类型和目标版本；由于 tracked 版本中的 `.kicad_dru` 宏保持 `20200610`，尚未实现专用降级规则。

## Warning 和报告语义

每个造成树结构变化的删除或兼容重写都会产生 warning。通用 feature gate 会报告删除节点数量和引入版本。报告字段包括 path、kind、source version、target version、changed 和 warnings。

## 维护规则

添加新 KiCad 版本差异时：

1. 先更新版本矩阵。
2. 再新增类似 `10.0 to 11.0` 或 `10.99 / current to 11.99 / current` 的区间章节。
3. 未发布的 development branch 发现应与 stable tag 分开记录。
4. 新版本影响已有目标降级时，同步更新 backport target summary。
5. `.kicad_pro` JSON schema migration 单独记录。

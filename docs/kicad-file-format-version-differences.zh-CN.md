# KiCad 文件格式版本差异

本文是 [英文基准文档](kicad-file-format-version-differences.md) 的简体中文版本，覆盖 V4/V5 legacy 格式、V6-V10 稳定版本矩阵、当前 development 分支边界和 backport 检查清单。KiCad 的 S-expression token、源码宏、文件扩展名和版本号保持英文原样，便于检索和对照源码。

最后更新：2026-06-04。

## 来源和口径

已核对的本地 KiCad 仓库与分支：

- `E:/WORKS/MY/kicadProject/kicad`
- `origin/4.0`, `4.0.0`
- `origin/5.0`, `origin/5.1`, `5.0.0`, `5.1.0`
- `6.0.0`, `7.0.0`, `8.0.0`, `9.0.0`, `10.0.0`, `origin/10.0`
- 当前 `master` 仅用于识别 10.0 后开发项边界，不纳入 KiCad 10.0 稳定版差异

主要源码证据：

- KiCad 4/5 PCB：`pcbnew/kicad_plugin.h`
- KiCad 6/7 PCB：`pcbnew/plugins/kicad/pcb_plugin.h`
- KiCad 8/9/10 PCB：`pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h`
- KiCad 4/5 legacy 原理图：`eeschema/general.h`, `eeschema/sch_legacy_plugin.h`
- KiCad 6 以后原理图/符号库：`eeschema/sch_file_versions.h`
- 图框：`include/drawing_sheet/ds_file_versions.h`
- 设计规则：`pcbnew/drc/drc_rule_parser.h`
- 旧格式示例：`demos/**.sch`, `demos/**.lib`, `demos/**.kicad_pcb`
- `kicad-backport-cplus` 实现：
  - `src/kicad_backport_versions.cpp`
  - `src/kicad_backport_rules.cpp`
  - `src/kicad_backport_rule_rewriters.cpp`
  - `src/kicad_backport.cpp`

说明：

- `.kicad_pcb` 和 `.kicad_mod` 共用 PCB S-expression 版本号。
- KiCad 4/5 的原理图和符号库不是 `.kicad_sch`/`.kicad_sym`，而是 legacy 文本格式 `.sch`/`.lib`/`.dcm`。
- KiCad 6.0 是最大格式分界点：项目、原理图、符号库、PCB、封装和图框都进入 KiCad 6 风格的 S-expression/JSON 文件族。
- `.kicad_pro` 是 JSON 项目文件，走 settings/schema migration，不使用 S-expression 日期版本宏。
- `.kicad_dru` 从 KiCad 6.0 到 10.0 稳定版中版本宏保持 `20200610`，但规则语义仍可能随 PCB 对象模型变化。

## 主版本文件族矩阵

| KiCad 主版本 | 项目文件 | 原理图 | 符号库 | PCB/封装 | 图框 | 设计规则 | 关键结论 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 4.0 | `.pro` legacy | `.sch` legacy, `EESCHEMA_VERSION=2` | `.lib` `EESchema-LIBRARY Version 2.3`, `.dcm` | `.kicad_pcb`/`.kicad_mod` S-expression, version `4` | legacy drawing sheet | 无独立 `.kicad_dru` | PCB 已是 S-expression；原理图/符号库仍是 legacy。 |
| 5.0/5.1 | `.pro` legacy | `.sch` legacy, `EESCHEMA_VERSION=4` | `.lib` 常见 `Version 2.4`, `.dcm` | `.kicad_pcb`/`.kicad_mod` S-expression, version `20171130` | legacy drawing sheet | 无独立 `.kicad_dru` | PCB 增加自定义焊盘、keepout、多层 keepout、3D model mm offset 等；原理图仍未 S-expression 化。 |
| 6.0 | `.kicad_pro` JSON, `.kicad_prl` JSON | `.kicad_sch` `20211123` | `.kicad_sym` `20211014` | `20211014` | `.kicad_wks` `20210606` | `.kicad_dru` `20200610` | 新原理图/符号库格式上线；旧 `.sch`/`.lib` 作为 legacy 输入迁移。 |
| 7.0 | `.kicad_pro` JSON | `.kicad_sch` `20230121` | `.kicad_sym` `20220914` | `20221018` | `.kicad_wks` `20220228` | `20200610` | 增加文字盒、字体、DNP、仿真模型、net-tie、图像、teardrop 关键字等。 |
| 8.0 | `.kicad_pro` JSON | `.kicad_sch` `20231120` | `.kicad_sym` `20231120` | `20240108` | `.kicad_wks` `20231118` | `20200610` | `generator_version` 和 V8 格式清理；PCB 字段、生成对象、UUID 规范化、teardrop 显式布尔。 |
| 9.0 | `.kicad_pro` JSON | `.kicad_sch` `20250114` | `.kicad_sym` `20241209` | `20241229` | `.kicad_wks` `20231118` | `20200610` | 嵌入文件、表格、规则区域、组件类、复杂 padstack、via stack、任意用户层等。 |
| 10.0 | `.kicad_pro` JSON | `.kicad_sch` `20260306` | `.kicad_sym` `20251024` | `20260206` | `.kicad_wks` `20231118` | `20200610` | 变体、跳线焊盘、条码、via 保护、backdrill、拆分 via 类型、netcode 停写等。 |

## 稳定版 S-expression 版本矩阵

| KiCad tag | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

## V4/V5 legacy 分界

### KiCad 4.0

| 类型 | KiCad 4.0 格式特征 |
| --- | --- |
| 项目 | `.pro` legacy，不是 `.kicad_pro` JSON |
| 原理图 | `.sch`，文件头 `EESchema Schematic File Version 2`，版本宏 `EESCHEMA_VERSION 2`，示例常见 `EELAYER 25 0` |
| 符号库 | `.lib`/`.dcm`，常见头部 `EESchema-LIBRARY Version 2.3` |
| PCB/封装 | `.kicad_pcb`/`.kicad_mod` S-expression，版本 `4` |

PCB/封装版本点：

| 版本 | 变化 |
| ---: | --- |
| 3 | 第一个 PCB S-expression 格式，仍使用 legacy copper stack |
| 4 | 反转铜层栈，`Inner*` 改为反序 `In*`，铜层从 16 层扩展到 32 层 |

回退要点：

- 回退到 V4 原理图和符号库时不能只改 `version`，必须生成 legacy `.sch` 和 `.lib`/`.dcm`。
- V6+ 的 UUID、图元对象、文本盒、规则区域、嵌入文件、变体等都不能直接映射到 V4。
- V4 PCB 根节点示例为 `(kicad_pcb (version 4) (host pcbnew "...") ...)`，`host` 字段仍存在。

### KiCad 5.0/5.1

| 类型 | KiCad 5.x 格式特征 |
| --- | --- |
| 项目 | `.pro` legacy |
| 原理图 | `.sch`，文件头 `EESchema Schematic File Version 4`，版本宏 `EESCHEMA_VERSION 4`，常见 `EELAYER 26 0` |
| 符号库 | `.lib`/`.dcm`，常见 `EESchema-LIBRARY Version 2.4` |
| PCB/封装 | `.kicad_pcb`/`.kicad_mod` S-expression，版本 `20171130` |

PCB/封装版本点：

| 版本 | 变化 |
| ---: | --- |
| 20160815 | netclass 增加 differential pair 设置 |
| 20170123 | `EDA_TEXT` 重构，`hide` 字段位置变化 |
| 20170920 | 长焊盘名和自定义 pad shape |
| 20170922 | keepout zone 可存在于多层 |
| 20171114 | 3D model offset 改为以 mm 保存，不再以 inch 保存 |
| 20171125 | footprint text 可 locked/unlocked |
| 20171130 | 3D model offset 使用 `offset` 参数写出 |

回退要点：

- KiCad 5 仍不是 `.kicad_sch`/`.kicad_sym`；从 V6+ 回退到 V5 仍需要 legacy writer。
- V5 比 V4 更接近现代库表工作流，但文件本体仍是 legacy 文本块。
- 回退 V4 时需要删除或改写 V5 自定义 pad shape、长焊盘名、多层 keepout、V5 3D model offset 写法。

## V6 到 V10 逐版本差异

### 6.0 相对 5.x

KiCad 6 是文件格式重构最大的版本。

| 类型 | KiCad 5.x | KiCad 6.0 |
| --- | --- | --- |
| 项目 | `.pro` legacy | `.kicad_pro` JSON |
| 本地项目状态 | 无统一 `.kicad_prl` | `.kicad_prl` JSON |
| 原理图 | `.sch` legacy | `.kicad_sch` S-expression |
| 符号库 | `.lib`/`.dcm` legacy | `.kicad_sym` S-expression |
| PCB | `.kicad_pcb` S-expression | `.kicad_pcb` S-expression |
| 封装 | `.kicad_mod` S-expression | `.kicad_mod` S-expression |
| 图框 | legacy | `.kicad_wks` S-expression |
| 设计规则 | 无独立 `.kicad_dru` | `.kicad_dru` S-expression-like rules |

5.x 到 6.0 PCB/封装变化：

| 版本 | 变化 |
| ---: | --- |
| 20190331 | hatched zones 和 chamfered round rect pads |
| 20190421 | custom pad 支持曲线 |
| 20190516 | 移除 zone segment count |
| 20190605 | 增加 layer defaults |
| 20190905 | `setup` 中增加 board physical stackup |
| 20190907 | footprint 中增加 keepout areas |
| 20191123 | pad 中保存 pin function |
| 20200104 | fabrication pad property |
| 20200119 | track arcs |
| 20200512 | `page` 改为 `paper` |
| 20200518 | 保存 `hole_to_hole_min` |
| 20200614 | 增加 `fp_rect` 和 `gr_rect` |
| 20200625 | 多层 zone、zone names、island controls |
| 20200628 | 移除 visibility settings |
| 20200724 | footprint 增加 KIID |
| 20200807 | zone hatch advanced settings |
| 20200808 | footprint properties |
| 20200809 | via 和 THT pad 增加 `REMOVE_UNUSED_LAYERS` |
| 20200811 | groups |
| 20200818 | 移除 Status flag bitmap 和 setup counts |
| 20200819 | board-level properties |
| 20200825 | 移除 host information |
| 20200828 | 新 fabrication attributes |
| 20200829 | exported footprint 移除 library name |
| 20200909 | dimension 格式变化 |
| 20200913 | leader dimension |
| 20200916 | center dimension |
| 20200921 | orthogonal dimension |
| 20200922 | layer definition 增加 user name |
| 20201002 | footprint editor 中 footprint 支持 groups |
| 20201114 | filled shapes 成为 first-class 对象 |
| 20201115 | `module` 改为 `footprint`，fill 语法变化 |
| 20201116 | footprint 文件写入 version 和 generator |
| 20201220 | free via token |
| 20210108 | pad locking 从 footprint 移到 pad |
| 20210126 | pad 中与 pinfunction 同时保存 pintype |
| 20210228 | global margins 移回 board 文件 |
| 20210424 | 修正 locked flag 语法，移除括号 |
| 20210606 | overbar 从 `~...~` 改为 `~{...}` |
| 20210623 | polygon 支持读写 arcs |
| 20210722 | group locked flags |
| 20210824 | 3D color opacity |
| 20210925 | `fp_text` locked flag |
| 20211014 | arc formatting |

6.0 原理图/符号库初始差异：

| 文件 | 关键变化 |
| --- | --- |
| `.kicad_sym` | 初始 S-expression 符号库；alternate pin definitions；overbar 新语法；arc formatting |
| `.kicad_sch` | 初始 S-expression 原理图；sheet fields 修正；BOM/board 排除；bus/junction properties；UUID；sheet instance properties；pin/net overbar 新语法；junction UUID |

回退要点：

- V6+ 到 V5/V4 是跨格式族转换，不是 S-expression 版本号回写。
- `page`/`paper`、`module`/`footprint`、`id`/`uuid`、`~...~`/`~{...}`、locked 布尔语法都需要边界转换。
- `.kicad_pro` 到 `.pro` 需要单独处理项目配置，不应与 PCB/SCH 的 S-expression 处理混在一起。

### 7.0 相对 6.0

| 文件类型 | KiCad 6.0.0 | KiCad 7.0.0 |
| --- | ---: | ---: |
| `.kicad_sym` | 20211014 | 20220914 |
| `.kicad_sch` | 20211123 | 20230121 |
| `.kicad_pcb`/`.kicad_mod` | 20211014 | 20221018 |
| `.kicad_wks` | 20210606 | 20220228 |
| `.kicad_dru` | 20200610 | 20200610 |

Symbol library：

| 版本 | 变化 |
| ---: | --- |
| 20220101 | class flags |
| 20220102 | fonts |
| 20220126 | text boxes |
| 20220328 | text box `start/end` 改为 `at/size` |
| 20220331 | text colors |
| 20220914 | symbol unit display names |
| 20220914 | 不再保存 property ID |

Schematic：

| 版本 | 变化 |
| ---: | --- |
| 20220101 | circles、arcs、rects、polys、beziers |
| 20220102 | dash-dot-dot |
| 20220103 | label fields |
| 20220104 | fonts |
| 20220124 | `netclass_flag` 改为 `directive_label` |
| 20220126 | text boxes |
| 20220328 | text box `start/end` 改为 `at/size` |
| 20220331 | text colors |
| 20220404 | default schematic symbol instance data |
| 20220622 | 新 simulation model 格式 |
| 20220820 | 修复 default symbol instance data |
| 20220822 | text objects 支持 hyperlinks |
| 20220903 | field name visibility |
| 20220904 | do not autoplace field option |
| 20220914 | DNP |
| 20220929 | 不再保存 property ID |
| 20221002 | instance data 移回 symbol definition |
| 20221004 | instance data 移回 symbol definition |
| 20221110 | sheet instance data 移到 sheet definition |
| 20221126 | instance data 移除 value 和 footprint |
| 20221206 | simulation model fields V6 到 V7 |
| 20230121 | `SCH_MARKER` sheet path serialization |

PCB/footprint：

| 版本 | 变化 |
| ---: | --- |
| 20211226 | radial dimension |
| 20211227 | thermal relief spoke angle overrides |
| 20211228 | `allow_soldermask_bridges` footprint attribute |
| 20211229 | stroke formatting |
| 20211230 | footprint 中的 dimensions |
| 20211231 | private footprint layers |
| 20211232 | fonts |
| 20220131 | textboxes |
| 20220211 | 结束 V5 zone fill strategy 支持 |
| 20220225 | 移除 `TEDIT` |
| 20220308 | knockout text 和 locked graphic text property |
| 20220331 | plot-on-all-layers selection setting |
| 20220417 | automatic dimension precisions |
| 20220427 | 从 footprint private layers 排除 Edge.Cuts 和 Margin |
| 20220609 | teardrop keywords |
| 20220621 | image support |
| 20220815 | `allow-soldermask-bridges-in-FPs` flag |
| 20220818 | first-class net ties |
| 20220914 | custom-shape pad number boxes |
| 20221018 | via 和 pad zone-layer-connections |

Worksheet：

| 版本 | 变化 |
| ---: | --- |
| 20220228 | font support |

回退要点：

- 回退到 V6 需要删除 DNP、V7 simulation model 字段、text boxes、字体属性、net-tie first-class 存储、image 等。
- 回退到 V6 或更早时，property ID 处理要反向补齐或转为目标格式允许的字段。

### 8.0 相对 7.0

| 文件类型 | KiCad 7.0.0 | KiCad 8.0.0 |
| --- | ---: | ---: |
| `.kicad_sym` | 20220914 | 20231120 |
| `.kicad_sch` | 20230121 | 20231120 |
| `.kicad_pcb`/`.kicad_mod` | 20221018 | 20240108 |
| `.kicad_wks` | 20220228 | 20231118 |
| `.kicad_dru` | 20200610 | 20200610 |

Symbol library：

| 版本 | 变化 |
| ---: | --- |
| 20230620 | `ki_description` 改为 `Description` field |
| 20231120 | `generator_version` 和 V8 cleanups |

Schematic：

| 版本 | 变化 |
| ---: | --- |
| 20230221 | modern power symbols，editable value = net |
| 20230409 | `exclude_from_sim` markup |
| 20230620 | `ki_description` 改为 `Description` field |
| 20230808 | `Sim.Enable` field 移到 `exclude_from_sim` attribute |
| 20230819 | 允许多层 library symbol inheritance depth |
| 20231120 | `generator_version` 和 V8 cleanups |

PCB/footprint：

| 版本 | 变化 |
| ---: | --- |
| 20230410 | schematic DNP attribute 传播到 PCB `attr` |
| 20230517 | pad 和 via teardrop parameters |
| 20230620 | PCB fields |
| 20230730 | graphic shapes connectivity |
| 20230825 | textbox explicit border flag |
| 20230906 | 文件中支持 multiple image types |
| 20230913 | custom-shaped-pad spoke templates |
| 20231007 | generative objects |
| 20231014 | V8 file format normalization |
| 20231212 | reference image locking/UUIDs，footprint boolean format |
| 20231231 | generator 和 group 使用 `uuid` 而不是 `id` |
| 20240108 | teardrop parameters 改为 explicit bools |

Worksheet：

| 版本 | 变化 |
| ---: | --- |
| 20230607 | images 保存为 base64 |
| 20231118 | `generator_version` 和 V8 file format cleanup |

回退要点：

- 回退 V7 要移除根 `generator_version`，并处理 `uuid`/`id`、布尔 presence atom、PCB fields、generative objects、reference image UUID 等。
- `ki_description` 与 `Description` 字段需要双向映射。

### 9.0 相对 8.0

| 文件类型 | KiCad 8.0.0 | KiCad 9.0.0 |
| --- | ---: | ---: |
| `.kicad_sym` | 20231120 | 20241209 |
| `.kicad_sch` | 20231120 | 20250114 |
| `.kicad_pcb`/`.kicad_mod` | 20240108 | 20241229 |
| `.kicad_wks` | 20231118 | 20231118 |
| `.kicad_dru` | 20200610 | 20200610 |

Symbol library：

| 版本 | 变化 |
| ---: | --- |
| 20240529 | embedded files |
| 20240819 | embedded file hash algorithm 改为 Murmur3 |
| 20241209 | private flags for `SCH_FIELD` |

Schematic：

| 版本 | 变化 |
| ---: | --- |
| 20240101 | tables |
| 20240417 | rule areas |
| 20240602 | sheet attributes |
| 20240620 | embedded files |
| 20240716 | multiple netclass assignments |
| 20240812 | netclass color highlighting |
| 20240819 | embedded file hash algorithm 改为 Murmur3 |
| 20241004 | symbols 中 `hide` 使用 booleans |
| 20241209 | private flags for `SCH_FIELD` |
| 20250114 | text variable cross references 使用 full paths |

PCB/footprint：

| 版本 | 变化 |
| ---: | --- |
| 20240201 | overrides 使用 nullable properties |
| 20240202 | tables |
| 20240225 | `solder_paste_margin` rationalization |
| 20240609 | 增加 `tenting` keyword |
| 20240617 | table angles |
| 20240703 | user layer types |
| 20240706 | embedded files |
| 20240819 | embedded file hash algorithm 改为 Murmur3 |
| 20240928 | component classes |
| 20240929 | complex padstacks |
| 20241006 | via stacks |
| 20241007 | tracks 可带 soldermask layer 和 margin |
| 20241009 | placement rule areas 文件格式演进 |
| 20241010 | graphic shapes 可带 soldermask layer 和 margin |
| 20241030 | dimension arrow directions，`suppress_zeroes` normalization |
| 20241129 | normalize `keep_text_aligned` 和 fill properties |
| 20241228 | teardrop curve points 改为 bool |
| 20241229 | user layers 扩展为任意数量 |

回退要点：

- 回退 V8 要删除 embedded files、tables、rule areas、component classes、complex padstack、via stack、任意用户层等。
- 9.0 对 PCB 对象模型影响很大，尤其是 padstack/via stack/user layer，不能只做字段删除。
- 嵌入文件若丢弃，应产生明确 warning 或 sidecar 元数据。

### 10.0 相对 9.0

| 文件类型 | KiCad 9.0.0 | KiCad 10.0.0 |
| --- | ---: | ---: |
| `.kicad_sym` | 20241209 | 20251024 |
| `.kicad_sch` | 20250114 | 20260306 |
| `.kicad_pcb`/`.kicad_mod` | 20241229 | 20260206 |
| `.kicad_wks` | 20231118 | 20231118 |
| `.kicad_dru` | 20200610 | 20200610 |

Symbol library：

| 版本 | 变化 |
| ---: | --- |
| 20250318 | `~` 不再表示 empty text |
| 20250324 | jumper pin groups |
| 20250829 | rounded rectangles |
| 20250901 | stacked pin notation |
| 20250925 | bus alias in project file |
| 20251024 | updated properties formatting：`do_not_autoplace`, `show_name` |

Schematic：

| 版本 | 变化 |
| ---: | --- |
| 20250222 | shape hatched fills |
| 20250227 | local power symbols |
| 20250318 | `~` 不再表示 empty text |
| 20250425 | tables 增加 UUID |
| 20250513 | groups 可带 design block `lib_id` |
| 20250610 | rule areas 增加 DNP 等 flags |
| 20250827 | custom body styles |
| 20250829 | rounded rectangles |
| 20250901 | stacked pin notation |
| 20250922 | schematic variants |
| 20251012 | flat schematic hierarchy support |
| 20251028 | updated properties formatting：`do_not_autoplace`, `show_name` |
| 20260101 | PCB variants |
| 20260306 | corrected variant `in_bom` semantics |

PCB/footprint：

| 版本 | 变化 |
| ---: | --- |
| 20250210 | textboxes 增加 knockout |
| 20250222 | PCB shapes 增加 hatching |
| 20250228 | IPC-4761 via protection features |
| 20250302 | zone hatching offsets |
| 20250309 | component class dynamic assignment rules |
| 20250324 | jumper pads |
| 20250401 | time domain length tuning |
| 20250513 | groups 可带 design block `lib_id` |
| 20250801 | `(island)` 改为 `(island yes/no)` |
| 20250811 | press-fit pad fabrication property support |
| 20250818 | footprint 支持 custom layer counts |
| 20250829 | rounded rectangles |
| 20250901 | PCB point objects |
| 20250907 | tables 增加 UUID |
| 20250909 | footprint unit metadata：`units`/`pins` |
| 20250914 | PCB barcode objects |
| 20250926 | via type 拆分为 blind/buried/through |
| 20251027 | pad-to-die delays 修正缩放保存 |
| 20251028 | 停止写出 netcodes，netcode 成为内部实现细节 |
| 20251101 | backdrill 和 tertiary drill support |
| 20260101 | PCB variants with per-footprint overrides |
| 20260206 | 修正 barcode 和 variant attribute serialization |

图框和设计规则：

- `.kicad_wks` 稳定版本仍为 `20231118`，没有 10.0 稳定版版本号抬升。
- `.kicad_dru` 稳定版本仍为 `20200610`，但 10.0 的 component class、variant、via protection、via type 等会影响规则可引用对象。

回退要点：

- 回退 V9 要移除或降级 variants、jumper pads、barcode、via protection、split via type、backdrill、tertiary drill、custom layer count、PCB points、table UUID 等。
- `netcodes` 停写是 V10 重要分界：回退到 V9 或更早时需要重建 board 根部 `net` 声明和对象上的旧式数值 netcode。
- `~` 空文本语义变化会影响原理图/符号库文本和属性；回退时必须明确区分 literal `~` 与空字符串。

## 10.0 后开发分支边界

当前本地 `master` 已进入 11.0 development，不能混入 V10 稳定清单。已识别但不属于 10.0 稳定版的开发项包括：

| 文件 | 开发版本 | 变化 |
| --- | ---: | --- |
| `.kicad_sym` | 20260508 | native ellipse primitive |
| `.kicad_sch` | 20260512 | net chains |
| `.kicad_pcb`/`.kicad_mod` | 20260410 | extruded 3D body |
| `.kicad_pcb`/`.kicad_mod` | 20260508 | native ellipse primitive |
| `.kicad_pcb`/`.kicad_mod` | 20260511 | dielectric frequency-dependent stackup models |
| `.kicad_pcb`/`.kicad_mod` | 20260512 | net chains |
| `.kicad_pcb`/`.kicad_mod` | 20260513 | copper thieving zone fill mode |
| `.kicad_pcb`/`.kicad_mod` | 20260521 | 焊盘仿真电气类型，写作 pad 上的 `sim_electrical_type` |
| `.kicad_pcb`/`.kicad_mod` | 20260603 | PCB 表格单元格 `knockout` 标志 |

这些项可作为未来 V11 文档的起点；本项目若支持读取 10.99/11.0-dev，应单独建目标版本别名和 warning，不应标记为 KiCad 10.0。

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

### 文档类型检测

优先使用根 S-expression head：`kicad_symbol_lib`、`kicad_sch`、`kicad_pcb`、`footprint`、`kicad_dru`、`kicad_wks` / `drawing_sheet`。如果 root head 不可识别，再按扩展名 `.kicad_sym`、`.kicad_sch`、`.kicad_pcb`、`.kicad_mod`、`.kicad_dru`、`.kicad_wks` 判断。

转换项目目录或 `.kicad_pro` 时，只复制可编辑的 KiCad 项目输入和常见本地 3D 模型文件；跳过 history、backup、Gerber、fabrication output、BOM 和临时文件。目标为 KiCad 7/8 board 时，还会生成 legacy `.kicad_prl` 可见性设置。

### 已实现的主要降级策略

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

### 当前开发版覆盖缺口

基于本地 KiCad `10.99.0-1273-gd90e32b6a0`，以下 current development 格式点已经出现在 KiCad 源码中，但还没有出现在 `kicad-backport-cplus` 的降级 feature gates 中：

| 引入版本 | 缺失的降级处理 | 说明 |
| ---: | --- | --- |
| 20260521 | Pad `sim_electrical_type` | 在 pad 上序列化为 `(sim_electrical_type source)` 或 `(sim_electrical_type sink)`。 |
| 20260603 | 表格单元格 `knockout` 标志 | 必须按 PCB table cell 上下文处理；`knockout` 不能作为全局 token gate，因为其它对象类型也使用这个 token。 |

## Backport 目标检查清单

### 目标 V4/V5

- 必须输出 legacy `.sch`，不能输出 `.kicad_sch`。
- 必须输出 legacy `.lib`/`.dcm` 或明确声明符号库不可无损回退，不能输出 `.kicad_sym`。
- 项目文件应回退为 `.pro` 和 legacy 库配置；`.kicad_pro`/`.kicad_prl` 只能作为源输入或 sidecar。
- PCB/封装版本分别为 V4 `4`、V5 `20171130`，需要处理 `host`、`module`/`footprint`、V5 自定义 pad shape 等边界。
- 所有 V6+ UUID、文本盒、嵌入文件、变体、规则区域、表格、组件类、复杂 padstack、via stack、backdrill 等都需要删除、降级或 sidecar 保存。

### 目标 V6

- `.kicad_sch` 目标版本 `20211123`，`.kicad_sym` 目标版本 `20211014`，PCB/封装目标版本 `20211014`。
- 移除 V7+ 的 DNP、仿真模型 V7 字段、text boxes、fonts、net-tie first-class、image、teardrop 等。
- 注意 overbar 语法、`paper`、`footprint`、UUID 和 locked flag 语法已经是 V6 口径。

### 目标 V7

- 移除 V8+ `generator_version` 和 V8 cleanups。
- `Description`/`ki_description`、`uuid`/`id`、boolean presence atom 等需要按 V7 写法重写。
- 删除 PCB fields、generative objects、reference image UUID、teardrop explicit bool 等 V8+ 构造。

### 目标 V8

- 删除 V9+ embedded files、tables、rule areas、component classes、complex padstack、via stack、任意用户层。
- 处理 Murmur3 hash 前后的 embedded file 差异；回退时一般不能无损保留。
- `.kicad_wks` 已是 `20231118`，通常不用再降版本，但仍要移除 V9/V10 未支持字段。

### 目标 V9

- 删除 V10+ variants、jumper pads、barcodes、via protection、split via type、backdrill、tertiary drill、custom footprint layer counts、PCB points、table UUID。
- 重建 V9 及更早需要的 netcode。
- 将 `(island yes/no)` 等 V10 写法恢复为 V9 可识别写法。

### 目标 V10

- V10 稳定目标使用 `10.0.0`/`origin/10.0` 的版本号：symbol `20251024`，schematic `20260306`，PCB `20260206`。
- 读取 10.99/11.0-dev 时要删掉 native ellipse、net chains、extruded 3D body、copper thieving、dielectric frequency model、pad `sim_electrical_type`、表格单元格 `knockout` 等开发分支项。
- 不要把当前 master 的 `SEXPR_BOARD_FILE_VERSION 20260603` 当成 KiCad 10.0 稳定版。

## Warning 和报告语义

每个造成树结构变化的删除或兼容重写都会产生 warning。通用 feature gate 会报告删除节点数量和引入版本。报告字段包括 path、kind、source version、target version、changed 和 warnings。

## 测试建议

每个目标版本至少准备以下 fixture：

- 一个项目目录：包含项目文件、库表、原理图、PCB、封装和图框。
- 一个最小原理图：覆盖符号属性、字段可见性、层级图纸、文本和网络标签。
- 一个最小符号库：覆盖属性、pin、图形图元、别名或继承关系。
- 一个最小 PCB：覆盖 footprint、pad、via、zone、dimension、graphic shape、netclass。
- 一个封装库目录：覆盖 `.kicad_mod` 独立写出。
- 一个 `.kicad_wks` 图框。
- 一个 `.kicad_dru` 规则文件，至少验证版本保留和未知规则 warning。

断言项目：

- 源版本识别正确。
- 目标文件扩展名正确。
- 根节点或 legacy 文件头正确。
- 目标版本号正确。
- 禁止 token 已删除或改写。
- 需要重建的字段，例如 V9 netcode、V5/V4 legacy `LIBS:`/cache lib、V4/V5 `.pro`，已生成。
- 每个有损转换都有 warning。

## 维护规则

添加新 KiCad 版本差异时：

1. 先更新主版本文件族矩阵和稳定版 S-expression 版本矩阵。
2. 再新增类似 `10.0 to 11.0` 或 `10.99 / current to 11.99 / current` 的区间章节。
3. 未发布的 development branch 发现应与 stable tag 分开记录。
4. 新版本影响已有目标降级时，同步更新 backport 目标检查清单和已实现策略摘要。
5. `.kicad_pro` JSON schema migration 单独记录。

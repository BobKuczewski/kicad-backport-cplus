# KiCad 文件格式迁移实现思路

本文基于 `kicad-file-format-version-differences.md` 中整理的格式差异，
给出相邻 KiCad 主版本之间升级和降级的实现建议。

最重要的边界是 KiCad 5 到 KiCad 6。KiCad 4 和 5 的原理图与符号库仍然
使用 legacy 文件：`.sch`、`.lib`、`.dcm`；KiCad 6 及之后版本改为
S-expression 文件：`.kicad_sch`、`.kicad_sym`。PCB 和 footprint 在这些
版本中一直是 S-expression 文件，但版本号和功能集合仍然需要明确的转换规则。

最后更新：2026-06-04。

## 范围

本文是实现路线图，不是版本支持承诺。它描述如何实现：

- KiCad 6 到 7、KiCad 5 到 6 等正向升级。
- KiCad 7 到 6、KiCad 6 到 5、KiCad 5 到 4 等反向降级。
- 后续相邻版本的扩展点，例如 7 到 8、8 到 9、9 到 10，以及 10 到当前开发分支。

当前 C++ 转换器主要是降级引擎。如果源文件格式版本低于目标版本，它目前会
保持文件不变并复制输出，而不是执行升级。因此升级能力应该作为独立迁移路径
加入，不应通过放宽降级规则来实现。

## 目标版本映射

| KiCad 目标 | 符号库 | 原理图 | PCB / footprint | Worksheet | 设计规则 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 4.0 | Legacy `.lib` 2.3 | Legacy `.sch` V2 | `4` | Legacy drawing sheet | 无 |
| 5.0 / 5.1 | Legacy `.lib` 2.4 | Legacy `.sch` V4 | `20171130` | Legacy drawing sheet | 无 |
| 6.0 | `20211014` | `20211123` | `20211014` | `20210606` | `20200610` |
| 7.0 | `20220914` | `20230121` | `20221018` | `20220228` | `20200610` |
| 8.0 | `20231120` | `20231120` | `20240108` | `20231118` | `20200610` |
| 9.0 | `20241209` | `20250114` | `20241229` | `20231118` | `20200610` |
| 10.0 | `20251024` | `20260306` | `20260206` | `20231118` | `20200610` |

## 迁移引擎形态

建议将迁移实现为带文件族适配器的流水线：

1. 通过根 S-expression head、legacy 扩展名或 legacy 文件头检测文档类型。
2. 解析为可修改的文档树，或解析为 typed intermediate model。
3. 根据源文件族/版本和目标文件族/版本选择迁移路线。
4. 按顺序执行迁移步骤。每个步骤都应由文档类型、源版本、目标版本共同控制。
5. 使用目标文件族、目标版本和目标扩展名写出文件。
6. 对每个有损删除、默认值补充或未实现语义转换输出 warning。

不要把迁移实现成只改顶层版本号。每个版本边界都可能新增或删除 token、
布局结构、对象属性，甚至完整的文件族。

## 跨文件族规则

### KiCad 4/5 Legacy 文件族

KiCad 4 和 5 的原理图与符号库输出需要 legacy writer：

- 原理图：`.sch`。KiCad 4 使用 `EESchema Schematic File Version 2`，
  KiCad 5 使用 `Version 4`。
- 符号库：`.lib` 加可选 `.dcm`。KiCad 4 常见为
  `EESchema-LIBRARY Version 2.3`，KiCad 5 常见为 `Version 2.4`。
- 工程：legacy `.pro`。

V6+ 原理图和符号数据不能靠删掉几个 S-expression 节点降级到 KiCad 4/5。
实现上需要真正的 legacy serializer；如果暂不支持，应给出明确的有损或未实现 warning。

### KiCad 6+ S-Expression 文件族

KiCad 6 及之后版本的原理图、符号库、PCB、footprint、worksheet 和设计规则
都使用 S-expression 文件。大多数相邻版本迁移可以实现为版本门控的树重写：

- 根据目标版本添加或移除 feature node。
- 在格式改名时重命名字段。
- 在 boolean list 和 legacy presence atom 之间互转。
- 为目标版本规范化 UUID、ID、font、text、property 等结构。

## 正向升级策略

正向升级应保留源语义，并只补充安全的目标版本默认值，不应凭空创造新的设计意图。

建议行为：

- 如果源和目标属于同一文件族，解析后写出较新的目标格式，并执行已知兼容性重写。
- 如果源文件缺少某个新格式字段，且新格式允许省略，就保持省略；否则写入 KiCad 风格的默认值。
- 如果迁移跨越 KiCad 4/5 legacy 到 KiCad 6+ S-expression 边界，需要为
  `.sch`、`.lib`、`.dcm`、`.pro` 编写专用 importer。
- 对复杂 legacy import，优先把 KiCad 自身作为参考：用对应版本或 KiCad 6+
  加载旧工程并保存，再将 KiCad 生成结果与转换器输出对比。

### KiCad 6 升级到 KiCad 7

这是同一 S-expression 文件族内的升级，可以实现为结构化重写加目标版本更新。

关键动作：

- 将目标版本更新为 symbol `20220914`、schematic `20230121`、
  board / footprint `20221018`、worksheet `20220228`。
- 保留已有 V6 原理图、符号、PCB、footprint、worksheet 和 DRC 对象。
- 只有当 KiCad 7 要求某个 KiCad 6 不写出的值时，才补充默认值。
- 必要时将旧字段转换为新名称，例如 `netclass_flag` 到 `directive_label`。
- 如果遇到旧的 `start` / `end` text box 表示，应规范化为 KiCad 7 的
  `at` / `size` 表示。
- 保持 V6 simulation 信息有效，但不要在信息不足时伪造完整 KiCad 7 simulation model。
- 仅在目标对象类型支持时，保留或补充 DNP 相关默认值。
- 对 PCB 和 footprint，保留 V6 对象，并在数据足够时转换为 V7 兼容形式，
  包括 stroke formatting、footprint private layers、dimensions、teardrop keywords、
  net ties、pad/via zone-layer connections。
- 对 worksheet，写入 V7 worksheet 版本，并只在能用 KiCad 7 兼容格式表达时保留 font 数据。

验证 fixture：

- 包含 label、field、层级 sheet、simulation field 的 V6 原理图。
- 包含 via、pad、zone、dimension、footprint text 的 V6 PCB。
- 包含 property 和简单图元的 V6 符号库。

### KiCad 5 升级到 KiCad 6

这是最重要的正向迁移边界，因为原理图和符号库会跨文件族。

需要的适配器：

- `.pro` 到 `.kicad_pro` JSON 工程迁移。
- Legacy `.sch` V4 到 `.kicad_sch` `20211123`。
- Legacy `.lib` / `.dcm` 2.4 到 `.kicad_sym` `20211014`。
- `.kicad_pcb` / `.kicad_mod` `20171130` 到 `20211014`。
- 如果支持 worksheet 转换，则需要 legacy drawing sheet 到 `.kicad_wks` `20210606`。

关键动作：

- 将 legacy 原理图记录解析为 typed model 后再写 KiCad 6 S-expression，
  不要对 `.sch` 做面向行的文本替换。
- 将 legacy schematic symbol 映射为 KiCad 6 symbol instance，并生成 UUID、
  property、path、sheet instance、library identifier。
- 在 KiCad 6 需要 UUID 的地方生成稳定 UUID。若要求可复现，可基于源路径和对象身份生成确定性 UUID。
- 将 legacy library alias、field、drawing primitive、pin、documentation record
  转换为 `.kicad_sym` symbol。
- 尽可能将 `.dcm` 中的 description、keyword、documentation link 保存为 KiCad 6 符号元数据。
- 仅转换有明确对应关系的 legacy project setting 到 `.kicad_pro` 和 `.kicad_prl`；
  对 UI/cache 等无法对应的设置输出 warning。
- 将 PCB 版本升级为 `20211014`，并保留 KiCad 5 PCB 功能，例如 custom pad、
  multi-layer keepout、以 `offset` 参数保存的 mm 3D model offset、
  footprint text lock state。

预期有损区域：

- 部分 legacy project setting 可能没有 KiCad 6 JSON 对应项。
- Legacy library rescue/cache 行为可能需要显式 symbol remapping。
- 依赖旧库查找规则的 V5 原理图结构可能需要 warning 或符号库 sidecar 数据。

## 反向降级策略

反向降级应删除或重写目标版本不支持的结构，并报告损失。即使部分新语义无法保留，
输出文件也应该能被目标 KiCad 版本加载。

建议行为：

- 从新到旧应用降级规则，先删除较新的 feature，再执行较旧的兼容性重写。
- Warning 应具体：写明删除了什么 feature、影响多少节点、该 feature 的引入版本。
- 对同一文件族的 S-expression 降级，在目标版本支持时尽量保留对象身份。
- 对 V6+ 到 V5/V4 的原理图和符号降级，应使用专用 legacy writer，
  并将不支持的 V6+ 对象视为有损转换。

### KiCad 7 降级到 KiCad 6

这是同一 S-expression 文件族内的降级，应实现为有针对性的删除和兼容性重写。

目标版本：

- 符号库：`20211014`。
- 原理图：`20211123`。
- PCB / footprint：`20211014`。
- Worksheet：`20210606`。
- 设计规则：`20200610`。

关键降级规则：

- 删除 symbol class flag、symbol font、text box、text color、unit display name
  以及 KiCad 7-only property-ID 行为。
- 删除 V6 之后引入且 V6 无等价表示的 schematic graphic primitive，包括 text box
  和较新的 label field。
- 如果能忠实映射，将 `directive_label` 改回 V6 兼容的 `netclass_flag`；
  否则输出 warning。
- 删除或降级 dash-dot-dot line style、text hyperlink、field name visibility、
  do-not-autoplace option、DNP support、V7 simulation model field。
- 将 symbol 和 sheet instance data 移动或简化为 KiCad 6 期望的布局。
- 删除 PCB text box、image object、first-class net tie、V7 teardrop keyword、
  V6 不能解析的 footprint private-layer 变化，以及 pad/via zone-layer-connection 新增项。
- 将 V7 stroke、font、boolean、lock、hide 格式转换回 V6 兼容形式。
- 删除 `20220228` 引入的 worksheet font support。

预期有损区域：

- Text box、image、net tie、DNP flag、hyperlink、modern simulation model data
  可能没有忠实的 V6 等价表示。
- 部分 V7 PCB dimension 和 teardrop 设置需要删除或展平。

### KiCad 6 降级到 KiCad 5

这是原理图和符号文件的跨文件族降级。

需要的 writer：

- `.kicad_sch` `20211123` 到 legacy `.sch` V4。
- `.kicad_sym` `20211014` 到 legacy `.lib` 2.4 加 `.dcm`。
- `.kicad_pro` JSON 到 legacy `.pro`。
- `.kicad_pcb` / `.kicad_mod` `20211014` 到 `20171130`。

关键降级规则：

- 将 KiCad 6 schematic symbol、wire、bus、label、junction、sheet、field、
  page setting 序列化为 legacy `.sch` record 格式。
- 将 KiCad 6 UUID 和 path 转换为 legacy timestamp 或确定性 identifier。
- 将 KiCad 6 symbol metadata 拆分为 `.lib` symbol definition 和 `.dcm`
  documentation record。
- 删除 legacy 文件无法直接表达的 KiCad 6-only 原理图和符号结构，并逐项 warning。
- 仅将有已知 legacy 对应项的 `.kicad_pro` 设置转换到 `.pro`；
  对 JSON-only 设置忽略或 warning。
- 将 board 和 footprint 版本降级为 `20171130`。
- 删除 KiCad 5 无法解析的 V6 board/footprint 功能，包括 V6+ UUID-only 结构、
  更新的 zone 行为、rule file 假设，以及任何 V5 PCB 格式之后引入的对象。
- 保留 KiCad 5 兼容的 PCB 功能：custom pad、multi-layer keepout、
  使用 `offset` 参数的 mm 3D model offset、locked footprint text。

预期有损区域：

- KiCad 6 原理图和符号 S-expression 细节不一定都能映射到 legacy record 字段。
- Project JSON 设置、现代 UUID 身份、部分库链接细节可能会被简化或丢弃。
- `.kicad_dru` 在 KiCad 5 中没有 standalone 等价文件。

### KiCad 5 降级到 KiCad 4

这主要是 legacy 文件族内部降级，但 PCB 仍需要 S-expression feature 删除。

目标格式：

- 原理图：`.sch` V2。
- 符号库：`.lib` 2.3 加 `.dcm`。
- PCB / footprint：version `4`。
- 工程：legacy `.pro`。

关键降级规则：

- 将原理图文件头从 `EESchema Schematic File Version 4` 改写为 `Version 2`。
- 删除或重写 KiCad 4 无法解析的 KiCad 5 schematic record。
- 将符号库头从 2.4 风格输出降级为 2.3 风格输出。
- 删除 KiCad 4 符号库不存在的 symbol field 或 attribute。
- 将 board 和 footprint 从 `20171130` 降级为 version `4`。
- 删除 KiCad 4 之后引入的 KiCad 5 PCB 功能，包括 net class differential pair
  setting、long pad name、custom pad shape detail、multi-layer keepout、
  KiCad 4 无法解释的 3D model offset 变化、locked/unlocked footprint text。
- 如果存在可逆单位转换，将 3D model offset 转换为 KiCad 4 表示；
  否则 warning 并删除不支持的 offset 字段。

预期有损区域：

- Custom pad 和 multi-layer keepout 可能需要简化或删除。
- Long pad name 可能需要截断或确定性重命名。
- 部分 net-class 和 footprint text-lock 元数据无法保留。

## 后续相邻版本降级模式

同一降级框架可以覆盖后续相邻版本：

| 路线 | 主要实现重点 |
| --- | --- |
| KiCad 8 到 7 | 删除 V8 generator cleanup 字段、PCB fields、generated objects、UUID normalization、custom pad spoke templates、modern teardrops、images、rule-area changes、simulation/exclude flags、worksheet embedded images。 |
| KiCad 9 到 8 | 删除 embedded files、tables、rule areas、component classes、complex padstacks、via stacks、arbitrary user layers、tenting、multiple netclass assignments、netclass color highlighting。 |
| KiCad 10 到 9 | 删除 variants、jumper pads、barcodes、via protection、zone hatch offsets、backdrill、split via types、stopped-netcode assumptions、rounded rectangles、stacked pins、PCB points、property-formatting updates。 |
| 当前开发版到 KiCad 10 | 删除 post-10.0 开发版功能：extruded 3D body metadata、native ellipses and ellipse arcs、dielectric frequency-dependent stackup fields、net chains、copper thieving、pad `sim_electrical_type`、PCB table-cell `knockout`。 |

这些路线的正向升级通常比降级更少损失，但仍然需要兼容性重写和目标默认值。
例如 KiCad 7 到 8 只应在目标格式需要时引入 V8-normalized `generator_version`
处理；KiCad 8 到 9 不应凭空创建 embedded files、component classes 或 padstacks，
除非源语义中已经存在对应信息。

## 测试策略

为每条相邻路线和每种文档类型创建 fixture：

- 工程文件：`.pro` 和 `.kicad_pro`。
- 原理图文件：legacy `.sch` 和 `.kicad_sch`。
- 符号库：`.lib`、`.dcm`、`.kicad_sym`。
- PCB 文件：`.kicad_pcb`。
- Footprint：`.kicad_mod`。
- Worksheet：legacy drawing sheet 和 `.kicad_wks`。
- 设计规则：仅 KiCad 6+ 使用 `.kicad_dru`。

每个测试应验证：

- 源文档类型和源版本检测正确。
- 目标文件族、扩展名、版本正确。
- 目标版本禁止的 token 不存在。
- 已知兼容结构被保留。
- 有损变化会产生 warning。
- 对同一迁移重复执行结果稳定，不会持续改写文件。

对 V5/V6 和 V6/V5 路线，尽可能加入由 KiCad 自身生成的 golden-file 测试。
这些路线跨文件族，是最容易发生语义漂移的高风险区域。

## 推荐实现顺序

建议顺序：

1. 先完成 V7 到 V6 的同文件族 S-expression 降级。
2. 加入 V5 PCB / footprint 到 V4 的降级，因为它仍然属于 PCB S-expression 文件族。
3. 实现 V5 到 V6 升级所需的 legacy schematic 和 symbol parser。
4. 实现 V6 到 V5 降级所需的 legacy schematic 和 symbol writer。
5. 加入 V5 到 V4 的 legacy schematic 和 symbol 降级。
6. 加入 V6 到 V7 等同文件族正向升级。
7. 将同一框架扩展到 V8、V9、V10 和当前开发版路线。

这个顺序可以先交付有用的降级覆盖，同时把更困难的 legacy 原理图和符号转换工作隔离出来。

# KiCad Backport 转换器格式差异与转换处理说明

本文档描述 `kicad-backport` 当前代码实现中真实支持的 KiCad 用户可见文件格式差异、版本锚点和转换处理方式。KiCad 文件格式本身以官方开发者文档为准：

- https://dev-docs.kicad.org/en/file-formats/index.html

## KiCad 开发者速读

| 关注点 | 本转换器的实现口径 |
| --- | --- |
| 格式识别 | 现代文件优先看 S-expression root token：`kicad_sch`、`kicad_symbol_lib`、`kicad_pcb`、`footprint`、`kicad_wks` / `drawing_sheet`。扩展名只作为 fallback。 |
| 版本识别 | 现代 S-expression 文件读取顶层 `(version ...)`；`.kicad_pro` 报告为 `kicad-project-json`；legacy `.sch/.lib/.dcm/.pro` 报告为 legacy 版本别名。 |
| 5/6 分界 | KiCad 6 是 schematic、symbol library、project 文件族切换点；KiCad 4/5 `.sch/.lib/.pro` 与 KiCad 6+ `.kicad_sch/.kicad_sym/.kicad_pro` 不是同一语法族。 |
| PCB/footprint | KiCad 4-10 PCB 和 footprint 都按 S-expression 处理，核心差异是 version anchor、对象集合和字段语法。 |
| `.kicad_pro` | 现代工程 JSON 通常不按目标 KiCad 主版本重写；KiCad 6/7/8 工程转换会清空不受支持的 `kicad-embed://...kicad_wks` 图框路径，KiCad 5/4 目标会写 legacy `.pro`。 |
| `.kicad_wks` | 识别并可改写 version；当前只有 worksheet `font` 降级规则，没有 KiCad 4/5 legacy worksheet writer。 |
| `.kicad_dru` | 设计规则文件会被识别；只有目标版本支持相同 `.kicad_dru` 固定锚点时才复制，不支持的目标会跳过并给出 warning。 |
| 转换粒度 | 同族 S-expression 转换是 AST 级规则重写；legacy 跨族转换是专用 reader/writer；工程目录转换还会补库表、嵌入符号、层级实例、legacy 原理图 cache library 和工程图框引用。 |

## 实现总览

| 主题 | 当前实现 |
| --- | --- |
| 目标版本别名 | 支持 `4.0`、`5.0`、`5.1`、`6.0`、`7.0`、`8.0`、`9.0`、`10.0`、`10.99`，也接受 `v9`、`kicad-9`、`9` 这类写法。 |
| 数字目标版本 | 如果 `--target-version` 是纯数字，则直接作为 S-expression `version` 使用；当源版本小于数字目标时，实现会保留源版本，避免把文件“升级”到没有规则覆盖的任意数字版本。 |
| 文件类型识别 | S-expression 文件优先按根节点识别；没有根节点上下文时按扩展名识别。`.kicad_pro` 和 legacy 文件按文本路径处理，不走 S-expression parser。 |
| 相同版本处理 | 源版本和目标版本相同且同属 S-expression 文件时，只复制文件或保持不变，不执行规则重写。 |
| 同族升级 | S-expression 源版本小于目标版本时执行 `ApplyUpgradeRules()`，只做已有语义可确定的兼容重写，不凭空生成 KiCad 新功能对象。 |
| 同族降级 | S-expression 源版本大于目标版本时执行 `ApplyDowngradeRules()`，删除、改名、扁平化或近似表达目标解析器不能接受的节点/字段。 |
| 旧版跨族转换 | `.sch`、`.lib`、`.dcm`、`.pro` 转到 KiCad 6+ 时写出现代文件族；现代原理图、符号库、工程转到 KiCad 5/4 时写回 legacy 文件族。 |
| 工程目录转换 | 复制可编辑工程输入文件，跳过备份/输出/制造目录；对文档文件按目标版本改扩展名并逐个转换；对库表、本地运行文件、legacy cache library 和工程图框引用做项目级修正。 |

`.kicad_dru` 在代码中可以被识别为 `design-rules`，并有固定版本锚点 `20200610`。工程转换只会在目标版本解析到同一设计规则锚点时复制它；目标文件族不支持 `.kicad_dru` 时会跳过该文件并报告 warning。

## 代码转换思路

| 阶段 | 代码行为 | 设计意图 | 对文档/转换结果的含义 |
| --- | --- | --- | --- |
| 读取 | `loadDocumentImpl()` 先读文本，再按扩展名判断 `.kicad_pro` 和 legacy 文件；其它文件解析为 S-expression。 | 避免把 JSON 工程文件和 legacy 文本格式误当作 S-expression。 | `.kicad_pro` 不做结构化 JSON 重写；legacy `.sch/.lib/.dcm/.pro` 进入专用转换路径。 |
| 类型识别 | `DetectKind()` 优先使用 S-expression 根节点，如 `kicad_sch`、`kicad_pcb`、`footprint`；没有根节点时用扩展名。 | 允许文件名不标准但根节点正确的 S-expression 文件仍被识别。 | 现代文件的“是什么格式”主要由根节点决定，扩展名主要是 fallback 和输出命名依据。 |
| 目标解析 | `ResolveTargetVersion()` 把用户别名解析为“每个文件族自己的格式版本”。 | 同一个 KiCad 主版本下，原理图、符号库、PCB、封装、图框版本号不同。 | 文档不能用单一版本号描述 KiCad 6/7/8/9/10，必须按文件族列版本锚点。 |
| 输出命名 | `withTargetFamilyExtension()` 按目标主版本切换 `.sch/.lib/.pro` 与 `.kicad_sch/.kicad_sym/.kicad_pro`；PCB/封装/图框保持现代扩展名。 | KiCad 5/6 是原理图、符号库、工程文件族的实际分界。 | 跨 5/6 转换不是只改 `version`，而是换文件族和 writer。 |
| 快速复制 | S-expression 源版本与目标版本相同，或 `.kicad_pro` 目标仍为 KiCad 6+ 时，直接复制/原文输出；工程转换仍可能对 KiCad 6/7/8 `.kicad_pro` 清空不受支持的嵌入式图框 URI。 | 避免无意义格式化和不必要规则扰动，同时修复已知工程加载失败。 | 相同版本转换不会全面“清洗”文件；工程 JSON 只有窄范围兼容清理。 |
| legacy 目标 | 目标主版本 `<=5` 且存在 legacy writer 时，现代原理图/符号库/工程转为 legacy 文本；PCB/封装仍走 S-expression 规则。 | KiCad 4/5 原理图、符号库、工程不是现代 S-expression。 | `.kicad_sch -> .sch`、`.kicad_sym -> .lib/.dcm`、`.kicad_pro -> .pro` 是对象级重写。 |
| 现代目标 | legacy 输入目标 `>5` 时，使用 legacy parser 生成现代 S-expression 或 JSON。 | 把 KiCad 4/5 文件提升到 KiCad 6+ 文件族。 | `.sch/.lib/.dcm/.pro` 到现代格式的转换有专门映射和 warning，不是文本替换。 |
| 同族升级 | 源 S-expression 数字版本小于目标版本时，执行 `ApplyUpgradeRules()`。 | 只修正旧语法到新语法，不生成没有源依据的新设计对象。 | 升级是保守规范化；不会自动产生 variants、padstack、barcodes 等新功能。 |
| 同族降级 | 源 S-expression 数字版本大于目标版本时，执行 `ApplyDowngradeRules()`。 | 让旧 KiCad parser 不遇到未知节点/字段。 | 降级通常会删除、改名、压缩或近似表达新功能，并通过 warning 报告。 |
| 版本落盘 | 规则执行后调用 `ensureVersion()` 插入或改写顶层 `(version ...)`。 | 保证输出文件版本锚点与目标一致。 | S-expression 输出会以解析后的目标版本为准，而不是保留源文件 `version`。 |
| 格式化写出 | S-expression 使用 `SEXPR::Format()` 重排输出；JSON/legacy 文本由各自 writer 或原文路径写出。 | 统一现代文件输出格式，避免对非 S-expression 做错误格式化。 | 现代文件可能出现排版变化；legacy/JSON 路径的变化取决于具体 writer。 |
| 报告与 warning | 每个文件生成 `FILE_REPORT`，记录 source/target kind、source/target version、changed 和 warnings。 | 把有损或兼容重写暴露给命令行和 JSON report。 | 文档中的“删除/近似/未完整映射”对应实际 warning 语义。 |

## 转换策略分层

| 层级 | 处理对象 | 核心策略 | 代码中的典型入口 |
| --- | --- | --- | --- |
| 文件族层 | legacy 与现代扩展名、根节点、工程 JSON | 先判定文件族，再决定是复制、跨族 writer，还是 S-expression 规则。 | `loadDocumentImpl()`、`DetectKind()`、`withTargetFamilyExtension()`、`normalizeFile()` |
| 版本锚点层 | `4.0` 到 `10.99` 别名和纯数字版本 | 别名映射到文件族专属版本；纯数字版本只对 S-expression 有意义。 | `ResolveTargetVersion()`、`TargetMajorVersion()`、`DisplayVersionAlias()` |
| legacy 映射层 | `.sch/.lib/.dcm/.pro` 与现代格式互转 | parser 提取可表达对象；writer 输出目标文本结构；不可表达内容 warning。 | `ConvertLegacyToSexprText()`、`ConvertSexprToLegacyText()`、`RewriteLegacyTextForTarget()` |
| S-expression 规则层 | `.kicad_sch/.kicad_sym/.kicad_pcb/.kicad_mod/.kicad_wks` | 根据目标版本阈值删除、改名、展开、压缩、近似节点。 | `ApplyUpgradeRules()`、`ApplyDowngradeRules()` |
| 工程补全层 | 工程目录、库表、局部设置、层级实例、嵌入符号、legacy cache library、工程图框引用 | 文件逐个转换后，再做能让工程打开的项目级修正。 | `copyProjectTree()`、`ensureProjectLocalSymbolLibraryTable()`、`embedProjectLocalSchematicSymbols()`、`rebuildKiCad6ProjectHierarchyInstances()`、`ensureLegacySchematicCacheLibrary()`、`ensureLegacyProjectPageLayoutRefs()` |

## 有损转换原则

| 情况 | 实现选择 | 原因 | 示例 |
| --- | --- | --- | --- |
| 目标版本完全不能解析某节点 | 删除节点或字段，并产生 warning。 | 旧 KiCad 解析器遇到未知结构会失败或行为不可控。 | `embedded_files`、`variants`、`barcodes`、`net_chains`、native ellipse。 |
| 目标版本有近似表达 | 改名、扁平化或转为旧式字段。 | 尽量保留可见或电气含义，而不是直接丢弃。 | `directive_label -> netclass_flag`、`stroke -> width`、`paper/page`、`uuid/tstamp/id`。 |
| 目标版本只支持较弱几何 | 转成旧几何或简单对象。 | 旧格式没有等价图元。 | PCB rectangles 转线段，track arcs 近似为 segments，custom/roundrect pad 降为 rect。 |
| 目标版本只支持旧属性布局 | 移动属性、补 id 或删除新字段。 | KiCad 6/7/8 对 property id、hide、font bool 的语法不同。 | property hide 在 `effects` 和 property 级之间移动；标准属性 id 增删。 |
| 目标解析器只接受较少的填充或绘图形式 | 替换不支持的填充或删除不支持的原理图绘图图元。 | KiCad 6 原理图解析器会拒绝较新版本生成的符号填充颜色和根级原理图绘图图元。 | `(fill (type color) ...)` 降为 `background`；KiCad 6 原理图删除根级 `rectangle`、`circle`、`arc`、`polyline`、`bezier`。 |
| 源文件缺少新语义 | 不主动生成新对象。 | 转换器不能推测用户设计意图。 | 升级到 KiCad 9/10 不自动生成 padstack、variants、component classes、barcodes。 |
| legacy 旧记录无法完整表达现代对象 | writer 输出可表达子集，并通过 warning 暴露。 | KiCad 4/5 `.sch/.lib` 能力远小于现代 S-expression。 | 现代符号图元、属性、实例结构、复杂 schematic 对象降到 legacy 时有损。 |

## 文件族与扩展名

| 文件族 | 现代扩展名 | legacy 扩展名 | 代码中的类型 | 主要转换方式 |
| --- | --- | --- | --- | --- |
| 工程配置 | `.kicad_pro` | `.pro` | `project` / `legacy-project` | `.kicad_pro` 是 JSON 原文处理；legacy `.pro` 可转为最小 KiCad 6+ JSON；现代 JSON 可转为最小 `.pro`。 |
| 原理图 | `.kicad_sch` | `.sch` | `schematic` / `legacy-schematic` | KiCad 6+ 为 S-expression；KiCad 4/5 为 Eeschema legacy 文本记录，跨 5/6 时走专用 parser/writer。 |
| 符号库 | `.kicad_sym` | `.lib`、`.dcm` | `symbol-library` / `legacy-symbol-library` / `legacy-symbol-documentation` | `.lib` 和 `.dcm` 可分别转成 `.kicad_sym`；现代 `.kicad_sym` 降到 legacy 时写 `.lib`，并生成 `.dcm` sidecar。 |
| PCB 板 | `.kicad_pcb` | 无单独 legacy 扩展名 | `board` | KiCad 4-10 都按 S-expression 处理，通过版本号和规则重写完成升降级。 |
| 封装 | `.kicad_mod` | 无单独 legacy 扩展名 | `footprint` | 跟随 PCB 规则；同样是 S-expression 版本号和节点/字段规则重写。 |
| 图框/图纸 | `.kicad_wks` | 无实现中的 legacy writer | `worksheet` | S-expression 版本号可改写；当前只有少量 worksheet 降级规则。 |
| 设计规则 | `.kicad_dru` | 无 | `design-rules` | 只有目标支持相同固定设计规则锚点时复制；不支持 `.kicad_dru` 的目标会跳过并报告 warning。 |
| 库表 | `sym-lib-table`、`fp-lib-table` | 同名 | `library-table` 报告项 | 作为工程目录附加处理：移除/写入版本字段、修正符号库类型/URI、补本地封装库别名。 |
| 本地运行状态 | `.kicad_prl` | 不输出到 KiCad 5/4 | 非主文档 | KiCad 6/7/8 目标会生成或规范化可见层/可见项；KiCad 5/4 目标跳过。 |

## 目标版本锚点

| KiCad 目标 | 符号库 `.kicad_sym` | 原理图 `.kicad_sch` | PCB `.kicad_pcb` | 封装 `.kicad_mod` | 图框 `.kicad_wks` | 说明 |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `4.0` | legacy `.lib` 2.3 | legacy `.sch` V2 | `4` | `4` | 未定义 | 只有 PCB/封装有 S-expression 目标版本；原理图/符号库走 legacy writer。 |
| `5.0` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | 未定义 | KiCad 5/5.1 在实现中 PCB/封装目标相同。 |
| `5.1` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | 未定义 | 与 `5.0` 使用同一格式锚点。 |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | 现代 S-expression 原理图/符号库起点。 |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | 保持现代文件族，字段和对象能力扩展。 |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `generator_version`、UUID/id、PCB 字段等兼容边界。 |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | 嵌入数据、表格、规则区、复杂 PCB 对象等边界。 |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | 当前代码支持的最高常规目标别名。 |
| `10.99` | `20251024` | `20260306` | `20260603` | `20260603` | `20231118` | 开发目标别名；当前只把 PCB/封装推进到 `20260603`，符号库/原理图仍与 `10.0` 相同。 |

## 版本差异与实现处理

| 版本线 | 工程配置 | 原理图 | 符号库 | PCB / 封装 | 图框 | 代码处理要点 |
| --- | --- | --- | --- | --- | --- | --- |
| KiCad 4 | `.pro` | `.sch` V2 | `.lib` 2.3 + `.dcm` 2.0 | `4` | 未定义 | legacy 原理图/符号库只改写头部；PCB/封装降到 `4` 时会移除差分对、keepout、custom/roundrect pad 等 KiCad 4 不兼容内容。 |
| KiCad 5/5.1 | `.pro` | `.sch` V4 | `.lib` 2.4 + `.dcm` 2.0 | `20171130` | 未定义 | legacy 原理图/符号库仍不是 S-expression；现代文件降到 5 时，原理图/符号库/工程写 legacy，PCB/封装写 KiCad 5 S-expression。 |
| KiCad 6 | `.kicad_pro` JSON | `20211123` | `20211014` | `20211014` | `20210606` | 现代文件族起点；降到 6 时会补 KiCad 6 原理图属性/实例兼容结构，移除后续版本解析器不认识的字段。 |
| KiCad 7 | `.kicad_pro` JSON | `20230121` | `20220914` | `20221018` | `20220228` | 增加文本框、DNP、仿真相关字段、stroke、图片、net tie、teardrop 等边界；降到 6 时删除或改写。 |
| KiCad 8 | `.kicad_pro` JSON | `20231120` | `20231120` | `20240108` | `20231118` | 包含 `generator_version`、PCB footprint fields、UUID/id 规范化等边界；降到 7/6 时回退。 |
| KiCad 9 | `.kicad_pro` JSON | `20250114` | `20241209` | `20241229` | `20231118` | embedded files/fonts、tables、rule areas、component classes、padstack/via stack、font face 等边界；降级以删除和兼容重写为主。 |
| KiCad 10 | `.kicad_pro` JSON | `20260306` | `20251024` | `20260206` | `20231118` | variants、jumper/power/position flags、body styles、group/locked、via protection、backdrill、barcodes、net chains、native ellipse 等边界；降到 9 或更早会丢失这些不可表达信息。 |
| KiCad 10.99 | `.kicad_pro` JSON | `20260306` | `20251024` | `20260603` | `20231118` | 仅 PCB/封装增加到 `20260603`；新增 table-cell `knockout` 等规则只按实现表处理。 |

## 转换分派流程

| 输入类型 | 目标版本 | 输出类型/处理 | 版本写入 | 主要损失来源 |
| --- | --- | --- | --- | --- |
| legacy `.sch` | `4.0`/`5.0`/`5.1` | 保持 `.sch`，只按目标主版本重写 `EESchema Schematic File Version 2/4` 头部。 | `legacy-sch-v2` 或 `legacy-sch-v4` 报告值。 | 不深度简化具体记录，保留原始记录可能仍含目标版本不支持内容。 |
| legacy `.lib` | `4.0`/`5.0`/`5.1` | 保持 `.lib`，只按目标主版本重写 `EESchema-LIBRARY Version 2.3/2.4` 头部。 | `legacy-lib-2.3` 或 `legacy-lib-2.4` 报告值。 | 不深度简化具体记录。 |
| legacy `.dcm` | `4.0`/`5.0`/`5.1` | 原文重写输出。 | `legacy-dcm-2.0`。 | 不做内容级迁移。 |
| legacy `.pro` | `4.0`/`5.0`/`5.1` | 原文重写输出。 | `legacy-pro`。 | 不做内容级迁移。 |
| legacy `.sch` | `6.0+` | 转为 `.kicad_sch`。 | 对应目标原理图版本。 | 非连线绘图项提示尚未完全映射；复杂 legacy 行为有损。 |
| legacy `.lib` | `6.0+` | 转为 `.kicad_sym`。 | 对应目标符号库版本。 | 绘图图元和引脚可迁移，但实现警告其转换有损或未完全映射。 |
| legacy `.dcm` | `6.0+` | 转为 `.kicad_sym`，主要从 `$CMP` 生成符号条目。 | 对应目标符号库版本。 | 描述/关键字/文档链接当前没有完整映射。 |
| legacy `.pro` | `6.0+` | 转为最小 `.kicad_pro` JSON。 | `kicad-project-json` 报告值。 | 只保留可识别 legacy 设置和库名，现代工程设置为空骨架。 |
| `.kicad_pro` | `6.0+` | 原文复制；工程转换到 KiCad 6/7/8 时清空嵌入式图框 page-layout URI。 | `kicad-project-json`。 | 不按目标主版本整体改写 JSON 内容；只对 KiCad 6/7/8 清理不支持的 `kicad-embed://...kicad_wks` 工程图框引用。 |
| `.kicad_pro` | `4.0`/`5.0`/`5.1` | 转为带 legacy 工程 section 的 `.pro`。 | `legacy-pro`。 | 只恢复 `legacy.project_settings` 和 legacy 符号库名；否则使用默认值或工程局部库名。 |
| `.kicad_sch` | `4.0`/`5.0`/`5.1` | 转为 `.sch`；工程转换还会添加 schematic cache library 引用。 | `legacy-sch-v2` 或 `legacy-sch-v4`。 | 现代对象、属性、实例和图元能力被压缩到 legacy 可表达子集；多行文本写为单行转义 `\n`，避免旧 Eeschema 把续行当作对象。 |
| `.kicad_sym` | `4.0`/`5.0`/`5.1` | 转为 `.lib`，同时写 `.dcm` sidecar；工程转换会把生成的 `.lib` 复制为 `<project>-cache.lib`。 | `legacy-lib-2.3` 或 `legacy-lib-2.4`。 | 符号属性、复杂图元、层级符号关系会按 legacy 记录近似；legacy `DEF` 引用字段使用 `U`、`BT`、`#PWR` 等前缀，而不是 `U1` 这类实例引用。 |
| `.kicad_pcb` / `.kicad_mod` | 任意已定义目标 | 保持 S-expression 文件族，改写 version 和节点/字段。 | 对应 PCB/封装版本。 | 目标不支持的几何、电气、制造、装配和缓存字段被删除或近似。 |
| `.kicad_wks` | `6.0+` | 保持 `.kicad_wks`，改写 version 并应用少量 worksheet 规则。 | 对应 worksheet 版本。 | 当前仅实现 `font` 在 `<20220228` 时移除；没有 legacy 图框 writer。 |
| `.kicad_dru` | 支持设计规则的目标 | 固定源锚点与目标锚点相同时复制。 | `6.0/7.0/8.0/9.0/10.0/10.99 (1)` 显示别名。 | 尚未实现设计规则格式转换；不支持的目标会跳过并报告 warning。 |

## 原理图转换细节

| 边界/目标 | 实现规则 | 转换效果 |
| --- | --- | --- |
| legacy `.sch` -> KiCad 6+ | 解析 `$Descr`、`$Comp`、字段、wire/bus、bus entry、label、global label、hierarchical label、text、sheet、sheet pin、junction、no-connect 等记录。 | 写出 `.kicad_sch`，生成现代节点、UUID/路径、属性、sheet/symbol instance；非连线绘图项提示未完全映射。 |
| KiCad 6+ -> legacy `.sch` | 从现代节点写 `EESchema Schematic File Version 2/4`、`LIBS:`、`$Descr`、元件、字段、wire/bus、entry、label/text、sheet、sheet pin、junction、no-connect；多行文本写成单行转义 `\n`。 | 现代结构压缩到 legacy 记录，同时避免 KiCad 4 把裸续行解析成非法对象。 |
| 升级到 `>=20240108` | 字体 `bold`/`italic` atom 展开为 boolean list。 | 适配 KiCad 8+ 字体布尔字段写法。 |
| 升级到 `>=20241004` | `pin_names`、`pin_numbers` 下的 `hide` atom 展开为 boolean list。 | 适配新符号可见性字段写法。 |
| 升级到 `>=20241209` | 把 property 的 hide 从 `effects` 移出到 property 级 `hide`。 | 适配 KiCad 9+ 属性隐藏字段位置。 |
| 降级到 `<20260326` | 移除 `locked` 后代字段。 | 避免较旧解析器遇到新锁定字段。 |
| 降级到 `<20260306` | 移除根级 `uuid`、`embedded_fonts`；移除 sheet 的 `exclude_from_sim`、`in_bom`、`on_board`、`dnp`；移除顶层 `group`。 | 回退 KiCad 10 原理图能力并兼容预 10 解析器。 |
| 降级到 `<20250827` | 移除 `body_styles` / `body_style`。 | 回退自定义 body style。 |
| 降级到 `<20250114` | 移除 text/textbox 的 `exclude_from_sim`。 | 回退 KiCad 9/10 文本仿真排除字段。 |
| 降级到 `<20241209` | 添加 legacy property id；把 property hide 移回 `effects`。 | 保持 KiCad 8 及更早属性布局。 |
| 降级到 `<20241004` | `pin_names`、`pin_numbers` 的 `hide` boolean list 扁平为 atom；通用 `hide` boolean list 降级。 | 回退旧符号可见性语法。 |
| 降级到 `<20231120` | 移除 `generator_version`；移除 symbol/sheet 的 `fields_autoplaced`。 | 回退 KiCad 8 清理字段。 |
| 降级到 `<=20230121` | 移除 `exclude_from_sim`；规范化 `uuid` atom 引号；规范化 KiCad 6/7 sheet property 名称和 id；移除 placed symbol pin UUID blocks。 | 回退 KiCad 7 及更早解析器差异。 |
| 降级到 `<20220914` | 移除 symbol 的 `dnp`。 | 回退 KiCad 7 前 DNP 字段。 |
| 降级到 `<20220822` | 移除 text/label/global_label/hierarchical_label/directive_label/netclass_flag 的 `hyperlink`。 | 回退文本超链接字段。 |
| 降级到 `<20220124` | `directive_label` 改名为 `netclass_flag`。 | 回退早期 KiCad 6 原理图字段名。 |
| 降级到 `<=20211123` | 移除 pin 的 `hide` 和 `alternate`；生成 KiCad 6 symbol instance table；标准属性补 id；移除根级原理图绘图图元；降级原理图填充颜色；移除 `instances`。 | 针对 KiCad 6 解析器的兼容修复。 |
| 特性门槛删除 | `text_box`/`textbox`、`simulation_model`/`sim_model`、`table`、`rule_area`、`embedded_files`/`embedded_file`、`private`、`rounded_rectangle`/`roundrect`、`variants`/`variant`、`ellipse`/`ellipse_arc`、`net_chain`/`net_chains`。 | 当目标版本早于这些节点引入版本时，整类节点会被删除并产生 warning。 |

## 符号库转换细节

| 边界/目标 | 实现规则 | 转换效果 |
| --- | --- | --- |
| legacy `.lib` -> KiCad 6+ | 解析 `DEF`、`ALIAS`、`F0/F1/...`、`DRAW`、`X` pin 等记录。 | 写 `.kicad_sym`，保留符号、属性、别名、绘图、引脚；复杂绘图/引脚转换会报告有损或未完全映射。 |
| legacy `.dcm` -> KiCad 6+ | 解析 `$CMP` 名称。 | 写一个 `.kicad_sym`，以组件名生成空符号骨架；描述信息当前未完整映射。 |
| KiCad 6+ -> legacy `.lib/.dcm` | 写 `EESchema-LIBRARY Version 2.3/2.4`、`DEF`、`ALIAS`、标准/自定义 `F` 字段、`DRAW`、`X` pin；另写 `.dcm` 中的 `D/K/F`。`DEF` 引用字段会规范化为 `U`、`BT`、`#PWR` 等前缀，而不是 `U1` 这类实例引用。 | 现代属性、嵌套符号和图元被压缩到 legacy 记录，并能被旧 Eeschema 库加载器解析。 |
| 升级到 `>=20240108` | 字体 `bold`/`italic` atom 展开为 boolean list。 | 适配 KiCad 8+ 字体写法。 |
| 升级到 `>=20241004` | `pin_names`、`pin_numbers` 的 `hide` atom 展开。 | 适配新可见性字段。 |
| 升级到 `>=20241209` | property hide 从 `effects` 移出。 | 适配 KiCad 9+ 属性布局。 |
| 所有升级 | 移除 legacy 标准属性 id。 | 现代目标不保留旧式标准属性 id。 |
| 降级到 `<20231120` | 移除 `generator_version`。 | 回退 KiCad 8 前元数据。 |
| 降级到 `<20241209` | 移除 `embedded_fonts`；添加 legacy property id；property hide 移入 `effects`。 | 回退 KiCad 8 及更早属性布局。 |
| 降级到 `<20230409` | 移除 symbol 的 `exclude_from_sim`。 | 回退早期符号库仿真排除字段。 |
| 降级到 `<20240108` | 字体布尔 list 降为 atom。 | 回退字体语法。 |
| 降级到 `<=20211014` | 移除 pin 的 `hide`，补 KiCad 6 标准属性 id，并降级符号库填充颜色。 | 兼容 KiCad 6 初始符号库格式。 |
| 降级到 `<20251024` | 移除 symbol 的 `in_pos_files`；移除 property 的 `show_name`、`do_not_autoplace`。 | 回退 KiCad 10 属性/定位字段。 |
| 降级到 `<20250324` | 移除 `duplicate_pin_numbers_are_jumpers`。 | 回退 jumper pin-number 字段。 |
| 降级到 `<20250227` | 移除 symbol 的 `power`。 | 回退 power class 字段。 |
| 特性门槛删除 | `text_box`/`textbox`、`embedded_files`/`embedded_file`、`private`、`pin_group`/`pin_groups`、`rounded_rectangle`/`roundrect`、`ellipse`/`ellipse_arc`。 | 目标版本早于引入版本时删除。 |

## PCB 与封装转换细节

| 边界/目标 | 实现规则 | 转换效果 |
| --- | --- | --- |
| 升级通用 | 移除 legacy PCB `host`；`page` 改为 `paper`；legacy arc angle 改为 midpoint arc；删除 legacy line `angle`；给旧 zone fill 补 polygon 标记和 layer。 | 把 KiCad 4/5 风格 PCB/封装结构整理为现代 S-expression 习惯。 |
| 升级到 `>=20231231` | `tstamp` 改名为 `uuid`。 | 适配 KiCad 8+ 对象标识写法。 |
| 升级到 `>=20230410` | 保留 footprint `attr` 中的 `dnp` atom，同时规范化其它受支持的 PCB 布尔字段。 | 当前 KiCad 解析器仍接受 `attr dnp`；避免错误展开为 `(dnp yes/no)`。 |
| 升级到 `>=20251028` | board net 引用从数字 netcode 升级为 net name。 | 适配 KiCad 10+ net 引用写法。 |
| 降级到 `<20260603` | 移除 `table_cell` 的 `knockout`。 | 10.99 PCB/封装开发格式回退。 |
| 降级到 `<20260521` | 移除 pad 的 `sim_electrical_type`。 | 回退 pad 仿真电气类型。 |
| 降级到 `<20260513` | copper thieving fill mode 改为 polygon fill。 | 新 zone fill 模式近似为旧式 polygon。 |
| 降级到 `<20260512` | 删除 `net_chains`/`net_chain`。 | 回退 net chain。 |
| 降级到 `<20260511` | 删除 `spec_frequency`、`dielectric_model`。 | 回退 stackup 频率相关字段。 |
| 降级到 `<20260508` | 删除 `gr_ellipse`、`gr_ellipse_arc`、`fp_ellipse`、`fp_ellipse_arc`。 | 回退 native ellipse 图元。 |
| 降级到 `<20260410` | 移除 typed/extruded 3D model block。 | 回退 extruded/typed 3D 模型。 |
| 降级到 `<20251101` | 删除 `backdrill`、`tertiary_drill`、`front_post_machining`、`back_post_machining`。 | 回退 backdrill/post-machining 字段。 |
| 降级到 `<20251028` | board net name 引用补 legacy netcode。 | 兼容旧版 net 引用。 |
| 降级到 `<20250914` | 删除 `barcode`、`pcb_barcode`、`gr_barcode`、`fp_barcode`。 | 回退 PCB barcode。 |
| 降级到 `<20250909` | 移除 footprint/module 的 `units`。 | 回退 footprint unit pin grouping。 |
| 降级到 `<20250901` | 删除 `point`。 | 回退 PCB point object。 |
| 降级到 `<20250829` | 删除 `rounded_rectangle`、`roundrect`。 | 回退 rounded rectangle 图元/字段。 |
| 降级到 `<20250818` | 删除 `custom_layer_count`、`custom_layer_counts`。 | 回退 custom footprint layer counts。 |
| 降级到 `<20250324` | 移除 footprint 的 `duplicate_pad_numbers_are_jumpers`、`jumper_pad_groups`；删除 `pin_group` 类特性。 | 回退 jumper pad/pin 语义。 |
| 降级到 `<20250228` | 删除 via protection 相关字段；在 `>=20240609` 目标中把 tenting front/back bool list 降为 atom。 | 回退 IPC-4761 via protection。 |
| 降级到 `<20250210` | 移除 PCB text render cache；移除 text box/layer `knockout`；将 cached zone fill 标记为 polygon fill。 | 回退 KiCad 9/10 文本缓存和 knockout。 |
| 降级到 `<=20241229` | 移除 `font face`。 | KiCad 9 及更早 PCB 字体不保留 face。 |
| 降级到 `<20241228` | `teardrops.curved_edges` 改为 `curve_points`，并把 bool 值映射为数字。 | 回退 teardrop 曲线字段。 |
| 降级到 `<20241030` | dimension boolean field 降为 atom；移除 dimension style `arrow_direction`。 | 回退 dimension 写法。 |
| 降级到 `<20241010` | 移除图形对象 `solder_mask_margin`。 | 回退图形 mask margin。 |
| 降级到 `<20241009` | 移除 zone `placement`。 | 回退 rule/placement area 关联。 |
| 降级到 `<20241007` | 移除 track/arc 的 `solder_mask_margin`、`solder_mask_layer`。 | 回退 track mask 字段。 |
| 降级到 `<20240703` | 移除 user-layer type qualifiers。 | 自定义用户层类型回退到旧式表达。 |
| 降级到 `<20240617` | 移除 table cell `angle`。 | 回退 PCB table cell 旋转。 |
| 降级到 `<20240108` | 字体布尔 list 降为 atom；移除 setup `allow_soldermask_bridges_in_footprints`；移除 group `locked`；移除 via layer-connection 字段。 | 兼容 KiCad 7 及更早。 |
| 降级到 `<20231231` | `uuid`/`tstamp`/`id` 去引号；footprint uuid 回 `tstamp`；board group/generated uuid 回 `id`。 | 回退 KiCad 8 前标识写法。 |
| 降级到 `<20231212` | board/footprint boolean locked/hide 降为 presence atom；移除 unlocked；移除 3D model `hide`。 | 回退 KiCad 7/8 边界字段。 |
| 降级到 `<20231014` | 移除 board/footprint `generator_version`。 | 回退 KiCad 8 前元数据。 |
| 降级到 `<20230924` | `pcbplotparams` bool 降级；shape fill `no` 改为 `none`。 | 回退 plot 和 fill 旧语法。 |
| 降级到 `<20230730` | 移除图形对象 net connectivity；对 `gr_rect/fp_rect` 在 KiCad 6/7 范围额外移除 `net`。 | 回退图形联网字段。 |
| 降级到 `<20230620` | PCB footprint fields 降级为 legacy storage。 | 回退 footprint 字段存储。 |
| 降级到 `<=20221018` | 移除 zone `attr`、footprint `net_tie_pad_groups`、pad/via `remove_unused_layers`、pad/zone `thermal_bridge_angle`；`thermal_bridge_width` 改为 `thermal_width`；stroke block 降为 width；dimension 降为图形注记；移除 locked/free 等不兼容字段。 | 兼容 KiCad 7 及更早 PCB 解析器。 |
| 降级到 `<=20171130` | 移除 zone name/fill thickness/island fields、setup stackup、via free、pad 多个现代字段、model opacity、footprint group/zone/property/attr；header/layer syntax 降到 KiCad 5；现代用户层映射为固定用户层；多层 zone 拆为单层；keepout zone 删除；footprint 改名为 module；UUID/tstamp 缩短为 legacy 8 位 ID；arc/rect/track arc 降为旧式 angle/line/segment。 | 兼容 KiCad 5 PCB/封装，损失较多。 |
| 降级到 `<20170922` | 移除 multilayer keepout。 | 兼容 KiCad 4。 |
| 降级到 `<20170920` | custom/roundrect pad 简化为 rectangular pad，并移除 primitives/options/roundrect ratio。 | KiCad 4 不保留自定义焊盘形状。 |
| 降级到 `<20171114` | 3D model offset 降为 KiCad 4 `at` 字段。 | 回退 3D model offset 旧格式。 |
| 降级到 `<20160815` | 移除 netclass 差分对约束。 | 兼容 KiCad 4 早期 PCB 格式。 |

## 图框/图纸转换细节

| 目标/边界 | 实现规则 | 转换效果 |
| --- | --- | --- |
| KiCad 6+ | `.kicad_wks` / `drawing_sheet` 根节点识别为 `worksheet`，目标版本由版本表决定。 | 可改写 `version` 字段并重新格式化。 |
| 降级到 `<20220228` | 移除 `font` 节点。 | 这是当前唯一专门的 worksheet 降级规则。 |
| KiCad 4/5 | 没有 `WORKSHEET` 对应的 legacy 目标版本，也没有 legacy worksheet writer。 | 不能作为独立图框跨到 KiCad 4/5 的完整格式转换路径。 |

## 工程目录转换处理

| 处理阶段 | 实现行为 | 对格式差异的影响 |
| --- | --- | --- |
| 输入判定 | 输入是目录或 `.kicad_pro` 时，按工程目录转换；`.kicad_pro` 输入会使用其父目录作为源目录。 | 工程转换不是只处理一个工程 JSON，而是复制/转换整个可编辑工程树。 |
| 文件筛选 | 复制 `.kicad_sch`、`.kicad_pcb`、`.kicad_sym`、`.kicad_mod`、`.kicad_wks`、`.kicad_pro`、legacy `.sch/.lib/.dcm/.pro`、`sym-lib-table`、`fp-lib-table`、`.kicad_prl`、3D model 文件等。 | 保留可重新打开和编辑工程所需输入。 |
| 目录筛选 | 跳过 `.git`、`.svn`、`.hg`、history、backup、archive、old、gerber、fabrication、outputs、production、plot、bom、assembly、JLCPCB/OSHPark 等目录。 | 不复制制造输出、备份和历史文件。 |
| 目标扩展名 | 目标为 KiCad 5/4 时，现代 schematic/symbol/project 改为 `.sch/.lib/.pro`；目标为 KiCad 6+ 时，legacy `.sch/.lib/.dcm/.pro` 改为 `.kicad_sch/.kicad_sym/.kicad_pro`。 | 文件族跨越 KiCad 5/6 时扩展名随目标族改变。 |
| `.dcm` 与 `.lib` | 目标为 KiCad 6+ 且同目录存在配对 `.lib` 时，`.dcm` 不作为单独文档复制转换。 | 避免 `.lib` 和 `.dcm` 同时生成重复 `.kicad_sym`。 |
| `.kicad_dru` | 目标不支持 `.kicad_dru` 时跳过设计规则文件；目标锚点与源固定锚点相同时复制。 | 避免把不受支持的设计规则文件复制到 legacy 目标，同时保留受支持目标的同格式设计规则。 |
| `.kicad_prl` | 目标为 KiCad 5/4 时跳过；目标为 KiCad 6/7/8 时会生成或规范化 local settings。 | KiCad 6/7/8 需要 numeric `visible_items` 和兼容 `visible_layers`；meta version 对 V6/V7/V8 使用 3。 |
| 工程图框引用 | KiCad 6/7/8 目标会清空 `.kicad_pro` 中的嵌入式 worksheet page-layout 引用。 | 防止 KiCad 6/7/8 尝试加载不支持的 `kicad-embed://...kicad_wks` 工程图框路径。 |
| 符号库表 | 目标为 KiCad 6+ 时，为生成的项目本地 `.kicad_sym` 写 `sym-lib-table`；KiCad 7+ 库表写 `(version 7)`，KiCad 6 移除 version。 | 让转换后的工程能找到项目本地符号库。 |
| 嵌入原理图符号 | 目标为 KiCad 6+ 时，会尝试把项目本地符号库中被原理图引用的符号嵌入到原理图 `lib_symbols`。 | 提高独立打开转换后原理图的成功率。 |
| 层级实例 | 目标为 KiCad 6+ 时，重建 KiCad 6 风格 schematic hierarchy instances。 | 修复层级原理图实例路径/页码兼容性。 |
| legacy 库表 | 目标为 KiCad 6 或更早时，对 `sym-lib-table` / `fp-lib-table` 做 legacy 兼容处理。 | `sym-lib-table` 移除 version；目标 KiCad 5/4 时把符号库 type 改为 `Legacy`、URI 从 `.kicad_sym` 改为 `.lib`；`fp-lib-table` 补项目本地 `Library.pretty` 别名。 |
| legacy 原理图 cache library | KiCad 5/4 目标会把 `Library.lib` 复制为 `<project>-cache.lib`，并向每个生成的 `.sch` 添加 `LIBS:<project>-cache`。 | 改善 KiCad 4/5 的符号解析，包括直接打开原理图和旧 cache library 工作流。 |

## 相邻版本转换摘要

| 转换方向 | 文件族变化 | 实现重点 | 典型有损点 |
| --- | --- | --- | --- |
| KiCad 4 -> KiCad 5 | 原理图/符号库仍为 legacy；PCB/封装从 `4` 到 `20171130`。 | legacy `.sch/.lib` 只改头部；PCB/封装执行升级规则但不会创造新 KiCad 5 特性。 | legacy 记录内部不做深度语义补全。 |
| KiCad 5 -> KiCad 4 | 原理图/符号库仍为 legacy；PCB/封装从 `20171130` 到 `4`。 | legacy `.sch/.lib` 头部、工程/库支持文件会被规范化；PCB/封装删除 KiCad 4 不支持字段并简化 custom pad。 | 差分对、keepout、自定义焊盘、3D offset 新写法等。 |
| KiCad 5 -> KiCad 6 | `.sch/.lib/.dcm/.pro` 跨到 `.kicad_sch/.kicad_sym/.kicad_pro`；PCB/封装到 `20211014`。 | legacy parser 到现代 writer；生成 UUID/实例/属性；工程 JSON 写最小骨架。 | legacy 非连线绘图、符号复杂图元、项目 UI/cache/完整库设置。 |
| KiCad 6 -> KiCad 5 | `.kicad_sch/.kicad_sym/.kicad_pro` 跨回 `.sch/.lib/.pro`；PCB/封装到 `20171130`。 | legacy writer；工程 `.pro` section、schematic cache library 和 PCB header/layer/module/netcode/arc/rect 等回退。 | 现代属性、实例结构、复杂符号/原理图对象、现代 PCB 几何和层信息。 |
| KiCad 6 -> KiCad 7 | 文件族不变，版本提高到 KiCad 7 锚点。 | 升级规则只做旧字段规范化。 | 不生成源文件没有的新 DNP、仿真、图片、teardrop 等意图。 |
| KiCad 7 -> KiCad 6 | 文件族不变，版本降到 KiCad 6 锚点。 | 移除 KiCad 7-only 字段；补 KiCad 6 property id、sheet property、symbol instance 兼容结构，并清理填充颜色和根级绘图图元。 | DNP、仿真字段、超链接、net tie、图片、stroke/dimension、原理图根级绘图图元等。 |
| KiCad 7 -> KiCad 8 | 文件族不变，版本提高到 KiCad 8 锚点。 | 升级标识、字体、属性隐藏等旧写法。 | 不生成 generated objects 或 PCB fields。 |
| KiCad 8 -> KiCad 7 | 文件族不变，版本降到 KiCad 7 锚点。 | 删除 `generator_version`、PCB fields、新 UUID/id 相关写法和 KiCad 8-only 字段。 | generated objects、PCB footprint fields、部分规范化元数据。 |
| KiCad 8 -> KiCad 9 | 文件族不变，版本提高到 KiCad 9 锚点。 | 只做可确定的旧语法升级。 | 不生成 embedded files、tables、component classes、padstack/via stack。 |
| KiCad 9 -> KiCad 8 | 文件族不变，版本降到 KiCad 8 锚点。 | 删除/改写 embedded、table、rule/placement、component class、padstack/via stack、font face、任意用户层类型等。 | 嵌入数据、复杂制造/约束/层类型和高级 pad/via 结构。 |
| KiCad 9 -> KiCad 10 | 文件族不变，版本提高到 KiCad 10 锚点。 | 旧语法升级，必要时 netcode 到 net name。 | 不推测 variants、jumper、backdrill、barcode、via protection 等新设计意图。 |
| KiCad 10 -> KiCad 9 | 文件族不变，版本降到 KiCad 9 锚点。 | 删除 KiCad 10-only 原理图/符号/PCB 字段，net name 回 legacy netcode，制造/装配高级对象移除。 | variants、net chains、native ellipse、barcodes、backdrill、via protection、body styles、position/power/jumper 字段。 |
| KiCad 10 -> KiCad 10.99 | 只对 PCB/封装目标版本提高到 `20260603`。 | 符号库/原理图版本与 `10.0` 相同；PCB/封装允许 `20260603`。 | 不代表完整 KiCad 11 或未来稳定格式。 |
| KiCad 10.99 -> KiCad 10 | PCB/封装从 `20260603` 降到 `20260206`；其他族基本同锚点。 | 移除 10.99-only PCB/封装字段，例如 table-cell `knockout`。 | 开发格式新增字段会被删除。 |

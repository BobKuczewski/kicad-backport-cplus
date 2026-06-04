# KiCad 형식 마이그레이션 구현 전략

이 문서는 영어 기준 문서와 구조적으로 동기화되어 있습니다. KiCad 기술 용어, token,
파일 이름은 오역을 피하기 위해 의도적으로 그대로 둡니다.

이 문서는 `kicad-file-format-version-differences.md`의 형식 차이를 인접한 KiCad
major version 간 migration 구현 지침으로 정리합니다.

가장 중요한 경계는 KiCad 5에서 KiCad 6으로 넘어가는 지점입니다. KiCad 4와 5는 schematic과
symbol library에 legacy file (`.sch`, `.lib`, `.dcm`)을 사용하지만, KiCad 6 이상은
S-expression file (`.kicad_sch`, `.kicad_sym`)을 사용합니다. PCB와 footprint file은 이
버전들에서 이미 S-expression file이지만, feature set과 version number에는 여전히 명시적인
rewrite rule이 필요합니다.

마지막 업데이트: 2026-06-04.

## 범위

이 문서는 구현 로드맵이며 release support 약속이 아닙니다. 다음 구현 방법을 설명합니다.

- KiCad 6에서 7, KiCad 5에서 6 같은 forward migration.
- KiCad 7에서 6, KiCad 6에서 5, KiCad 5에서 4 같은 backward migration.
- 7에서 8, 8에서 9, 9에서 10, 10에서 current development branch 같은 이후 인접 version의
  extension point.

현재 C++ converter는 주로 downgrade engine입니다. source format version이 요청된 target
version보다 오래된 경우, 현재는 upgrade하지 않고 file을 변경 없이 copy합니다. 따라서 upgrade
support는 downgrade rule을 약화하는 방식이 아니라 별도의 migration path로 추가해야 합니다.

## Target Version Map

| KiCad target | Symbol library | Schematic | Board / footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: |
| 4.0 | Legacy `.lib` 2.3 | Legacy `.sch` V2 | `4` | Legacy drawing sheet | 없음 |
| 5.0 / 5.1 | Legacy `.lib` 2.4 | Legacy `.sch` V4 | `20171130` | Legacy drawing sheet | 없음 |
| 6.0 | `20211014` | `20211123` | `20211014` | `20210606` | `20200610` |
| 7.0 | `20220914` | `20230121` | `20221018` | `20220228` | `20200610` |
| 8.0 | `20231120` | `20231120` | `20240108` | `20231118` | `20200610` |
| 9.0 | `20241209` | `20250114` | `20241229` | `20231118` | `20200610` |
| 10.0 | `20251024` | `20260306` | `20260206` | `20231118` | `20200610` |

## Migration Engine Shape

migration은 file-family adapter를 가진 pipeline으로 구현합니다.

1. root S-expression head 또는 legacy extension/header로 document kind를 감지합니다.
2. mutable document tree 또는 typed intermediate model로 parse합니다.
3. source family/version에서 target family/version으로 가는 migration route를 선택합니다.
4. 정렬된 migration step을 적용합니다. 각 step은 document kind, source version,
   target version으로 guard해야 합니다.
5. target file family, target version, target extension으로 파일을 씁니다.
6. lossy removal, fallback default, 구현되지 않은 semantic conversion마다 warning을 냅니다.

migration을 top-level version number rewrite만으로 구현하지 마십시오. 각 version boundary는
token, layout structure, object attribute, 심지어 전체 file family를 추가하거나 제거할 수 있습니다.

## Cross-Family Rules

### KiCad 4/5 Legacy Family

KiCad 4와 5는 schematic과 symbol-library output에 legacy writer가 필요합니다.

- Schematic: `.sch`. KiCad 4는 `EESchema Schematic File Version 2`, KiCad 5는
  `Version 4`.
- Symbol library: `.lib`와 optional `.dcm`. 일반적으로 KiCad 4는
  `EESchema-LIBRARY Version 2.3`, KiCad 5는 `Version 2.4`.
- Project: legacy `.pro`.

V6+ schematic과 symbol data는 몇 개의 S-expression node를 제거하는 것만으로 KiCad 4/5로
downgrade할 수 없습니다. 구현에는 실제 legacy serializer가 필요하며, 지원하지 않는 경우
conversion이 unsupported임을 명확한 warning으로 알려야 합니다.

### KiCad 6+ S-Expression Family

KiCad 6 이상은 schematic, symbol library, PCB, footprint, worksheet, design rules에
S-expression file을 사용합니다. 대부분의 인접 migration은 version-gated tree rewrite로 처리할 수 있습니다.

- target version에 따라 feature node를 추가하거나 제거합니다.
- format이 바뀐 field를 rename합니다.
- boolean-list format과 legacy presence atom을 상호 변환합니다.
- target에 맞게 UUID, ID, font, text, property structure를 normalize합니다.

## Forward Migration Strategy

forward migration은 source semantics를 보존하고 안전한 target default만 합성해야 합니다.
새로운 design intent를 만들어내면 안 됩니다.

권장 동작:

- source와 target이 같은 file family에 있으면 parse 후 알려진 compatibility rewrite를 적용하고
  더 새로운 target format으로 씁니다.
- source에 format field가 없으면 새 format이 생략을 허용할 때는 생략하고, 그렇지 않으면
  KiCad-like default를 씁니다.
- KiCad 4/5 legacy file에서 KiCad 6+ S-expression file로 넘어가면 `.sch`, `.lib`, `.dcm`,
  `.pro`용 dedicated importer를 사용합니다.
- 어려운 legacy import에서는 KiCad 자체를 reference oracle로 사용합니다. 오래된 project를 해당
  KiCad version 또는 KiCad 6+에서 열고 저장한 뒤, 생성된 structure를 converter output과 비교합니다.

### KiCad 6 to KiCad 7 Upgrade

이는 같은 family 안의 S-expression upgrade입니다. structured rewrite와 target version update로
구현할 수 있습니다.

주요 작업:

- target version을 symbol `20220914`, schematic `20230121`, board / footprint
  `20221018`, worksheet `20220228`로 업데이트합니다.
- 기존 V6 schematic, symbol, board, footprint, worksheet, DRC object를 보존합니다.
- KiCad 7이 요구하지만 KiCad 6이 쓰지 않았던 값에만 default를 추가합니다.
- 필요한 경우 `netclass_flag`에서 `directive_label`처럼 old-compatible field를 새 spelling으로 변환합니다.
- 오래된 `start` / `end` text box 표현을 만나면 KiCad 7 `at` / `size` 표현으로 normalize합니다.
- V6 simulation information은 유효하게 유지하되, 충분한 field가 없으면 완전한 KiCad 7
  simulation model data를 만들어내지 않습니다.
- target object type이 지원하는 경우에만 DNP 관련 default를 보존하거나 합성합니다.
- PCB와 footprints에서는 V6 object를 보존하고, 데이터가 충분할 때 stroke formatting,
  footprint private layers, dimensions, teardrop keywords, net ties, pad/via
  zone-layer connections를 V7-compatible form으로 활성화합니다.
- worksheets에서는 V7 worksheet version을 쓰고, KiCad 7-compatible하게 표현 가능한 경우에만
  font data를 보존합니다.

Validation fixtures:

- labels, fields, hierarchical sheets, simulation fields가 있는 V6 schematic.
- vias, pads, zones, dimensions, footprint text가 있는 V6 board.
- properties와 simple primitives가 있는 V6 symbol library.

### KiCad 5 to KiCad 6 Upgrade

이는 forward migration에서 가장 중요한 경계입니다. schematic과 symbol file이 file family를 바꾸기 때문입니다.

필요한 adapter:

- `.pro`에서 `.kicad_pro` JSON project migration.
- Legacy `.sch` V4에서 `.kicad_sch` `20211123`.
- Legacy `.lib` / `.dcm` 2.4에서 `.kicad_sym` `20211014`.
- `.kicad_pcb` / `.kicad_mod` `20171130`에서 `20211014`.
- worksheet conversion을 지원하는 경우 legacy drawing sheet에서 `.kicad_wks` `20210606`.

주요 작업:

- KiCad 6 S-expression을 쓰기 전에 legacy schematic records를 typed model로 parse합니다.
  `.sch` file에 line-oriented text replacement를 사용하지 않습니다.
- legacy schematic symbols를 UUIDs, properties, paths, sheet instances,
  library identifiers를 가진 KiCad 6 symbol instances로 map합니다.
- KiCad 6이 요구하는 곳에 stable UUID를 생성합니다. reproducibility가 중요하면 source path와
  object identity에서 deterministic UUID를 파생합니다.
- legacy library aliases, fields, drawing primitives, pins, documentation records를
  `.kicad_sym` symbols로 변환합니다.
- 가능한 경우 `.dcm` descriptions, keywords, documentation links를 KiCad 6 symbol metadata로 보존합니다.
- 명확한 equivalent가 있는 legacy project settings만 `.kicad_pro`와 `.kicad_prl`로 변환합니다.
  ignored UI/cache settings에는 warning을 냅니다.
- PCB version을 `20211014`로 upgrade하면서 custom pads, multi-layer keepouts,
  3D model offsets, footprint text lock state 같은 KiCad 5 board features를 보존합니다.

예상 lossy areas:

- 일부 legacy project settings에는 KiCad 6 JSON equivalent가 없을 수 있습니다.
- Legacy library rescue/cache behavior는 explicit symbol remapping이 필요할 수 있습니다.
- old library lookup rules에 의존하는 V5 schematic constructs는 warning 또는 symbol-library
  sidecar data가 필요할 수 있습니다.

## Backward Migration Strategy

backward migration은 unsupported constructs를 제거하거나 rewrite하고 loss를 보고해야 합니다.
새로운 semantics 일부를 보존할 수 없더라도 target file은 요청된 KiCad version에서 load 가능해야 합니다.

권장 동작:

- downgrade rule을 newest에서 oldest 순서로 적용하여 나중 feature가 오래된 compatibility rewrite보다
  먼저 제거되도록 합니다.
- warning은 구체적으로 유지합니다. 제거된 feature 이름, affected nodes 수, introduction version을 포함합니다.
- same-family S-expression downgrade에서는 target이 지원하는 곳에서 object identity를 보존합니다.
- V6+에서 V5/V4로 schematic 또는 symbol을 downgrade할 때는 dedicated legacy writer를 사용하고
  unsupported V6+ object를 lossy로 처리합니다.

### KiCad 7 to KiCad 6 Downgrade

이는 같은 family 안의 S-expression downgrade입니다. targeted removal과 compatibility rewrite로 구현해야 합니다.

Target versions:

- Symbol library: `20211014`.
- Schematic: `20211123`.
- Board / footprint: `20211014`.
- Worksheet: `20210606`.
- Design rules: `20200610`.

주요 downgrade rules:

- symbol class flags, symbol fonts, text boxes, text colors, unit display names,
  KiCad 7-only property-ID behavior를 제거합니다.
- V6 이후 도입되었고 V6 equivalent가 없는 schematic graphic primitives를 제거합니다.
  text boxes와 newer label fields를 포함합니다.
- faithful mapping이 있으면 `directive_label`을 V6-compatible `netclass_flag` form으로 되돌리고,
  그렇지 않으면 warning을 냅니다.
- dash-dot-dot line style, text hyperlinks, field name visibility, do-not-autoplace options,
  DNP support, V7 simulation model fields를 제거하거나 downgrade합니다.
- symbol and sheet instance data를 KiCad 6이 기대하는 layout으로 이동하거나 단순화합니다.
- PCB text boxes, image objects, first-class net ties, V7 teardrop keywords, V6가 parse할 수 없는
  footprint private-layer changes, pad/via zone-layer-connection additions를 제거합니다.
- V7 stroke, font, boolean, lock, hide formats를 V6-compatible forms로 변환합니다.
- `20220228`에 추가된 worksheet font support를 제거합니다.

예상 lossy areas:

- Text boxes, images, net ties, DNP flags, hyperlinks, modern simulation model data에는
  faithful V6 equivalent가 없을 수 있습니다.
- 일부 V7 PCB dimensions와 teardrop settings는 제거하거나 flatten해야 합니다.

### KiCad 6 to KiCad 5 Downgrade

이는 schematic과 symbol file의 cross-family downgrade입니다.

필요한 writer:

- `.kicad_sch` `20211123`에서 legacy `.sch` V4.
- `.kicad_sym` `20211014`에서 legacy `.lib` 2.4 plus `.dcm`.
- `.kicad_pro` JSON에서 legacy `.pro`.
- `.kicad_pcb` / `.kicad_mod` `20211014`에서 `20171130`.

주요 downgrade rules:

- KiCad 6 schematic symbols, wires, buses, labels, junctions, sheets, fields,
  page settings를 legacy `.sch` record format으로 serialize합니다.
- target이 요구하는 경우 KiCad 6 UUIDs와 paths를 legacy timestamps 또는 deterministic identifiers로 변환합니다.
- KiCad 6 symbol metadata를 `.lib` symbol definitions와 `.dcm` documentation records로 분리합니다.
- legacy file이 직접 표현할 수 없는 KiCad 6-only schematic과 symbol structures를 제거합니다.
  각 removal마다 warning을 냅니다.
- 알려진 legacy equivalent가 있을 때만 `.kicad_pro` settings를 `.pro`로 변환합니다.
  JSON-only settings는 ignore하거나 warning을 냅니다.
- board와 footprint version을 `20171130`으로 downgrade합니다.
- KiCad 5가 parse할 수 없는 V6 board/footprint features를 제거합니다. V6+ UUID-only structures,
  newer zone behavior, rule file assumptions, V5 PCB format 이후 도입된 object를 포함합니다.
- KiCad 5-compatible PCB features를 보존합니다. custom pads, multi-layer keepouts,
  `offset` parameter를 사용하는 mm 단위 3D model offset, locked footprint text.

예상 lossy areas:

- KiCad 6 schematic과 symbol S-expression details는 legacy record fields에 항상 mapping되지 않습니다.
- Project JSON settings, modern UUID identity, 일부 library linkage details는 줄어들거나 삭제될 수 있습니다.
- `.kicad_dru` files에는 KiCad 5 standalone equivalent가 없습니다.

### KiCad 5 to KiCad 4 Downgrade

이는 대부분 legacy-family downgrade이지만, PCB는 여전히 S-expression feature removal이 필요합니다.

Target formats:

- Schematic: `.sch` V2.
- Symbol library: `.lib` 2.3 plus `.dcm`.
- PCB / footprint: version `4`.
- Project: legacy `.pro`.

주요 downgrade rules:

- schematic header를 `EESchema Schematic File Version 4`에서 `Version 2`로 rewrite합니다.
- KiCad 4가 parse할 수 없는 KiCad 5 schematic records를 제거하거나 rewrite합니다.
- symbol library headers를 2.4-style output에서 2.3-style output으로 downgrade합니다.
- KiCad 4 libraries에 없는 symbol fields 또는 attributes를 제거합니다.
- board와 footprint files를 `20171130`에서 version `4`로 downgrade합니다.
- KiCad 4 이후 도입된 KiCad 5 PCB features를 제거합니다. differential pair settings per net class,
  long pad names, custom pad shape details, multi-layer keepouts, KiCad 4가 해석할 수 없는
  3D model offset changes, locked/unlocked footprint text를 포함합니다.
- reversible unit conversion이 알려져 있으면 3D model offsets를 KiCad 4 representation으로 변환합니다.
  그렇지 않으면 warning을 내고 unsupported offset fields를 제거합니다.

예상 lossy areas:

- Custom pads와 multi-layer keepouts는 단순화하거나 제거해야 할 수 있습니다.
- Long pad names는 truncation 또는 deterministic renaming이 필요할 수 있습니다.
- 일부 net-class와 footprint text-lock metadata는 보존할 수 없습니다.

## Later Adjacent Downgrade Patterns

같은 downgrade framework로 이후 인접 version도 처리해야 합니다.

| Route | Main implementation focus |
| --- | --- |
| KiCad 8 to 7 | V8 generator cleanup fields, PCB fields, generated objects, UUID normalization, custom pad spoke templates, modern teardrops, images, rule-area changes, simulation/exclude flags, worksheet embedded images를 제거합니다. |
| KiCad 9 to 8 | Embedded files, tables, rule areas, component classes, complex padstacks, via stacks, arbitrary user layers, tenting, multiple netclass assignments, netclass color highlighting을 제거합니다. |
| KiCad 10 to 9 | Variants, jumper pads, barcodes, via protection, zone hatch offsets, backdrill, split via types, stopped-netcode assumptions, rounded rectangles, stacked pins, PCB points, property-formatting updates를 제거합니다. |
| Current development to KiCad 10 | Post-10.0 development features를 제거합니다: extruded 3D body metadata, native ellipses and ellipse arcs, dielectric frequency-dependent stackup fields, net chains, copper thieving, pad `sim_electrical_type`, PCB table-cell `knockout`. |

같은 route의 forward migration은 보통 downgrade보다 lossy가 적지만, compatibility rewrite와
target defaults는 여전히 필요합니다. 예를 들어 KiCad 7에서 8은 target format이 기대하는 곳에만
V8-normalized `generator_version` handling을 도입해야 하며, KiCad 8에서 9는 source semantics에
이미 존재하지 않는 embedded files, component classes, padstacks를 만들어내면 안 됩니다.

## Test Strategy

인접 route와 document kind마다 fixture set을 만듭니다.

- Project files: `.pro`와 `.kicad_pro`.
- Schematic files: legacy `.sch`와 `.kicad_sch`.
- Symbol libraries: `.lib`, `.dcm`, `.kicad_sym`.
- PCB files: `.kicad_pcb`.
- Footprints: `.kicad_mod`.
- Worksheets: legacy drawing sheet와 `.kicad_wks`.
- Design rules: KiCad 6+에서만 `.kicad_dru`.

각 test는 다음을 검증해야 합니다.

- source kind와 source version이 올바르게 감지됩니다.
- target file family, extension, version이 올바릅니다.
- target version에서 금지된 token이 없습니다.
- 알려진 compatible structures가 보존됩니다.
- lossy changes가 warning을 생성합니다.
- 같은 migration을 다시 실행해도 stable하며 file이 계속 변경되지 않습니다.

V5/V6와 V6/V5 routes에는 가능한 경우 KiCad 자체가 생성한 golden-file tests를 추가합니다.
이 routes는 file-family boundary를 지나며 semantic drift 위험이 가장 큽니다.

## Implementation Order

권장 순서:

1. V7에서 V6으로의 same-family S-expression downgrades를 완료합니다.
2. V5 PCB / footprint downgrade to V4를 추가합니다. 이는 PCB S-expression family에 남기 때문입니다.
3. V5에서 V6 upgrade를 위한 legacy schematic과 symbol parsers를 구현합니다.
4. V6에서 V5 downgrade를 위한 legacy schematic과 symbol writers를 구현합니다.
5. V5에서 V4 legacy schematic과 symbol downgrade를 추가합니다.
6. V6에서 V7 같은 forward same-family upgrades를 추가합니다.
7. 같은 framework를 V8, V9, V10, current-development routes로 확장합니다.

이 순서는 유용한 downgrade coverage를 일찍 제공하면서, 훨씬 어려운 legacy schematic과
symbol conversion work를 분리합니다.

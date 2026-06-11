# KiCad Backport 변환기 형식 차이와 변환 동작

이 문서는 현재 `kicad-backport` 구현이 실제로 처리하는 KiCad 파일 형식 차이, 버전 앵커, 변환 분기, 재작성 규칙, 손실 가능 경로를 설명합니다. KiCad 파일 형식 자체의 기준 문서는 KiCad 개발자 문서입니다.

- https://dev-docs.kicad.org/en/file-formats/index.html

## KiCad 개발자 빠른 요약

| 항목 | 구현 기준 |
| --- | --- |
| 형식 판별 | 최신 파일은 주로 S-expression root token(`kicad_sch`, `kicad_symbol_lib`, `kicad_pcb`, `footprint`, `kicad_wks` / `drawing_sheet`)으로 판별합니다. 확장자는 fallback 입니다. |
| 버전 판별 | 최신 S-expression 파일은 top-level `(version ...)`을 읽습니다. `.kicad_pro`는 `kicad-project-json`, legacy `.sch/.lib/.dcm/.pro`는 legacy alias 로 보고됩니다. |
| KiCad 5/6 경계 | schematic, symbol library, project 는 KiCad 6에서 파일 패밀리가 바뀝니다. KiCad 4/5 `.sch/.lib/.pro`와 KiCad 6+ `.kicad_sch/.kicad_sym/.kicad_pro`는 다른 문법입니다. |
| PCB / footprint | KiCad 4-10 board 와 footprint 는 S-expression 으로 처리됩니다. 차이는 version anchor, node set, field syntax 입니다. |
| `.kicad_pro` | 최신 project JSON 은 target KiCad major 별로 다시 작성하지 않습니다. KiCad 6+ target 은 보통 원문 복사, KiCad 5/4 target 은 최소 `.pro` 생성입니다. |
| `.kicad_wks` | worksheet 는 감지와 version 재작성이 가능하지만, 전용 downgrade 규칙은 제한적이며 KiCad 4/5 legacy writer 는 없습니다. |
| `.kicad_dru` | 코드에서는 감지 가능하고 anchor `20200610`을 갖지만, 이 문서의 주요 사용자 노출 형식 표에는 포함하지 않습니다. |

## 구현 모델

| 단계 | 구현 | 의미 |
| --- | --- | --- |
| 읽기 | `loadDocumentImpl()`이 텍스트를 읽고 `.kicad_pro` 및 legacy 파일을 확장자로 분기하며, 나머지는 S-expression 으로 parse 합니다. | JSON 또는 legacy text 를 S-expression 으로 오인하지 않습니다. |
| 종류 판별 | `DetectKind()`는 root token 을 우선하고 확장자를 fallback 으로 사용합니다. | root 가 올바르면 파일 이름이 일반적이지 않아도 처리할 수 있습니다. |
| target 해석 | `ResolveTargetVersion()`은 KiCad alias 를 파일 종류별 format version 으로 변환합니다. | 하나의 KiCad release 가 모든 파일에서 같은 version 을 쓰는 것은 아닙니다. |
| 출력 확장자 | `withTargetFamilyExtension()`은 KiCad 5/6 경계에서 `.sch/.lib/.pro`와 `.kicad_sch/.kicad_sym/.kicad_pro`를 전환합니다. | 5/6 변환은 단순한 `(version ...)` 수정이 아닙니다. |
| 같은 version | S-expression source 와 target version 이 같으면 복사하거나 변경하지 않습니다. | 불필요한 formatting churn 을 피합니다. |
| upgrade | source version 이 target 보다 낮으면 `ApplyUpgradeRules()`가 보수적인 문법 정규화를 수행합니다. | 새로운 설계 의도를 생성하지 않습니다. |
| downgrade | source version 이 target 보다 높으면 `ApplyDowngradeRules()`가 삭제, rename, flatten, approximation 을 수행합니다. | 오래된 KiCad parser 가 알 수 없는 node 를 읽지 않게 합니다. |

## target version anchor

| Target | Symbol library | Schematic | Board | Footprint | Worksheet | 비고 |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `4.0` | legacy `.lib` 2.3 | legacy `.sch` V2 | `4` | `4` | undefined | schematic/symbol 은 legacy writer. |
| `5.0` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | undefined | `5.1`과 같은 PCB/footprint anchor. |
| `5.1` | legacy `.lib` 2.4 | legacy `.sch` V4 | `20171130` | `20171130` | undefined | `5.0`과 동일. |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | 최신 schematic/symbol 시작점. |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | 최신 S-expression 확장. |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `generator_version`, UUID/id, PCB fields 경계. |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | embedded data, tables, rule areas, 복잡한 PCB object 경계. |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | 코드가 지원하는 최고 일반 target alias. |
| `10.99` | `20251024` | `20260306` | `20260603` | `20260603` | `20231118` | 개발 alias. board/footprint 만 `10.0`보다 앞섭니다. |

## 파일 패밀리별 변환

| 파일 | 변환 동작 | 주의점 |
| --- | --- | --- |
| `.kicad_pro` | KiCad 6+ target 은 raw JSON 복사, KiCad 5/4 target 은 최소 `.pro` 생성. | target major 별 JSON 재작성은 하지 않습니다. |
| legacy `.pro` | KiCad 6+ 에서 최소 `.kicad_pro` JSON 생성. | 인식 가능한 legacy setting 과 library name 만 보존합니다. |
| `.kicad_sch` | KiCad 5/4 는 legacy `.sch` writer, KiCad 6+ 는 S-expression rules. | 최신 property, instance, 복잡한 object 는 legacy 에서 손실됩니다. |
| legacy `.sch` | KiCad 6+ 는 `.kicad_sch`로 변환, KiCad 5/4 는 header rewrite. | legacy non-wire drawing 은 완전하게 mapping 되지 않습니다. |
| `.kicad_sym` | KiCad 5/4 는 `.lib` 와 `.dcm` 출력. | 최신 symbol property, graphics, nested symbol 은 근사됩니다. |
| legacy `.lib/.dcm` | KiCad 6+ 는 `.kicad_sym` 생성. | `.dcm` 단독 입력은 symbol skeleton 이며 documentation metadata 는 완전하지 않습니다. |
| `.kicad_pcb/.kicad_mod` | 모든 정의된 target 에서 S-expression 을 유지하고 version/node/field 를 rewrite. | 대상이 지원하지 않는 geometry/electrical/manufacturing/cache field 는 삭제 또는 근사됩니다. |
| `.kicad_wks` | KiCad 6+ 에서 version rewrite 와 제한적인 worksheet rule 적용. | KiCad 4/5 legacy worksheet writer 는 없습니다. |

## downgrade 원칙

| 조건 | 구현 선택 | 예 |
| --- | --- | --- |
| target parser 가 node 를 읽을 수 없음 | node 또는 field 를 삭제하고 warning 을 냅니다. | `embedded_files`, `variants`, `barcodes`, `net_chains`, native ellipse. |
| 오래된 표현으로 근사 가능 | rename, flatten, legacy field 변환. | `directive_label -> netclass_flag`, `stroke -> width`, `uuid/tstamp/id`. |
| 오래된 geometry 만 있음 | 단순 geometry 로 변환. | PCB rectangle 을 line 으로, track arc 를 segment 로, custom pad 를 rectangular pad 로. |
| 오래된 property layout | property 이동, ID 추가 또는 삭제. | property hide 위치, standard property id. |
| source 에 새 의미가 없음 | 새 기능 object 를 생성하지 않음. | padstack, variants, component classes, barcodes 는 자동 생성하지 않습니다. |

## project directory 변환

| 처리 | 구현 |
| --- | --- |
| 입력 | directory 또는 `.kicad_pro`는 project tree 로 처리됩니다. |
| 복사 대상 | KiCad document, legacy document, library table, `.kicad_prl`, 3D model file. |
| skip 대상 | VCS, history, backup, archive, gerber/fabrication/output/plot/BOM/assembly/vendor output. |
| 확장자 변환 | KiCad 5/4 target 은 `.kicad_sch/.kicad_sym/.kicad_pro`를 `.sch/.lib/.pro`로, KiCad 6+ 는 반대로 변환합니다. |
| `.dcm` | KiCad 6+ target 에서 대응 `.lib`가 있으면 `.dcm`은 별도로 변환하지 않습니다. |
| library table | `sym-lib-table` / `fp-lib-table`을 target family 에 맞게 정규화합니다. |
| schematic support | KiCad 6+ target 에서 project-local symbol 을 schematic `lib_symbols`에 embed 하고 hierarchy instances 를 재구성합니다. |


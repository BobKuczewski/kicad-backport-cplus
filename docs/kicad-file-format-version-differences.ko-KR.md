# KiCad 파일 형식 버전 차이

이 문서는 [영문 기준 문서](kicad-file-format-version-differences.md)의 한국어 버전입니다. KiCad Backport C++ 변환기가 사용하는 파일 형식 버전, 주요 차이, 구현된 다운그레이드 규칙을 정리합니다. KiCad token, 소스 macro, 확장자, 버전 번호는 검색과 코드 대조를 위해 원문 그대로 둡니다.

최종 업데이트: 2026-05-29.

## 출처

- KiCad 공식 tag 및 소스 version header.
- 로컬 KiCad `master`: `10.99.0-1153-g8c1d374f29`.
- `kicad-backport-cplus` 구현: `kicad_backport_versions.cpp`, `kicad_backport_rules.cpp`, `kicad_backport_rule_rewriters.cpp`, `kicad_backport.cpp`.

버전은 `SEXPR_SYMBOL_LIB_FILE_VERSION`, `SEXPR_SCHEMATIC_FILE_VERSION`, `SEXPR_BOARD_FILE_VERSION`, `SEXPR_WORKSHEET_FILE_VERSION`, `DRC_RULE_FILE_VERSION` macro를 기준으로 합니다.

## 안정 버전 형식 매트릭스

| KiCad tag | `.kicad_sym` | `.kicad_sch` | `.kicad_pcb` / `.kicad_mod` | `.kicad_wks` | `.kicad_dru` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6.0.0` | 20211014 | 20211123 | 20211014 | 20210606 | 20200610 |
| `7.0.0` | 20220914 | 20230121 | 20221018 | 20220228 | 20200610 |
| `8.0.0` | 20231120 | 20231120 | 20240108 | 20231118 | 20200610 |
| `9.0.0` | 20241209 | 20250114 | 20241229 | 20231118 | 20200610 |
| `10.0.0` | 20251024 | 20260306 | 20260206 | 20231118 | 20200610 |

Board S-expression 버전은 `.kicad_pcb`와 `.kicad_mod`에 함께 적용됩니다. `.kicad_dru`는 추적 범위에서 `20200610`으로 유지되지만, 규칙 의미가 변하지 않았다는 뜻은 아닙니다. `.kicad_pro`는 JSON schema migration 방식이므로 별도 추적 대상입니다.

## 10.99 / current

| 파일 유형 | 10.99/current version | 메모 |
| --- | ---: | --- |
| Board `.kicad_pcb` | `20260513` | Copper thieving zone fill mode |
| Footprint `.kicad_mod` | `20260513` | PCB S-expression version 사용 |
| Schematic `.kicad_sch` | `20260512` | Net chains |
| Symbol library `.kicad_sym` | `20260508` | Native ellipse primitive |
| Worksheet `.kicad_wks` | `20231118` | KiCad 8 cleanup 이후 bump 없음 |
| Design rules `.kicad_dru` | `20200610` | 10.99 전용 bump 없음 |

KiCad 10 이후의 주요 추가 항목은 extruded 3D body metadata, native ellipse / ellipse-arc, dielectric frequency fields, net chains, copper thieving입니다.

## C++ 구현 대상 버전

| Alias | Symbol | Schematic | Board | Footprint | Worksheet | Design rules |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `6.0` | `20211014` | `20211123` | `20211014` | `20211014` | `20210606` | `20200610` |
| `7.0` | `20220914` | `20230121` | `20221018` | `20221018` | `20220228` | `20200610` |
| `8.0` | `20231120` | `20231120` | `20240108` | `20240108` | `20231118` | `20200610` |
| `9.0` | `20241209` | `20250114` | `20241229` | `20241229` | `20231118` | `20200610` |
| `10.0` | `20251024` | `20260306` | `20260206` | `20260206` | `20231118` | `20200610` |

변환기는 오래된 파일을 업그레이드하지 않습니다. source version이 target보다 낮으면 파일을 그대로 복사하고 report에 원래 version을 유지합니다.

## 주요 다운그레이드 규칙

Symbol library / schematic:

- 오래된 parser가 읽을 수 없는 `text_box`, embedded files, private flags, rounded rectangles, native ellipses, schematic variants, net chains 등을 제거합니다.
- legacy property IDs를 추가하고 property `hide`를 `effects` 안으로 이동합니다.
- boolean `hide`와 font `bold` / `italic`을 list 형식에서 presence atom으로 되돌립니다.
- `generator_version`, `embedded_fonts`, font `face`, `show_name`, `do_not_autoplace`, power class flags, jumper pin flags를 필요한 경우 제거합니다.

Board / footprint:

- text boxes, images, net ties, generated objects, teardrops, tables, tenting, embedded files, component classes, padstacks, via stacks, rule areas, via protection, custom layer counts, rounded rectangles, points, barcodes, backdrill, variants, extruded models, native ellipses, dielectric frequency fields, net chains, copper thieving을 제거하거나 다운그레이드합니다.
- copper thieving fill mode는 polygon fill로 되돌립니다.
- footprint `Reference` / `Value` property는 legacy `fp_text`로 되돌립니다.
- scoped `uuid`는 `tstamp`로, group/generated `uuid`는 `id`로 되돌립니다.
- KiCad 7에서 지원하지 않는 PCB dimensions는 보이는 text annotation으로 변환합니다.
- 오래된 board 형식을 위해 numeric netcodes와 root-level net declarations를 재구성합니다.

Worksheet는 target이 `20220228`보다 오래된 경우 `font` blocks를 제거합니다. Design rules는 현재 감지와 target version 해석만 수행하며 전용 rewrite는 없습니다.

## Report 및 유지보수

삭제 또는 호환 rewrite가 발생하면 warning이 기록됩니다. report에는 path, kind, source version, target version, changed, warnings가 포함됩니다. 새 KiCad 버전을 추가할 때는 version matrix를 먼저 업데이트하고 stable tag와 development branch 차이를 분리해서 기록해야 합니다.

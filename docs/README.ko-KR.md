# KiCad Backport C++

`kicad-backport`는 portable C++ 로 구현된 독립 명령줄 변환기이며 KiCad 프로젝트와 파일을 오래된 KiCad 파일 형식 target 으로 변환합니다. 목표는 parser compatibility 입니다. 오래된 KiCad 에 동등한 표현이 있으면 그 표현으로 재작성하고, 없으면 지원되지 않는 syntax 를 삭제하거나 근사하며 warning 으로 보고합니다.

구현은 dependency-free 이며 작은 KiCad 스타일 S-expression parser/formatter 를 포함합니다. 직접 실행하거나 plugin wrapper 에서 사용할 수 있습니다.

## 문서

- [Documentation index](README.md)
- [KiCad backport converter format differences](kicad-backport-converter-format-differences.ko-KR.md)
- [English README](../README.md)

## 명령

```text
kicad-backport convert [--quiet] --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

예:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions --json E:\tmp\project
```

`convert`는 단일 KiCad document 또는 project directory 를 받습니다. `.kicad_pro`를 전달하면 해당 project directory 를 변환합니다. 출력 path 에는 `project_V9` 같은 target suffix 가 붙습니다.

`inspect`는 감지된 file kind 와 version 을 보고합니다. `detect-versions`는 file/directory 를 가볍게 scan 하며 JSON 출력도 가능합니다.

## 지원 target

| Target | 현재 동작 |
| --- | --- |
| KiCad 10.99 | 개발 alias. board/footprint 는 `20260603`, schematic/symbol-library 는 KiCad 10 anchor 를 유지합니다. |
| KiCad 10 | `10.0` target anchor 에 속하지 않는 최신 개발 syntax 를 삭제하거나 재작성합니다. |
| KiCad 9 | variants, barcodes, backdrill/post-machining, jumper pad metadata, net-name-only board references 등을 삭제하거나 downgrade 합니다. |
| KiCad 8 | KiCad 9+ tables, embedded files/fonts, component classes, padstacks, via stacks, rule/placement areas, user-layer type qualifiers, font face fields 를 삭제하거나 재작성합니다. |
| KiCad 7 | UUID/tstamp, PCB footprint fields, teardrops, generated objects, images, text boxes, stroke/dimension syntax 호환 처리를 적용합니다. |
| KiCad 6 | 첫 modern schematic/symbol/project family 를 target 으로 하며 필요한 parser compatibility structures 를 추가합니다. |
| KiCad 5.0/5.1 | board/footprint 는 `20171130`; schematic, symbol-library, project 는 legacy `.sch`, `.lib/.dcm`, `.pro`를 씁니다. |
| KiCad 4 | board/footprint 는 `4`; V4 legacy schematic/library header 를 재작성하고 KiCad 5+ PCB construct 를 가능한 범위에서 단순화합니다. |

자세한 내용은 [format differences](kicad-backport-converter-format-differences.ko-KR.md)를 참고하십시오.

## 변환 정책

- target format 이 표현할 수 있는 기존 의도는 보존합니다.
- 오래된 동등 syntax 가 있으면 그것으로 재작성합니다.
- 오래된 KiCad parser 가 읽을 수 없거나 동등 표현이 없을 때만 삭제합니다.
- 손실 rewrite 와 삭제는 warning 으로 출력합니다.
- upgrade 는 보수적이며 source 에 없는 KiCad 기능을 생성하지 않습니다.

KiCad 5/6 경계를 넘으면 file family 가 바뀝니다: `.sch -> .kicad_sch`, `.lib/.dcm -> .kicad_sym`, `.pro -> .kicad_pro`, 반대로 `.kicad_sch -> .sch`, `.kicad_sym -> .lib + .dcm`, `.kicad_pro -> .pro`.

## Project conversion

project directory 변환은 편집 가능한 KiCad input 과 일반 local 3D model file 만 복사한 뒤 복사본 안에서 KiCad document 를 변환합니다. manufacturing output, backup, history, Gerbers, BOM, plot/export directory, temporary file 은 건너뜁니다.

project-level repair 에는 `sym-lib-table` / `fp-lib-table` 정규화, KiCad 6/7/8 용 `.kicad_prl` visibility data, KiCad 6+ 용 project-local schematic symbols embedding, schematic hierarchy instances 재구성이 포함됩니다.

## Build

```powershell
.\build.ps1
```

```sh
./build.sh
```

scripts 는 `kicad_backport_sources.txt`를 읽고 `g++` 또는 `clang++`로 compile 한 뒤 실행 파일을 `dist/`에 복사합니다. 필요하면 `-std=c++17`에서 `-std=c++1z`로 fallback 합니다.

## Validation

변환 후 target KiCad version 으로 열고 필요한 ERC/DRC 를 실행하십시오. production 사용 전 converter warning 을 검토해야 합니다.

## 감사의 말

이 프로젝트 개발 과정에서 도움을 준 Hubert 에게 특별히 감사드립니다.

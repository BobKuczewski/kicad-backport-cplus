# KiCad 백포트 C++

KiCad Backport 다운그레이드 CLI의 독립형 C++17 구현입니다. 도구
최신 KiCad S-표현식 프로젝트 파일을 이전 KiCad 파일 형식으로 변환합니다.
삭제보다 동등한 레거시 구문을 선호합니다.

## 현지화된 문서

- [简体中文](docs/README.zh-CN.md)
- [일본어](docs/README.ja-JP.md)
- [한국어](docs/README.ko-KR.md)
- [프랑스어](docs/README.fr-FR.md)
- [독일어](docs/README.de-DE.md)
- [스페인어](docs/README.es-ES.md)
- [이탈리아어](docs/README.it-IT.md)

추가 참고자료:

- [KiCad 파일 형식 버전 차이](docs/kicad-file-format-version-differences.md)
- [현지화된 파일 형식 버전 차이](docs/README.md#kicad-file-format-version-differences)

## 명령

명령줄 인터페이스는 Go 구현을 반영하며 다음과 같은 목적으로 만들어졌습니다.
직접적으로나 Python 플러그인에서 모두 사용할 수 있습니다.

```text
kicad-backport convert --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

변환기는 KiCad S-표현식 파일을 읽고 버전 기반 다운그레이드를 적용합니다.
규칙을 작성하고 버전 접미사 출력 경로를 작성하며 전체 KiCad 프로젝트를 복사할 수 있습니다.
복사본의 모든 KiCad 파일을 정규화하기 전에 디렉터리. 변환 중에는 각 KiCad
파일에서 감지한 source file version과 해석된 target file version을 출력합니다.
사람이 읽는 텍스트 출력은 원시 파일 형식 번호 대신 `9.0 (20241229)` 또는
`10.99-dev (20260513)` 같은 KiCad alias를 우선 표시합니다. `detect-versions`는
KiCad 관련 파일 종류와 version을 보고하는 데 필요한 만큼만 파일 텍스트를 읽는 빠른
디렉터리 스캔입니다. 텍스트 출력은 같은 alias 표시를 사용하고, JSON report는 원시
파일 형식 version을 유지합니다. 먼저 지원되는 KiCad 확장자로 필터링하고, version을
식별할 수 없는 파일은 결과에서 생략합니다.

예:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions E:\tmp\project
```

지원되는 릴리스 별칭은 `4.0`, `5.0`, `5.1`, `6.0`, `7.0`, `8.0`,
`9.0`, `10.0` 및 `10.99`입니다. 특정 파서 경계를 테스트할 때 원시 KiCad 파일
형식 버전 번호도 전달할 수 있습니다.

## 지원현황

현재 구현은 KiCad 4부터 KiCad 10 파일 계열을 대상으로 합니다.

| 목표 | 상태 |
| --- | --- |
| KiCad 10 | 20260521 패드 `sim_electrical_type` 및 20260603 테이블 셀 `knockout`을 포함하여 10.0 이후/현재 개발 구문을 제거합니다. |
| KiCad 10.99 | 현재 개발판 board/footprint target입니다. board와 footprint는 `20260603`으로 쓰고, symbol library와 schematic은 KiCad 10 target version(`20251024` / `20260306`)을 계속 사용합니다. |
| KiCad 9 | 변형, 바코드, 백드릴/사후 가공, 점퍼 패드 및 넷코드 생략과 같은 KiCad 10/현재 기능을 제거하거나 다운그레이드합니다. |
| KiCad 8 | 스택, 규칙 영역 및 임의의 사용자 레이어 양식을 통해 KiCad 9+ 테이블, 내장 파일, 구성 요소 클래스, 패드 스택을 제거합니다. |
| KiCad 7 | UUID/tstamp 양식, PCB 풋프린트 필드, 눈물 방울, 생성된 개체, 이미지 및 텍스트 상자에 대해 이전 파서 호환성 재작성을 적용합니다. |
| KiCad 6 | 기본 파일 다운그레이드 지원이 거의 완료되었습니다. 변환된 테스트 프로젝트는 검증을 위해 실제 KiCad 6 애플리케이션에서 수동으로 열었습니다. |
| KiCad 5 | board/footprint 대상 version `20171130`과 legacy `.sch`, `.lib`, `.dcm`, `.pro`의 기본 import/export를 지원합니다. 세부 schematic 객체, symbol drawing primitive, pin은 아직 손실 변환이며 warning으로 보고됩니다. |
| KiCad 4 | board/footprint 대상 version `4`, V4 legacy schematic/library header rewrite, V4 출력 suffix/extension을 지원합니다. custom pad 같은 V5-only PCB 기능은 가능한 경우 단순화됩니다. |

## 다운그레이드 정책

변환기는 다음에서 사용 가능한 가장 호환 가능한 표현을 적용합니다.
대상 형식:

- 가능한 경우 새 개체나 필드는 이전의 동등한 구문에 매핑됩니다.
- 가시적/제조 정보는 이전 형식으로 표현할 수 있는 곳에 보관됩니다.
- 지원되지 않는 구문은 이전 KiCad 파서가 로드할 수 없는 경우에만 제거됩니다.
이전 파일 형식에는 동등한 표현이 없습니다.
- 각각의 제거 또는 호환성 재작성은 경고로 보고됩니다.

예를 들어 레거시 넷 코드는 기존 PCB 형식, 최신 부울에 맞게 재구성됩니다.
필드 형태는 필요한 경우 존재 원자로 변환됩니다. KiCad 7 PCB
치수는 눈에 보이는 그래픽으로 유지되며 레거시 프로젝트-로컬 보드
KiCad 6/7/8 대상에 대한 가시성 파일이 생성됩니다.

프로젝트 디렉터리 또는 `.kicad_pro`을(를) 변환할 때 도구는 복사만 합니다.
편집 가능한 KiCad 입력 및 공통 로컬 3D 모델 파일. 생성된 제조
출력, 기록/백업 폴더, Gerber, BOM 및 임시 파일은 건너뜁니다.
KiCad 5/6 경계를 넘을 때는 필요한 경우 확장자를 자동으로 변경합니다.
예: `.sch -> .kicad_sch`, `.lib -> .kicad_sym`, `.kicad_sch -> .sch`,
`.kicad_sym -> .lib/.dcm`, `.kicad_pro -> .pro`.

## 프로젝트 레이아웃

코드는 책임에 따라 분할되므로 이후 KiCad 버전을 추가할 수 있습니다.
작고 국지적인 변경 사항:

- `src/kicad_backport.cpp`: CLI 흐름, 프로젝트 복사/필터링, 파일 변환.
- `src/kicad_backport_document.cpp`: KiCad 문서 종류 감지.
- `src/kicad_backport_legacy.cpp`: legacy KiCad `.sch`, `.lib`, `.dcm`, `.pro` 읽기/쓰기 helper.
- `src/kicad_backport_pathmap.cpp`: 대상 파일 확장자 매핑 도우미.
- `src/kicad_backport_report.cpp`: JSON 보고서 형식입니다.
- `src/kicad_backport_rules.cpp`: 버전 게이트 및 다운그레이드 규칙 순서.
- `src/kicad_backport_rule_rewriters.cpp`: S-표현 트리 재작성 도우미.
- `src/kicad_backport_upgrade.cpp`: 이전 소스 파일에 대한 제한된 구문 정규화.
- `src/kicad_backport_versions.cpp`: KiCad 릴리스 별칭 및 형식 버전.
- `src/kicad_backport_util.cpp`: 공유 문자열, 파일 및 JSON 도우미.
- `src/sexpr.cpp`: 최소한의 KiCad 스타일 S-표현식 파서/포맷터.
- `src/internal/`: 소스 파일에서만 사용되는 비공개 구현 헤더입니다.
- `include/kicad_backport/`: 실행 파일에서 사용되는 공개 프로젝트 헤더입니다.

단일 작업 다운그레이드 규칙은 대신 작은 `applyWhen()` 도우미를 사용합니다.
`std::function`, 힙 할당을 추가하지 않고 규칙을 간결하게 유지합니다.
다중 작업 규칙은 문제를 주문할 때 그룹화된 상태로 유지됩니다.

최상위 구조는 의도적으로 단순합니다.

```text
kicad-backport-cplus/
  include/kicad_backport/   public headers
  src/                      implementation files
  src/internal/             private headers
  build/                    generated build trees
  dist/                     packaged command-line binaries
```

## 빌드

There are two simple direct build entrypoints:

- `build.ps1` for Windows native MinGW/g++ builds.
- `build.sh` for native Linux, Raspberry Pi, and macOS builds.

Native Linux/RPi/macOS build:

```sh
git clone <repo-url> kicad-backport-cplus
cd kicad-backport-cplus
./build.sh --config Release
```

Windows native MinGW/g++ build:

```powershell
git clone <repo-url> kicad-backport-cplus
cd kicad-backport-cplus
.\build.ps1
```

Useful native options:

```sh
./build.sh --clean
./build.sh --compiler g++-8
./build.sh --static-runtime off
```

Outputs are copied to `dist/`. The current source requires C++17 support for
newer standard-library filesystem, view-string, PMR, and memory-resource facilities; it uses a small project-owned path/directory API plus `std::string`. Direct builds fall back from `-std=c++17` to
`-std=c++1z` when needed. Direct builds also probe for supported section garbage collection and symbol stripping flags, enabling them only when the active toolchain accepts them.

Manual direct GCC build:

```sh
./build.sh --config Release --target native
```

## 감사의 말

이 프로젝트 개발 중 도움을 준 Hubert에게 특별히 감사드립니다.

## 확인

변환 후 일치하는 KiCad 버전으로 각 대상의 유효성을 검사합니다. 을 위한
KiCad 8/9/10 이는 일반적으로 회로도 ERC 및 PCB DRC 실행을 의미합니다.

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 CLI에는 더 작은 명령 세트가 있으므로 netlist 및 Gerber 내보내기를 사용하여
변환된 회로도 및 PCB 파일이 로드되는지 확인합니다.

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

KiCad 6에는 CLI 검증 범위가 제한되어 있습니다. PCB 파일의 경우 빠른 파서 검사
KiCad 6의 Python 모듈을 통해 수행할 수 있습니다.

```powershell
& 'D:\KiCad\6.0\bin\python.exe' -c "import pcbnew; pcbnew.LoadBoard(r'E:\tmp\project_V6\project.kicad_pcb'); print('pcb ok')"
```

KiCad 6 회로도 및 기호의 경우 수동 GUI 열기가 가장 유용합니다.
엔드투엔드 검증. 현재 V6 회귀 샘플은 이런 방식으로 확인되었습니다.

ERC/DRC 위반은 프로젝트의 설계 규칙 결과입니다. 그들은 그렇지 않다
KiCad가 로드 또는 구문 분석 오류를 보고하지 않는 한 형식 변환이 실패합니다.

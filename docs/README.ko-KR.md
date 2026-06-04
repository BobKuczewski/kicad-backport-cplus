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
kicad-backport convert --target-version <6.0|7.0|8.0|9.0|10.0|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

변환기는 KiCad S-표현식 파일을 읽고 버전 기반 다운그레이드를 적용합니다.
규칙을 작성하고 버전 접미사 출력 경로를 작성하며 전체 KiCad 프로젝트를 복사할 수 있습니다.
복사본의 모든 KiCad 파일을 정규화하기 전에 디렉터리.

예:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

지원되는 릴리스 별칭은 `6.0`, `7.0`, `8.0`, `9.0` 및 `10.0`입니다. 원시
특정 파일을 테스트할 때 KiCad 파일 형식 버전 번호를 전달할 수도 있습니다.
파서 컷오프.

## 지원현황

현재 구현은 KiCad 6부터 KiCad 10 S-표현 파일을 대상으로 합니다.
가족:

| 목표 | 상태 |
| --- | --- |
| KiCad 10 | 20260521 패드 `sim_electrical_type` 및 20260603 테이블 셀 `knockout`을 포함하여 10.0 이후/현재 개발 구문을 제거합니다. |
| KiCad 9 | 변형, 바코드, 백드릴/사후 가공, 점퍼 패드 및 넷코드 생략과 같은 KiCad 10/현재 기능을 제거하거나 다운그레이드합니다. |
| KiCad 8 | 스택, 규칙 영역 및 임의의 사용자 레이어 양식을 통해 KiCad 9+ 테이블, 내장 파일, 구성 요소 클래스, 패드 스택을 제거합니다. |
| KiCad 7 | UUID/tstamp 양식, PCB 풋프린트 필드, 눈물 방울, 생성된 개체, 이미지 및 텍스트 상자에 대해 이전 파서 호환성 재작성을 적용합니다. |
| KiCad 6 | 기본 파일 다운그레이드 지원이 거의 완료되었습니다. 변환된 테스트 프로젝트는 검증을 위해 실제 KiCad 6 애플리케이션에서 수동으로 열었습니다. |
| KiCad 5 및 이전 버전 | 아직 구현되지 않았습니다. 이제 코드는 레거시 문서 감지, 경로 매핑, 업그레이드/다운그레이드 규칙을 분리하여 향후 V5 지원을 준비합니다. |

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

## 프로젝트 레이아웃

코드는 책임에 따라 분할되므로 이후 KiCad 버전을 추가할 수 있습니다.
작고 국지적인 변경 사항:

- `src/kicad_backport.cpp`: CLI 흐름, 프로젝트 복사/필터링, 파일 변환.
- `src/kicad_backport_document.cpp`: KiCad 문서 종류 감지.
- `src/kicad_backport_legacy.cpp`: 레거시 KiCad 문서 로드 스캐폴딩.
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
  scripts/                  cross-build environment setup
  build/                    generated build trees
  dist/                     packaged command-line binaries
```

## 짓다

현재 플랫폼에서 빌드:

```powershell
.\build.ps1
```

```sh
./build.sh
```

사전에 가장 작은 실용적인 툴체인을 자동으로 감지하고 설치하려면
건물:

```powershell
.\build.ps1 -SetupMissingTools
```

```sh
./build.sh --setup
```

두 스크립트 모두 표준 릴리스 대상을 시도하고 성공적인 출력을
`dist/` 플러그인 호환 이름 사용:

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

이전 빌드를 제거하려면 `.\build.ps1 -Clean` 또는 `./build.sh --clean`을 사용하세요.
재구축 전 출력.

C++ 크로스 컴파일에는 플랫폼 툴체인이 필요합니다. Windows에서는 `build.ps1`
Visual Studio를 사용하여 `windows-amd64` 및 `windows-arm64`을 빌드하고
WSL 도구 체인을 사용할 수 있는 경우 WSL을 통한 `linux-amd64`/`linux-arm64`입니다.
Linux에서 `build.sh`은 기본 Linux를 빌드하고 다음과 같은 경우 `linux-arm64`을 빌드할 수 있습니다.
`aarch64-linux-gnu-g++`이(가) 설치되었습니다. macOS에서 `build.sh`은 Darwin을 빌드합니다.
amd64/arm64와 Apple SDK. Darwin 바이너리는 macOS에서 생성되어야 합니다.

하위 집합을 작성하려면 다음을 수행하십시오.

```powershell
.\build.ps1 -Targets windows-amd64,windows-arm64
```

```sh
TARGETS="linux-amd64 linux-arm64" ./build.sh
```

크로스 빌드 환경 설정:

```powershell
.\scripts\setup-cross.ps1
.\scripts\setup-cross.ps1 -CheckOnly
```

```sh
./scripts/setup-cross.sh
./scripts/setup-cross.sh --check-only
```

설정 스크립트는 가장 작은 실제 빌드 도구 체인을 자동으로 설치합니다.
호스트 플랫폼용. 누락된 내용만 보고하려면 `-CheckOnly` 또는 `--check-only`을(를) 사용하세요.
아무것도 설치하지 않고 도구.

Windows에서 설정 스크립트는 CMake, Visual Studio C++를 설치하거나 준비합니다.
Linux에 필요한 빌드 도구, WSL, Ubuntu 및 최소 WSL 패키지
amd64/arm64 빌드. Linux에서는 기본 C++ 컴파일러인 CMake, Ninja,
호스트 패키지에서 지원하는 aarch64 Linux 크로스 컴파일러
관리자. macOS에서는 Apple 명령줄 도구를 트리거하고 CMake/Ninja를 설치합니다.
가능한 경우 Homebrew를 통해.

수동 CMake 빌드:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

구현은 의도적으로 종속성이 없으며 KiCad 스타일 C++를 따릅니다.
형식 지정 규칙.

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

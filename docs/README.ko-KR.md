# KiCad Backport C++

KiCad Backport C++는 KiCad Backport 다운그레이드 CLI의 독립 C++17 구현입니다. 최신 KiCad S-expression 프로젝트 파일을 이전 KiCad 파일 형식으로 변환하며, 삭제보다 이전 형식에서 표현 가능한 동등한 구문으로 변환하는 것을 우선합니다.

## 명령

```text
kicad-backport convert --target-version <7.0|8.0|9.0|10.0|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

예:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

지원되는 릴리스 별칭은 `7.0`, `8.0`, `9.0`, `10.0`입니다. 특정 파서 기준을 테스트하려면 KiCad 파일 형식 버전 번호를 직접 지정할 수도 있습니다.

## 다운그레이드 정책

- 새 객체나 필드는 가능하면 이전 형식의 동등한 구문으로 변환합니다.
- 이전 형식에서 표현 가능한 표시 정보와 제조 정보는 보존합니다.
- 이전 KiCad가 읽을 수 없거나 이전 형식에 표현 방법이 없는 구문만 제거합니다.
- 제거와 호환성 변환은 warning으로 보고됩니다.

프로젝트 디렉터리 변환 시 KiCad 프로젝트 관련 파일과 일반적인 로컬 3D 모델만 복사합니다. `.history`, 백업, Gerber, BOM, PDF, README 같은 관련 없는 파일은 복사하지 않습니다.

## 빌드

Windows:

```powershell
.\build.ps1
.\build.ps1 -SetupMissingTools
```

Linux/macOS:

```sh
./build.sh
./build.sh --setup
```

결과 파일은 `dist/`에 생성됩니다.

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

Windows에서는 Visual Studio로 Windows amd64/arm64를 빌드하고, WSL 도구 체인이 있으면 Linux amd64/arm64도 빌드합니다. Darwin 바이너리는 macOS와 Apple SDK가 필요합니다.

## 검증

변환 후 대상 KiCad CLI 버전으로 검증하십시오. KiCad 8/9/10은 보통 ERC와 DRC를 실행합니다.

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7은 명령이 적으므로 netlist 및 Gerber export로 파일 로드를 확인합니다.

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

ERC/DRC 위반은 프로젝트의 설계 규칙 결과입니다. KiCad가 로드 또는 파싱 오류를 보고하지 않는다면 형식 변환 실패가 아닙니다.

# KiCad Backport C++

KiCad Backport C++ es una implementación independiente en C++17 de la CLI de degradación KiCad Backport. Convierte archivos de proyecto KiCad recientes a formatos de KiCad anteriores, priorizando una representación equivalente en el formato antiguo antes que eliminar datos.

## Comandos

```text
kicad-backport convert --target-version <7.0|8.0|9.0|10.0|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

Ejemplos:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

Los alias compatibles son `7.0`, `8.0`, `9.0` y `10.0`. También se puede pasar un número de versión de formato KiCad para probar un punto concreto del parser.

## Política de degradación

- Los objetos o campos nuevos se convierten a sintaxis antigua equivalente cuando es posible.
- La información visible y de fabricación se conserva si el formato antiguo puede representarla.
- La sintaxis no compatible solo se elimina cuando KiCad antiguo no puede cargarla o no existe una representación equivalente.
- Cada eliminación o reescritura de compatibilidad se informa como warning.

Al convertir un directorio de proyecto, solo se copian archivos relacionados con KiCad y modelos 3D locales comunes. No se copian `.history`, copias de seguridad, Gerber, BOM, PDF, README ni otros archivos no relacionados.

## Compilación

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

Los binarios se generan en `dist/`.

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

En Windows, `build.ps1` compila Windows amd64/arm64 con Visual Studio y Linux amd64/arm64 mediante WSL si la cadena de herramientas está disponible. Los binarios Darwin requieren macOS y Apple SDK.

## Validación

Después de convertir, valide con la versión correspondiente de KiCad CLI. Para KiCad 8/9/10 normalmente se ejecutan ERC y DRC:

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 tiene menos comandos CLI, por lo que se puede usar la exportación de netlist y Gerber para confirmar que los archivos cargan correctamente:

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

Las infracciones ERC/DRC son resultados de reglas de diseño del proyecto. No indican un fallo de conversión salvo que KiCad informe un error de carga o de parsing.

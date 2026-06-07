# Soporte KiCad C++

Implementación independiente en C++17 de la CLI de degradación de KiCad Backport. la herramienta
convierte archivos de proyecto KiCad S-expression más nuevos a formatos de archivo KiCad más antiguos
al tiempo que se prefiere la sintaxis heredada equivalente a la eliminación.

## Documentación localizada

- [简体中文](docs/README.zh-CN.md)
- [日本語](docs/README.ja-JP.md)
- [한국어](docs/README.ko-KR.md)
- [Francés](docs/README.fr-FR.md)
- [Deutsch](docs/README.de-DE.md)
- [Español](docs/README.es-ES.md)
- [Italiano](docs/README.it-IT.md)

Referencias adicionales:

- [Diferencias de versión del formato de archivo KiCad](docs/kicad-file-format-version-differences.md)
- [Diferencias de versión de formato de archivo localizado](docs/README.md#kicad-file-format-version-differences)

## Comandos

La interfaz de línea de comando refleja la implementación de Go y está destinada a ser
utilizable tanto directamente como desde el complemento de Python:

```text
kicad-backport convert --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport version
```

El convertidor lee archivos KiCad S-expression y aplica una degradación basada en la versión
reglas, escribe una ruta de salida con sufijo de versión y puede copiar todo el proyecto KiCad
directorios antes de normalizar todos los archivos KiCad en la copia.

Ejemplos:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
```

Los alias de versión admitidos son `4.0`, `5.0`, `5.1`, `6.0`, `7.0`, `8.0`,
`9.0`, `10.0` y `10.99`. También se puede pasar una versión sin procesar del formato de
archivo KiCad para probar un corte específico del analizador.

## Estado de soporte

La implementación actual apunta a las familias de archivos KiCad 4 a KiCad 10:

| Objetivo | Estado |
| --- | --- |
| KiCad 10 | Elimina la sintaxis de desarrollo actual/posterior a 10.0, incluido el pad 20260521 `sim_electrical_type` y la celda de tabla 20260603 `knockout`. |
| KiCad 10.99 | Objetivo de desarrollo actual para board/footprint: escribe la version `20260603`; las bibliotecas de simbolos y los esquemas aun usan las versiones objetivo de KiCad 10 (`20251024` / `20260306`). |
| KiCad 9 | Elimina o degrada las características actuales de KiCad 10, como variantes, códigos de barras, retroperforación/postmecanizado, puentes y omisión de códigos de red. |
| KiCad 8 | Elimina tablas KiCad 9+, archivos incrustados, clases de componentes, padstacks, pilas vía, áreas de reglas y formularios arbitrarios de capa de usuario. |
| KiCad 7 | Aplica reescrituras de compatibilidad de analizadores anteriores para formularios UUID/tstamp, campos de huellas de PCB, lágrimas, objetos generados, imágenes y cuadros de texto. |
| KiCad 6 | El soporte básico para la degradación de archivos está prácticamente completo. Los proyectos de prueba convertidos se abrieron manualmente en la aplicación KiCad 6 real para su validación. |
| KiCad 5 | Admite la versión de destino board/footprint `20171130` y la importación/exportación básica de archivos legacy `.sch`, `.lib`, `.dcm` y `.pro`. Los objetos detallados de esquema, primitivas gráficas de símbolos y pines siguen siendo conversiones con pérdida y se informan con advertencias. |
| KiCad 4 | Admite la versión de destino board/footprint `4`, la reescritura de encabezados legacy V4 para esquemas/bibliotecas y sufijos/extensiones de salida V4. Las funciones PCB exclusivas de V5, como pads personalizados, se simplifican cuando es posible. |

## Política de degradación

El convertidor aplica la representación más compatible disponible en el
formato de destino:

- Los nuevos objetos o campos se asignan a una sintaxis equivalente más antigua cuando es posible.
- La información visible/de fabricación se guarda donde el formato antiguo pueda expresarla.
- La sintaxis no compatible se elimina sólo cuando los analizadores KiCad más antiguos no pueden cargarla o
el formato de archivo más antiguo no tiene representación equivalente.
- Cada eliminación o reescritura de compatibilidad se informa como una advertencia.

Por ejemplo, los códigos de red heredados se reconstruyen para formatos de PCB antiguos, booleanos más nuevos
las formas de campo se convierten en átomos de presencia cuando sea necesario, KiCad 7 PCB
Las dimensiones se conservan como gráficos visibles y tablero local del proyecto heredado.
Los archivos de visibilidad se generan para objetivos KiCad 6/7/8.

Al convertir un directorio de proyecto o `.kicad_pro`, la herramienta solo copia
Entradas KiCad editables y archivos de modelos 3D locales comunes. Fabricación generada
Se omiten las salidas, las carpetas de historial/copia de seguridad, los Gerbers, las listas de materiales y los archivos temporales.

## Diseño del proyecto

El código está dividido por responsabilidad para que se puedan agregar versiones posteriores de KiCad con
pequeños cambios localizados:

- `src/kicad_backport.cpp`: flujo CLI, copia/filtrado de proyectos, conversión de archivos.
- `src/kicad_backport_document.cpp`: Detección del tipo de documento KiCad.
- `src/kicad_backport_legacy.cpp`: andamiaje de carga de documentos KiCad heredado.
- `src/kicad_backport_pathmap.cpp`: ayudantes de mapeo de extensión de archivo de destino.
- `src/kicad_backport_report.cpp`: formato de informe JSON.
- `src/kicad_backport_rules.cpp`: puertas de versión y orden de reglas de degradación.
- `src/kicad_backport_rule_rewriters.cpp`: Ayudantes de reescritura del árbol de expresión S.
- `src/kicad_backport_upgrade.cpp`: normalización de sintaxis limitada para archivos fuente más antiguos.
- `src/kicad_backport_versions.cpp`: alias de lanzamiento de KiCad y versiones de formato.
- `src/kicad_backport_util.cpp`: cadena compartida, archivo y ayudantes JSON.
- `src/sexpr.cpp`: analizador/formateador de expresión S mínimo estilo KiCad.
- `src/internal/`: encabezados de implementación privados utilizados solo por archivos fuente.
- `include/kicad_backport/`: encabezados de proyecto públicos utilizados por el ejecutable.

Las reglas de degradación de acción única utilizan un pequeño ayudante `applyWhen()` en lugar de
`std::function`, manteniendo las reglas compactas sin agregar asignaciones de montón.
Las reglas de acción múltiple permanecen agrupadas al ordenar los asuntos.

La estructura de nivel superior es intencionalmente simple:

```text
kicad-backport-cplus/
  include/kicad_backport/   public headers
  src/                      implementation files
  src/internal/             private headers
  scripts/                  cross-build environment setup
  build/                    generated build trees
  dist/                     packaged command-line binaries
```

## Construir

Construir sobre la plataforma actual:

```powershell
.\build.ps1
```

```sh
./build.sh
```

Para detectar e instalar automáticamente la cadena de herramientas práctica más pequeña antes
edificio:

```powershell
.\build.ps1 -SetupMissingTools
```

```sh
./build.sh --setup
```

Ambos scripts prueban los objetivos de versión estándar y copian los resultados exitosos en
`dist/` usando nombres compatibles con complementos:

- `kicad-backport-windows-amd64.exe`
- `kicad-backport-windows-arm64.exe`
- `kicad-backport-linux-amd64`
- `kicad-backport-linux-arm64`
- `kicad-backport-darwin-amd64`
- `kicad-backport-darwin-arm64`

Utilice `.\build.ps1 -Clean` o `./build.sh --clean` para eliminar la compilación anterior
resultados antes de la reconstrucción.

La compilación cruzada de C++ requiere cadenas de herramientas de plataforma. En Windows, `build.ps1`
compila `windows-amd64` y `windows-arm64` con Visual Studio y compila
`linux-amd64`/`linux-arm64` a través de WSL cuando la cadena de herramientas WSL esté disponible.
En Linux, `build.sh` construye Linux nativo y puede construir `linux-arm64` cuando
`aarch64-linux-gnu-g++` está instalado. En macOS, `build.sh` construye Darwin
amd64/arm64 con el SDK de Apple. Los binarios de Darwin deben generarse en macOS.

Para construir un subconjunto:

```powershell
.\build.ps1 -Targets windows-amd64,windows-arm64
```

```sh
TARGETS="linux-amd64 linux-arm64" ./build.sh
```

Configuración del entorno de compilación cruzada:

```powershell
.\scripts\setup-cross.ps1
.\scripts\setup-cross.ps1 -CheckOnly
```

```sh
./scripts/setup-cross.sh
./scripts/setup-cross.sh --check-only
```

Los scripts de configuración instalan automáticamente la cadena de herramientas de construcción práctica más pequeña.
para la plataforma anfitriona. Utilice `-CheckOnly` o `--check-only` para informar solo faltas
herramientas sin instalar nada.

En Windows, el script de instalación instala o prepara CMake, Visual Studio C++
Herramientas de compilación, WSL, Ubuntu y los paquetes WSL mínimos necesarios para Linux
compilaciones amd64/arm64. En Linux, instala CMake, un compilador nativo de C++, Ninja,
y el compilador cruzado de Linux aarch64 donde lo admitía el paquete host
gerente. En macOS, activa las herramientas de línea de comandos de Apple e instala CMake/Ninja
a través de Homebrew cuando esté disponible.

Construcción manual de CMake:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

La implementación está intencionalmente libre de dependencias y sigue el estilo C++ de KiCad.
convenciones de formato.

## Validación

Después de la conversión, valide cada objetivo con la versión de KiCad correspondiente. Para
KiCad 9/8/10 esto generalmente significa ejecutar ERC esquemático y PCB DRC:

```powershell
& 'D:\KiCad\9.0\bin\kicad-cli.exe' sch erc --output erc.rpt project.kicad_sch
& 'D:\KiCad\9.0\bin\kicad-cli.exe' pcb drc --output drc.rpt project.kicad_pcb
```

KiCad 7 CLI tiene un conjunto de comandos más pequeño, así que use netlist y Gerber export para
verifique que se carguen los archivos de PCB y esquemas convertidos:

```powershell
& 'D:\KiCad\7.0\bin\kicad-cli.exe' sch export netlist --output netlist.net project.kicad_sch
& 'D:\KiCad\7.0\bin\kicad-cli.exe' pcb export gerbers --output gerbers project.kicad_pcb
```

KiCad 6 tiene una cobertura de validación CLI limitada. Para archivos PCB, una comprobación rápida del analizador
se puede hacer a través del módulo Python de KiCad 6:

```powershell
& 'D:\KiCad\6.0\bin\python.exe' -c "import pcbnew; pcbnew.LoadBoard(r'E:\tmp\project_V6\project.kicad_pcb'); print('pcb ok')"
```

Para los esquemas y símbolos de KiCad 6, la apertura manual de la GUI sigue siendo la más útil
validación de un extremo a otro. Las muestras de regresión V6 actuales se han comprobado de esta manera.

Las violaciones de ERC/DRC son hallazgos de las reglas de diseño del proyecto. ellos no son
fallas de conversión de formato a menos que KiCad informe un error de carga o análisis.

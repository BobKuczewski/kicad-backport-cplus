# KiCad Backport C++

`kicad-backport` es un conversor de línea de comandos independiente implementado en C++ portable para mover proyectos y archivos KiCad a objetivos de formato más antiguos. Su prioridad es la compatibilidad del parser: cuando una versión antigua de KiCad tiene una representación equivalente, reescribe a esa representación; si no, elimina o aproxima la sintaxis no soportada y lo informa como warning.

La implementación no depende de bibliotecas externas, incluye un pequeño parser/formatter S-expression estilo KiCad y puede usarse directamente o desde un plugin wrapper.

## Documentación

- [Índice de documentación](README.md)
- [Diferencias de formato del conversor](kicad-backport-converter-format-differences.es-ES.md)
- [README en inglés](../README.md)

## Comandos

```text
kicad-backport convert [--quiet] --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> [--report report.json] <input> <output>
kicad-backport inspect <input>
kicad-backport detect-versions [--json] <input>
kicad-backport version
```

Ejemplo:

```powershell
.\dist\kicad-backport-windows-amd64.exe convert --target-version 9.0 E:\tmp\project E:\tmp\project_V9
.\dist\kicad-backport-windows-amd64.exe inspect E:\tmp\project
.\dist\kicad-backport-windows-amd64.exe detect-versions --json E:\tmp\project
```

`convert` acepta un documento KiCad o un directorio de proyecto. Un `.kicad_pro` convierte el directorio de proyecto que lo contiene. La salida recibe un sufijo de destino como `project_V9`.

`inspect` informa tipo y versión detectados. `detect-versions` hace un escaneo ligero y puede emitir JSON.

## Objetivos soportados

| Objetivo | Comportamiento actual |
| --- | --- |
| KiCad 10.99 | Alias de desarrollo. Board/footprint escriben `20260603`; schematic/symbol-library mantienen los anchors KiCad 10. |
| KiCad 10 | Elimina o reescribe sintaxis de desarrollo fuera de los anchors `10.0`. |
| KiCad 9 | Elimina o baja variants, barcodes, backdrill/post-machining, jumper pad metadata y referencias board solo por net name. |
| KiCad 8 | Elimina o reescribe KiCad 9+ tables, embedded files/fonts, component classes, padstacks, via stacks, rule/placement areas, user-layer type qualifiers y font face fields. |
| KiCad 7 | Aplica compatibilidad para UUID/tstamp, PCB footprint fields, teardrops, generated objects, images, text boxes y stroke/dimension syntax. |
| KiCad 6 | Objetivo de la primera familia moderna schematic/symbol/project con estructuras de compatibilidad necesarias. |
| KiCad 5.0/5.1 | Board/footprint usan `20171130`; schematic, symbol-library y project escriben legacy `.sch`, `.lib/.dcm`, `.pro`. |
| KiCad 4 | Board/footprint usan `4`; se reescriben headers legacy V4 y se simplifican constructs PCB KiCad 5+ cuando es posible. |

Véanse las [diferencias de formato](kicad-backport-converter-format-differences.es-ES.md).

## Política de conversión

- Preserva la intención existente cuando el formato objetivo puede representarla.
- Reescribe a sintaxis antigua equivalente cuando existe.
- Elimina solo lo que los parsers antiguos no pueden leer o no tiene equivalente.
- Los cambios con pérdida y eliminaciones se emiten como warning.
- Los upgrades no crean funciones KiCad ausentes en la fuente.

La frontera KiCad 5/6 cambia familias de archivos: `.sch -> .kicad_sch`, `.lib/.dcm -> .kicad_sym`, `.pro -> .kicad_pro`, y al revés `.kicad_sch -> .sch`, `.kicad_sym -> .lib + .dcm`, `.kicad_pro -> .pro`.

## Conversión de proyecto

Para directorios de proyecto, el conversor copia solo entradas KiCad editables y modelos 3D locales comunes, y luego convierte los documentos copiados. Omite outputs de fabricación, backups, history, Gerbers, BOM, directorios plot/export y temporales.

Las reparaciones incluyen `sym-lib-table` / `fp-lib-table`, `.kicad_prl` para KiCad 6/7/8, al degradar board/footprint nuevos la extracción de recursos 3D embebidos a `3D/` y la reescritura de URI de modelo `kicad-embed://...` a `${KIPRJMOD}/3D/...`, símbolos locales embebidos en `lib_symbols` y reconstrucción de hierarchy instances.

## Build

```powershell
.\build.ps1
```

```sh
./build.sh
```

Opciones POSIX útiles:

```sh
./build.sh --clean
./build.sh --config Release
./build.sh --compiler g++-8
./build.sh --static-runtime off
./build.sh --zstd off
```

PowerShell también acepta `-Zstd auto|on|off`. El modo predeterminado `auto` compila el descompresor zstd vendored cuando existe `src/third_party/zstd`. Use `off` si la toolchain antigua no puede compilar esas fuentes C; en ese caso no se pueden extraer recursos 3D embebidos y el conversor avisa cuando debe eliminar referencias embedded model no soportadas.

Los scripts leen `kicad_backport_sources.txt`, compilan con `g++` o `clang++` y copian el ejecutable a `dist/`. Si hace falta, usan fallback de `-std=c++17` a `-std=c++1z`.

## Validación

Después de convertir, abrir con la versión KiCad objetivo y ejecutar ERC/DRC cuando corresponda. Revisar los warnings antes de uso en producción.

## Agradecimientos

Un agradecimiento especial a Hubert por la ayuda prestada durante el desarrollo de este proyecto.

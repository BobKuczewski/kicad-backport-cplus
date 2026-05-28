#!/usr/bin/env sh
set -eu

CONFIG="${CONFIG:-Release}"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
BUILD_ROOT="$ROOT/build"
DIST_DIR="$ROOT/dist"
CLEAN=0
SETUP_MISSING_TOOLS=0

TARGETS="${TARGETS:-windows-amd64 windows-arm64 linux-amd64 linux-arm64 darwin-amd64 darwin-arm64}"

for arg in "$@"; do
    case "$arg" in
        --clean)
            CLEAN=1
            ;;
        --setup)
            SETUP_MISSING_TOOLS=1
            ;;
        *)
            echo "Unsupported argument: $arg" >&2
            exit 1
            ;;
    esac
done

host_os() {
    case "$(uname -s)" in
        Darwin*) echo "darwin" ;;
        Linux*) echo "linux" ;;
        MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
        *) uname -s | tr '[:upper:]' '[:lower:]' ;;
    esac
}

host_arch() {
    case "$(uname -m)" in
        x86_64|amd64) echo "amd64" ;;
        arm64|aarch64) echo "arm64" ;;
        i386|i686) echo "386" ;;
        *) uname -m ;;
    esac
}

skip_target() {
    echo "Skipped $1: $2"
}

build_cmake_target() {
    target="$1"
    build_dir="$BUILD_ROOT/$target"
    output_name="kicad-backport-$target"
    generator_args=""

    if [ "${CLEAN:-0}" = "1" ]; then
        rm -rf "$build_dir" "$DIST_DIR/$output_name"
    fi

    if command -v ninja >/dev/null 2>&1; then
        generator_args="-G Ninja"
    fi

    shift
    # shellcheck disable=SC2086
    cmake -S "$ROOT" -B "$build_dir" $generator_args -DCMAKE_BUILD_TYPE="$CONFIG" "$@"
    cmake --build "$build_dir" --config "$CONFIG"

    if [ -f "$build_dir/kicad-backport" ]; then
        built_exe="$build_dir/kicad-backport"
    elif [ -f "$build_dir/$CONFIG/kicad-backport" ]; then
        built_exe="$build_dir/$CONFIG/kicad-backport"
    else
        echo "Cannot find built executable under $build_dir" >&2
        exit 1
    fi

    mkdir -p "$DIST_DIR"
    cp "$built_exe" "$DIST_DIR/$output_name"
    chmod +x "$DIST_DIR/$output_name"
    echo "Built $DIST_DIR/$output_name"
}

if [ "$SETUP_MISSING_TOOLS" = "1" ]; then
    "$ROOT/scripts/setup-cross.sh"
fi

HOST_OS="$(host_os)"
HOST_ARCH="$(host_arch)"

for target in $TARGETS; do
    case "$target" in
        windows-amd64|windows-arm64)
            skip_target "$target" "Windows C++ binaries require Visual Studio CMake generator. Run .\\build.ps1 on Windows."
            ;;
        linux-amd64)
            if [ "$HOST_OS" = "linux" ] && [ "$HOST_ARCH" = "amd64" ]; then
                build_cmake_target "$target"
            else
                skip_target "$target" "Requires a linux-amd64 host."
            fi
            ;;
        linux-arm64)
            if [ "$HOST_OS" = "linux" ] && [ "$HOST_ARCH" = "arm64" ]; then
                build_cmake_target "$target"
            elif command -v aarch64-linux-gnu-g++ >/dev/null 2>&1; then
                build_cmake_target "$target" \
                    -DCMAKE_SYSTEM_NAME=Linux \
                    -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
                    -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
            else
                skip_target "$target" "Requires a linux-arm64 host or aarch64-linux-gnu-g++ cross compiler."
            fi
            ;;
        darwin-amd64)
            if [ "$HOST_OS" = "darwin" ]; then
                build_cmake_target "$target" -DCMAKE_OSX_ARCHITECTURES=x86_64
            else
                skip_target "$target" "Requires macOS and the Apple SDK."
            fi
            ;;
        darwin-arm64)
            if [ "$HOST_OS" = "darwin" ]; then
                build_cmake_target "$target" -DCMAKE_OSX_ARCHITECTURES=arm64
            else
                skip_target "$target" "Requires macOS and the Apple SDK."
            fi
            ;;
        *)
            echo "Unsupported target: $target" >&2
            exit 1
            ;;
    esac
done

echo "Done. Generated files are in $DIST_DIR"

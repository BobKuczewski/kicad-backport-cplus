#!/usr/bin/env sh
set -eu

CONFIG="${CONFIG:-Release}"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
BUILD_ROOT="$ROOT/build"
DIST_DIR="$ROOT/dist"
CLEAN=0
GENERATOR="${GENERATOR:-}"
JOBS="${JOBS:-}"

usage() {
    cat <<'EOF'
usage: ./build-linux.sh [--clean] [--config <Debug|Release|RelWithDebInfo|MinSizeRel>] [--generator <cmake-generator>] [--jobs <n>]

Builds the current Linux host architecture with native CMake and the host C++ toolchain.
Outputs dist/kicad-backport-linux-<amd64|arm64>.
EOF
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --clean)
            CLEAN=1
            shift
            ;;
        --config)
            if [ "$#" -lt 2 ]; then
                echo "Missing value for --config" >&2
                exit 2
            fi
            CONFIG="$2"
            shift 2
            ;;
        --generator)
            if [ "$#" -lt 2 ]; then
                echo "Missing value for --generator" >&2
                exit 2
            fi
            GENERATOR="$2"
            shift 2
            ;;
        --jobs|-j)
            if [ "$#" -lt 2 ]; then
                echo "Missing value for --jobs" >&2
                exit 2
            fi
            JOBS="$2"
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "Unsupported argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [ "$(uname -s)" != "Linux" ]; then
    echo "build-linux.sh must be run on Linux." >&2
    exit 2
fi

case "$(uname -m)" in
    x86_64|amd64) ARCH="amd64" ;;
    aarch64|arm64) ARCH="arm64" ;;
    *)
        echo "Unsupported Linux architecture: $(uname -m)" >&2
        exit 2
        ;;
esac

if ! command -v cmake >/dev/null 2>&1; then
    echo "cmake is required. Install CMake or run ./scripts/setup-cross.sh on Linux." >&2
    exit 2
fi

if [ -z "$GENERATOR" ] && command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
fi

TARGET="linux-$ARCH"
BUILD_DIR="$BUILD_ROOT/$TARGET-native"
OUTPUT_PATH="$DIST_DIR/kicad-backport-$TARGET"

if [ "$CLEAN" = "1" ]; then
    rm -rf "$BUILD_DIR" "$OUTPUT_PATH"
fi

if [ -n "$GENERATOR" ]; then
    cmake -S "$ROOT" -B "$BUILD_DIR" -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$CONFIG"
else
    cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIG"
fi

if [ -n "$JOBS" ]; then
    cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel "$JOBS"
else
    cmake --build "$BUILD_DIR" --config "$CONFIG"
fi

if [ -f "$BUILD_DIR/kicad-backport" ]; then
    BUILT_EXE="$BUILD_DIR/kicad-backport"
elif [ -f "$BUILD_DIR/$CONFIG/kicad-backport" ]; then
    BUILT_EXE="$BUILD_DIR/$CONFIG/kicad-backport"
else
    echo "Cannot find built executable under $BUILD_DIR" >&2
    exit 1
fi

mkdir -p "$DIST_DIR"
cp "$BUILT_EXE" "$OUTPUT_PATH"
chmod +x "$OUTPUT_PATH"
echo "Built $OUTPUT_PATH"

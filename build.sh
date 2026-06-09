#!/usr/bin/env sh
set -eu

CONFIG="${CONFIG:-Release}"
TARGET="${TARGET:-native}"
STATIC_RUNTIME="${STATIC_RUNTIME:-auto}"
GENERATOR="${GENERATOR:-}"
JOBS="${JOBS:-}"
CXX="${CXX:-auto}"
DIRECT_BUILD=0

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
BUILD_ROOT="$ROOT/build"
DIST_DIR="$ROOT/dist"
SOURCE_LIST="$ROOT/cmake/kicad_backport_sources.txt"
CLEAN=0
SETUP_MISSING_TOOLS=0

usage() {
    cat <<'EOF'
usage: ./build.sh [options]

Builds the current POSIX host natively. Cross-compilation is intentionally not
handled here; Windows cross builds are handled by build.ps1 through WSL.

Options:
  --clean                         remove the selected build output first
  --setup                         install the native compiler/CMake when possible
  --config <name>                 Debug, Release, RelWithDebInfo, or MinSizeRel
  --target <native|linux-amd64|linux-arm64|linux-armhf|darwin-amd64|darwin-arm64>
  --generator <cmake-generator>   use a specific CMake generator
  --jobs <n>, -j <n>              pass parallelism to CMake
  --compiler <command>            direct compiler fallback, default: g++/clang++
  --direct, --no-cmake            force direct compiler build
  --static-runtime <auto|on|off>  direct g++ runtime mode; CMake uses on/off

Environment mirrors the options: CONFIG, TARGET, GENERATOR, JOBS,
STATIC_RUNTIME, CXX, CXXFLAGS, and LDFLAGS.
EOF
}

need_value() {
    if [ "$#" -lt 2 ]; then
        echo "Missing value for $1" >&2
        exit 2
    fi
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --clean)
            CLEAN=1
            shift
            ;;
        --setup)
            SETUP_MISSING_TOOLS=1
            shift
            ;;
        --config)
            need_value "$@"
            CONFIG="$2"
            shift 2
            ;;
        --target|--targets)
            need_value "$@"
            TARGET="$2"
            shift 2
            ;;
        --generator)
            need_value "$@"
            GENERATOR="$2"
            shift 2
            ;;
        --jobs|-j)
            need_value "$@"
            JOBS="$2"
            shift 2
            ;;
        --compiler)
            need_value "$@"
            CXX="$2"
            shift 2
            ;;
        --direct|--no-cmake)
            DIRECT_BUILD=1
            shift
            ;;
        --static-runtime)
            need_value "$@"
            STATIC_RUNTIME="$2"
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

case "$CONFIG" in
    Debug|Release|RelWithDebInfo|MinSizeRel) ;;
    *) echo "Unsupported config: $CONFIG" >&2; exit 2 ;;
esac

case "$STATIC_RUNTIME" in
    auto|on|off) ;;
    *) echo "Unsupported static runtime mode: $STATIC_RUNTIME" >&2; exit 2 ;;
esac

has_cmd() {
    command -v "$1" >/dev/null 2>&1
}

host_os() {
    case "$(uname -s)" in
        Darwin*) echo "darwin" ;;
        Linux*) echo "linux" ;;
        *) uname -s | tr '[:upper:]' '[:lower:]' ;;
    esac
}

host_arch() {
    case "$(uname -m)" in
        x86_64|amd64) echo "amd64" ;;
        arm64|aarch64) echo "arm64" ;;
        armv7l|armv7*|armv6l|armv6*) echo "armhf" ;;
        i386|i686) echo "386" ;;
        *) uname -m ;;
    esac
}

install_native_tools() {
    os="$(host_os)"

    case "$os" in
        linux)
            if has_cmd apt-get; then
                if [ "$(id -u)" = "0" ]; then
                    DEBIAN_FRONTEND=noninteractive apt-get update
                    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends g++ cmake ninja-build
                elif has_cmd sudo; then
                    sudo env DEBIAN_FRONTEND=noninteractive apt-get update
                    sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends g++ cmake ninja-build
                else
                    echo "sudo is not available; install g++ manually." >&2
                fi
            elif has_cmd dnf; then
                sudo dnf install -y gcc-c++ cmake ninja-build
            elif has_cmd yum; then
                sudo yum install -y gcc-c++ cmake
            elif has_cmd zypper; then
                sudo zypper --non-interactive install gcc-c++ cmake ninja
            elif has_cmd pacman; then
                sudo pacman -S --needed gcc cmake ninja
            elif has_cmd apk; then
                sudo apk add g++ cmake ninja
            else
                echo "No supported package manager found. Install g++ manually." >&2
            fi
            ;;
        darwin)
            if ! xcode-select -p >/dev/null 2>&1; then
                xcode-select --install || true
                echo "Complete the Apple Command Line Tools installer, then rerun this script."
            fi
            if ( ! has_cmd cmake || ! has_cmd ninja ) && has_cmd brew; then
                brew install cmake ninja
            fi
            ;;
        *)
            echo "Unsupported setup host: $os" >&2
            ;;
    esac
}

cmake_is_usable() {
    has_cmd cmake || return 1

    version="$(cmake --version | sed -n '1s/.*version //p')"
    major="$(printf '%s' "$version" | cut -d. -f1)"
    minor="$(printf '%s' "$version" | cut -d. -f2)"

    case "$major" in ''|*[!0-9]*) return 1 ;; esac
    case "$minor" in ''|*[!0-9]*) return 1 ;; esac

    [ "$major" -gt 3 ] || { [ "$major" -eq 3 ] && [ "$minor" -ge 10 ]; }
}

host_target() {
    printf '%s-%s\n' "$(host_os)" "$(host_arch)"
}

resolve_target() {
    if [ "$TARGET" = "native" ]; then
        host_target
    else
        printf '%s\n' "$TARGET"
    fi
}

resolve_compiler() {
    if [ "$CXX" != "auto" ] && [ -n "$CXX" ]; then
        printf '%s\n' "$CXX"
        return
    fi

    case "$(host_os)" in
        darwin)
            if has_cmd xcrun && xcrun --find clang++ >/dev/null 2>&1; then
                xcrun --find clang++
            else
                printf '%s\n' clang++
            fi
            ;;
        *)
            printf '%s\n' g++
            ;;
    esac
}

cmake_static_runtime_arg() {
    case "$STATIC_RUNTIME" in
        on|auto) printf '%s\n' "-DKICAD_BACKPORT_STATIC_RUNTIME=ON" ;;
        off) printf '%s\n' "-DKICAD_BACKPORT_STATIC_RUNTIME=OFF" ;;
    esac
}

validate_native_target() {
    target="$1"
    actual="$(host_target)"

    if [ "$target" != "$actual" ]; then
        if [ "${KICAD_BACKPORT_ALLOW_CROSS:-0}" = "1" ]; then
            return 0
        fi

        echo "build.sh only supports native POSIX builds. Requested $target, host is $actual." >&2
        echo "Use build.ps1 on Windows for temporary cross-build support." >&2
        exit 2
    fi
}

build_with_cmake() {
    target="$1"
    build_dir="$BUILD_ROOT/$target"
    output_path="$DIST_DIR/kicad-backport-$target"
    static_arg="$(cmake_static_runtime_arg)"

    if [ "$CLEAN" = "1" ]; then
        rm -rf "$build_dir" "$output_path"
    fi

    if [ -n "$GENERATOR" ]; then
        # shellcheck disable=SC2086
        cmake -S "$ROOT" -B "$build_dir" -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$CONFIG" $static_arg
    elif has_cmd ninja; then
        # shellcheck disable=SC2086
        cmake -S "$ROOT" -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE="$CONFIG" $static_arg
    else
        # shellcheck disable=SC2086
        cmake -S "$ROOT" -B "$build_dir" -DCMAKE_BUILD_TYPE="$CONFIG" $static_arg
    fi

    if [ -n "$JOBS" ]; then
        cmake --build "$build_dir" --config "$CONFIG" --parallel "$JOBS"
    else
        cmake --build "$build_dir" --config "$CONFIG"
    fi

    if [ -f "$build_dir/kicad-backport" ]; then
        built_exe="$build_dir/kicad-backport"
    elif [ -f "$build_dir/$CONFIG/kicad-backport" ]; then
        built_exe="$build_dir/$CONFIG/kicad-backport"
    else
        echo "Cannot find built executable under $build_dir" >&2
        exit 1
    fi

    mkdir -p "$DIST_DIR"
    cp "$built_exe" "$output_path"
    chmod +x "$output_path"
    echo "Built $output_path"
}

config_cxxflags() {
    case "$CONFIG" in
        Debug) printf '%s\n' "-O0 -g" ;;
        Release) printf '%s\n' "-O3 -DNDEBUG" ;;
        RelWithDebInfo) printf '%s\n' "-O2 -g -DNDEBUG" ;;
        MinSizeRel) printf '%s\n' "-Os -DNDEBUG" ;;
    esac
}

select_cxx_standard_flag() {
    compiler="$1"
    probe_dir="${TMPDIR:-/tmp}/kicad-backport-cxx-std-$$"
    mkdir -p "$probe_dir"
    printf '%s\n' 'int main() { return 0; }' > "$probe_dir/probe.cpp"

    if "$compiler" -std=c++17 "$probe_dir/probe.cpp" -o "$probe_dir/probe" >/dev/null 2>&1; then
        rm -rf "$probe_dir"
        printf '%s\n' "-std=c++17"
        return
    fi

    if "$compiler" -std=c++1z "$probe_dir/probe.cpp" -o "$probe_dir/probe" >/dev/null 2>&1; then
        rm -rf "$probe_dir"
        printf '%s\n' "-std=c++1z"
        return
    fi

    rm -rf "$probe_dir"
    echo "$compiler does not accept -std=c++17 or -std=c++1z." >&2
    exit 2
}

build_direct() {
    target="$1"
    compiler="$(resolve_compiler)"
    build_dir="$BUILD_ROOT/$target-direct"
    object_dir="$build_dir/obj"
    output_path="$DIST_DIR/kicad-backport-$target"

    if ! has_cmd "$compiler"; then
        echo "$compiler is required for direct native builds." >&2
        exit 2
    fi

    if [ "$CLEAN" = "1" ]; then
        rm -rf "$build_dir" "$output_path"
    fi

    mkdir -p "$object_dir" "$DIST_DIR"

    platform_cxxflags=""
    platform_ldflags=""
    static_ldflags=""

    case "$(host_os)" in
        linux)
            platform_cxxflags="-pthread"
            platform_ldflags="-pthread"
            static_ldflags="-static-libstdc++ -static-libgcc"
            ;;
    esac

    std_flag="$(select_cxx_standard_flag "$compiler")"
    common_cxxflags="$std_flag -Wall -Wextra -Wpedantic -I$ROOT/include -I$ROOT/src"
    cfg_cxxflags="$(config_cxxflags)"

    echo "Compiler: $("$compiler" --version | sed -n '1p')"
    echo "Target: $target"
    if "$compiler" -dumpmachine >/dev/null 2>&1; then
        echo "Compiler target: $("$compiler" -dumpmachine)"
    fi
    echo "Static runtime: $STATIC_RUNTIME"

    objects=""
    while IFS= read -r source || [ -n "$source" ]; do
        source="$(printf '%s' "$source" | tr -d '\r')"
        case "$source" in
            ""|\#*) continue ;;
        esac

        object="$object_dir/${source%.cpp}.o"
        mkdir -p "$(dirname "$object")"
        # shellcheck disable=SC2086
        "$compiler" $common_cxxflags $platform_cxxflags $cfg_cxxflags ${CXXFLAGS:-} -c "$ROOT/$source" -o "$object"
        objects="$objects $object"
    done < "$SOURCE_LIST"

    link_output() {
        # shellcheck disable=SC2086
        "$compiler" $objects ${LDFLAGS:-} $platform_ldflags "$@" -o "$output_path"
    }

    try_link() {
        description="$1"
        shift
        echo "Linking: $description"
        link_output "$@"
    }

    if [ "$STATIC_RUNTIME" = "on" ]; then
        if ! try_link "static libstdc++/libgcc" $static_ldflags; then
            try_link "static libstdc++/libgcc + stdc++fs" $static_ldflags -lstdc++fs
        fi
    elif [ "$STATIC_RUNTIME" = "off" ]; then
        if ! try_link "dynamic runtime"; then
            try_link "dynamic runtime + stdc++fs" -lstdc++fs
        fi
    else
        if ! try_link "static libstdc++/libgcc" $static_ldflags; then
            if ! try_link "static libstdc++/libgcc + stdc++fs" $static_ldflags -lstdc++fs; then
                if ! try_link "dynamic runtime"; then
                    try_link "dynamic runtime + stdc++fs" -lstdc++fs
                fi
            fi
        fi
    fi

    chmod +x "$output_path"
    echo "Built $output_path"
}

if [ "$SETUP_MISSING_TOOLS" = "1" ]; then
    install_native_tools
fi

TARGET="$(resolve_target)"
validate_native_target "$TARGET"

if [ "$TARGET" != "$(host_target)" ]; then
    build_direct "$TARGET"
elif [ "$DIRECT_BUILD" = "1" ]; then
    build_direct "$TARGET"
elif cmake_is_usable; then
    build_with_cmake "$TARGET"
else
    build_direct "$TARGET"
fi

echo "Done. Generated files are in $DIST_DIR"

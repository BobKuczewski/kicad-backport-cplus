#!/usr/bin/env sh
set -eu

CONFIG="${CONFIG:-Release}"
TARGET="${TARGET:-native}"
CXX="${CXX:-auto}"
STATIC_RUNTIME="${STATIC_RUNTIME:-auto}"
CLEAN=0

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
BUILD_ROOT="$ROOT/build"
DIST_DIR="$ROOT/dist"
SOURCE_LIST="$ROOT/kicad_backport_sources.txt"

usage() {
    cat <<'EOF'
usage: ./build.sh [options]

Direct native build for Linux, Raspberry Pi, and macOS. It compiles each source
file with g++/clang++ and links the executable; No project generator is used.

Options:
  --clean                         remove the direct build output first
  --config <name>                 Debug, Release, RelWithDebInfo, or MinSizeRel
  --target <native|linux-amd64|linux-arm64|linux-armhf|darwin-amd64|darwin-arm64>
  --compiler <command>            compiler command, default: g++ on Linux, clang++ on macOS
  --static-runtime <auto|on|off>  link libstdc++/libgcc statically when supported
  --help, -h                      show this help

Environment mirrors the options: CONFIG, TARGET, CXX, STATIC_RUNTIME, CXXFLAGS,
and LDFLAGS.
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
        --compiler)
            need_value "$@"
            CXX="$2"
            shift 2
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
        *) uname -m ;;
    esac
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

    if [ "$(host_os)" = "darwin" ]; then
        if has_cmd xcrun && xcrun --find clang++ >/dev/null 2>&1; then
            xcrun --find clang++
        else
            printf '%s\n' clang++
        fi
    else
        printf '%s\n' g++
    fi
}

config_flags() {
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
    cat > "$probe_dir/probe.cpp" <<'EOF'
#include <memory>
#include <string>
#include <vector>
int main() { std::unique_ptr<int> p(new int(1)); std::vector<std::string> v; v.push_back("x"); return *p == 1 && !v.empty() ? 0 : 1; }
EOF

    if "$compiler" -std=c++17 -c "$probe_dir/probe.cpp" -o "$probe_dir/probe.o" >/dev/null 2>&1; then
        rm -rf "$probe_dir"
        printf '%s\n' "-std=c++17"
        return
    fi

    if "$compiler" -std=c++1z -c "$probe_dir/probe.cpp" -o "$probe_dir/probe.o" >/dev/null 2>&1; then
        rm -rf "$probe_dir"
        printf '%s\n' "-std=c++1z"
        return
    fi

    rm -rf "$probe_dir"
    echo "$compiler cannot compile the required C++ mode probe." >&2
    exit 2
}

supports_compile_flag() {
    compiler="$1"
    std_flag="$2"
    flag="$3"
    probe_dir="${TMPDIR:-/tmp}/kicad-backport-cxx-flag-$$"
    mkdir -p "$probe_dir"
    printf '%s\n' 'int main() { return 0; }' > "$probe_dir/probe.cpp"

    if "$compiler" $std_flag "$flag" -Werror -c "$probe_dir/probe.cpp" -o "$probe_dir/probe.o" >/dev/null 2>&1; then
        rm -rf "$probe_dir"
        return 0
    fi

    rm -rf "$probe_dir"
    return 1
}

supports_link_flag() {
    compiler="$1"
    std_flag="$2"
    flag="$3"
    probe_dir="${TMPDIR:-/tmp}/kicad-backport-ld-flag-$$"
    mkdir -p "$probe_dir"
    printf '%s\n' 'int main() { return 0; }' > "$probe_dir/probe.cpp"

    if "$compiler" $std_flag -c "$probe_dir/probe.cpp" -o "$probe_dir/probe.o" >/dev/null 2>&1 \
        && "$compiler" "$probe_dir/probe.o" "$flag" -o "$probe_dir/probe" >/dev/null 2>&1; then
        rm -rf "$probe_dir"
        return 0
    fi

    rm -rf "$probe_dir"
    return 1
}

size_compile_flags() {
    compiler="$1"
    std_flag="$2"

    if [ "$CONFIG" = "Debug" ]; then
        return
    fi

    flags=""
    for flag in -ffunction-sections -fdata-sections; do
        if supports_compile_flag "$compiler" "$std_flag" "$flag"; then
            flags="$flags $flag"
        fi
    done

    printf '%s\n' "$flags"
}

size_link_flags() {
    compiler="$1"
    std_flag="$2"

    if [ "$CONFIG" = "Debug" ]; then
        return
    fi

    flags=""
    if [ "$(host_os)" = "darwin" ]; then
        gc_flag="-Wl,-dead_strip"
    else
        gc_flag="-Wl,--gc-sections"
    fi

    if supports_link_flag "$compiler" "$std_flag" "$gc_flag"; then
        flags="$flags $gc_flag"
    fi

    if { [ "$CONFIG" = "Release" ] || [ "$CONFIG" = "MinSizeRel" ]; } \
        && supports_link_flag "$compiler" "$std_flag" "-Wl,-s"; then
        flags="$flags -Wl,-s"
    fi

    printf '%s\n' "$flags"
}

quote_target_check() {
    target="$1"
    actual="$(host_target)"

    if [ "$target" != "$actual" ] && [ "${KICAD_BACKPORT_ALLOW_CROSS:-0}" != "1" ]; then
        echo "Requested $target, but this host is $actual." >&2
        echo "Run this script on the target system, set CXX for a matching compiler, or set KICAD_BACKPORT_ALLOW_CROSS=1." >&2
        exit 2
    fi
}

compile_direct() {
    target="$1"
    compiler="$(resolve_compiler)"
    build_dir="$BUILD_ROOT/$target-direct"
    object_dir="$build_dir/obj"
    output_path="$DIST_DIR/kicad-backport-$target"

    if ! has_cmd "$compiler"; then
        echo "$compiler is required." >&2
        exit 2
    fi

    if [ ! -f "$SOURCE_LIST" ]; then
        echo "Missing source list: $SOURCE_LIST" >&2
        exit 2
    fi

    if [ "$CLEAN" = "1" ]; then
        rm -rf "$build_dir" "$output_path"
    fi

    mkdir -p "$object_dir" "$DIST_DIR"

    std_flag="$(select_cxx_standard_flag "$compiler")"
    size_cxxflags="$(size_compile_flags "$compiler" "$std_flag")"
    size_ldflags="$(size_link_flags "$compiler" "$std_flag")"
    cxxflags="$std_flag -Wall -Wextra -Wpedantic -I$ROOT/include -I$ROOT/src $(config_flags) $size_cxxflags"
    ldflags="$size_ldflags"
    static_flags=""

    if [ "$(host_os)" = "linux" ]; then
        static_flags="-static-libstdc++ -static-libgcc"
    fi

    echo "Compiler: $("$compiler" --version | sed -n '1p')"
    echo "Target: $target"

    objects=""
    while IFS= read -r source || [ -n "$source" ]; do
        source="$(printf '%s' "$source" | tr -d '\r')"
        case "$source" in
            ""|\#*) continue ;;
        esac

        object="$object_dir/${source%.cpp}.o"
        mkdir -p "$(dirname "$object")"
        # shellcheck disable=SC2086
        "$compiler" $cxxflags ${CXXFLAGS:-} -c "$ROOT/$source" -o "$object"
        objects="$objects $object"
    done < "$SOURCE_LIST"

    link_once() {
        # shellcheck disable=SC2086
        "$compiler" $objects ${LDFLAGS:-} $ldflags "$@" -o "$output_path"
    }

    if [ "$STATIC_RUNTIME" = "on" ]; then
        link_once $static_flags
    elif [ "$STATIC_RUNTIME" = "off" ]; then
        link_once
    else
        link_once $static_flags || link_once
    fi

    chmod +x "$output_path"
    echo "Built $output_path"
}

TARGET="$(resolve_target)"
quote_target_check "$TARGET"
compile_direct "$TARGET"

echo "Done. Generated files are in $DIST_DIR"

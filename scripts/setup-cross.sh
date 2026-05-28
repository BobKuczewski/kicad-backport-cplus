#!/usr/bin/env sh
set -eu

CHECK_ONLY=0

if [ "${1:-}" = "--check-only" ]; then
    CHECK_ONLY=1
fi

status() {
    if [ "$2" = "1" ]; then
        printf '[OK] %s: %s\n' "$1" "$3"
    else
        printf '[MISSING] %s: %s\n' "$1" "$3"
    fi
}

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

install_linux_deps() {
    if has_cmd apt-get; then
        sudo env DEBIAN_FRONTEND=noninteractive apt-get update
        sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
            cmake g++ ninja-build gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
    elif has_cmd dnf; then
        sudo dnf install -y cmake gcc-c++ ninja-build gcc-aarch64-linux-gnu gcc-c++-aarch64-linux-gnu
    elif has_cmd pacman; then
        sudo pacman -S --needed cmake gcc ninja aarch64-linux-gnu-gcc
    else
        echo "No supported package manager found. Install cmake, g++, ninja, and an aarch64 Linux cross compiler manually." >&2
        return 1
    fi
}

install_macos_deps() {
    if ! xcode-select -p >/dev/null 2>&1; then
        xcode-select --install || true
        echo "Complete the Xcode Command Line Tools installer, then rerun this script."
    fi

    if ! has_cmd cmake || ! has_cmd ninja; then
        if has_cmd brew; then
            brew install cmake ninja
        else
            echo "Install Homebrew or install CMake/Ninja manually." >&2
        fi
    fi
}

OS="$(host_os)"

echo "KiCad Backport C++ minimal cross-build environment setup"
if [ "$CHECK_ONLY" = "1" ]; then
    echo "Mode: check only"
else
    echo "Mode: setup missing tools"
fi
echo ""

if has_cmd cmake; then
    status "CMake" 1 "$(cmake --version | sed -n '1p')"
else
    status "CMake" 0 "Required for all builds."
fi

if has_cmd ninja; then
    status "Ninja" 1 "Available; optional but recommended."
else
    status "Ninja" 0 "Optional; installed as part of minimal setup when possible."
fi

case "$OS" in
    linux)
        if has_cmd g++; then
            status "g++" 1 "Required for linux-amd64 native builds."
        else
            status "g++" 0 "Required for linux-amd64 native builds."
        fi

        if has_cmd aarch64-linux-gnu-g++; then
            status "aarch64-linux-gnu-g++" 1 "Required for linux-arm64 cross-builds."
        else
            status "aarch64-linux-gnu-g++" 0 "Required for linux-arm64 cross-builds."
        fi

        if [ "$CHECK_ONLY" = "0" ]; then
            install_linux_deps
        fi
        ;;
    darwin)
        if xcode-select -p >/dev/null 2>&1; then
            status "Xcode Command Line Tools" 1 "Required for darwin-amd64/darwin-arm64 builds."
        else
            status "Xcode Command Line Tools" 0 "Required for Darwin builds."
        fi

        if [ "$CHECK_ONLY" = "0" ]; then
            install_macos_deps
        fi
        ;;
    *)
        echo "Unsupported POSIX host for automatic setup: $OS" >&2
        ;;
esac

echo ""
echo "Summary:"
echo "- Linux amd64/arm64: this script installs the native compiler and aarch64 cross compiler where supported."
echo "- Darwin amd64/arm64: this script triggers Apple Command Line Tools and installs CMake/Ninja via Homebrew when available."
echo "- Windows amd64/arm64: run scripts/setup-cross.ps1 on Windows."
echo ""
echo "Next:"
echo "  ./build.sh"

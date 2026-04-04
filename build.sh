#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
BACKEND_WAYLAND=ON
BACKEND_DRM=ON
CLEAN=0
JOBS=$(nproc 2>/dev/null || echo 4)

usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --wayland-only   Disable DRM backend (no libinput needed)"
    echo "  --drm-only       Disable Wayland backend"
    echo "  --clean          Remove build directory before building"
    echo "  --install-deps   Install build dependencies (requires sudo)"
    echo "  -j N             Parallel jobs (default: $(nproc 2>/dev/null || echo 4))"
    echo "  -h, --help       Show this help"
}

install_deps() {
    echo "--- Installing build dependencies ---"
    sudo apt install -y \
        cmake \
        pkg-config \
        libwayland-dev \
        libwayland-egl-backend-dev \
        wayland-protocols \
        libegl-dev \
        libgles-dev \
        libxkbcommon-dev \
        libdrm-dev \
        libgbm-dev \
        libinput-dev \
        libudev-dev
}

while [ $# -gt 0 ]; do
    case "$1" in
        --wayland-only) BACKEND_DRM=OFF ;;
        --drm-only)     BACKEND_WAYLAND=OFF ;;
        --clean)        CLEAN=1 ;;
        --install-deps) install_deps; exit 0 ;;
        -j)             JOBS="$2"; shift ;;
        -h|--help)      usage; exit 0 ;;
        *)              echo "Unknown option: $1"; usage; exit 1 ;;
    esac
    shift
done

if [ "$CLEAN" -eq 1 ] && [ -d "$BUILD_DIR" ]; then
    echo "--- Cleaning build directory ---"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "--- Configuring (Wayland=$BACKEND_WAYLAND, DRM=$BACKEND_DRM) ---"
cmake "$SCRIPT_DIR" \
    -DUI_BACKEND_WAYLAND="$BACKEND_WAYLAND" \
    -DUI_BACKEND_DRM="$BACKEND_DRM"

echo "--- Building (jobs=$JOBS) ---"
make -j"$JOBS"

echo ""
echo "Build complete. Run the hello example with:"
echo "  $BUILD_DIR/clue_hello"

#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
BACKEND_WAYLAND=ON
BACKEND_X11=ON
BACKEND_DRM=ON
CLEAN=0
INSTALL=0
JOBS=$(nproc 2>/dev/null || echo 4)

usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --wayland-only   Build only the Wayland backend"
    echo "  --x11-only       Build only the X11 backend"
    echo "  --drm-only       Build only the DRM/KMS backend"
    echo "  --no-wayland     Disable Wayland backend"
    echo "  --no-x11         Disable X11 backend"
    echo "  --no-drm         Disable DRM backend"
    echo "  --clean          Remove build directory before building"
    echo "  --install-deps   Install build dependencies (requires sudo)"
    echo "  --install        Build, install, and build demo"
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
        --wayland-only) BACKEND_X11=OFF; BACKEND_DRM=OFF ;;
        --x11-only)     BACKEND_WAYLAND=OFF; BACKEND_DRM=OFF ;;
        --drm-only)     BACKEND_WAYLAND=OFF; BACKEND_X11=OFF ;;
        --no-wayland)   BACKEND_WAYLAND=OFF ;;
        --no-x11)       BACKEND_X11=OFF ;;
        --no-drm)       BACKEND_DRM=OFF ;;
        --clean)        CLEAN=1 ;;
        --install)      INSTALL=1 ;;
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

echo "--- Configuring (Wayland=$BACKEND_WAYLAND, X11=$BACKEND_X11, DRM=$BACKEND_DRM) ---"
cmake "$SCRIPT_DIR" \
    -DUI_BACKEND_WAYLAND="$BACKEND_WAYLAND" \
    -DUI_BACKEND_X11="$BACKEND_X11" \
    -DUI_BACKEND_DRM="$BACKEND_DRM"

echo "--- Building (jobs=$JOBS) ---"
make -j"$JOBS"

if [ "$INSTALL" -eq 1 ]; then
    echo "--- Installing ---"
    sudo make install
    echo "--- Building demo ---"
    DEMO_DIR="${CMAKE_INSTALL_PREFIX:-/usr/local}/share/clue/examples/demo"
    if [ -d "$DEMO_DIR" ]; then
        cd "$DEMO_DIR"
        sudo make
        echo ""
        echo "Install complete. Run the demo with:"
        echo "  $DEMO_DIR/clue_demo"
    fi
else
    echo ""
    echo "Build complete. To install:"
    echo "  $0 --install"
fi

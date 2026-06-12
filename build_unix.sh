#!/bin/bash
# BLUSK Linux/macOS 빌드 스크립트

BUILD_DIR="build_unix"

echo "[BLUSK] Configuring..."
cmake -S . -B $BUILD_DIR \
    -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo "[BLUSK] CMake configure failed."
    exit 1
fi

echo "[BLUSK] Building..."
cmake --build $BUILD_DIR --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ $? -ne 0 ]; then
    echo "[BLUSK] Build failed."
    exit 1
fi

echo ""
echo "[BLUSK] Build succeeded!"
echo "  Executable: $BUILD_DIR/blusk"
echo ""
echo "Usage:"
echo "  ./build_unix/blusk examples/01_basic.blusk"
echo "  ./build_unix/blusk --repl"
echo "  ./build_unix/blusk --version"

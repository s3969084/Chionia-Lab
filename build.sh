#!/bin/bash

set -e

BUILD_DIR="cmake-build-debug"
SHADER_DIR="shaders"

echo "🔄 Cleaning previous build..."
rm -rf "$BUILD_DIR"
mkdir "$BUILD_DIR"

echo "🎨 Compiling shaders..."
rm -f $SHADER_DIR/*.spv  # Remove previously compiled shader binaries
glslc $SHADER_DIR/point.vert -o $SHADER_DIR/point.vert.spv
glslc $SHADER_DIR/point.frag -o $SHADER_DIR/point.frag.spv

echo "🛠️ Running CMake configuration with Ninja..."
cd "$BUILD_DIR"
cmake -G Ninja ..

echo "🚧 Building the project..."
cmake --build .

echo "✅ Build complete."

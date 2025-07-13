#!/bin/bash

set -e

BUILD_DIR="cmake-build-debug"
SHADER_DIR="shaders"

echo "🔄 Cleaning previous build..."
rm -rf "$BUILD_DIR"
rm -f $SHADER_DIR/*.spv  # Remove previously compiled shader binaries
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

echo "🛠️ Running CMake configuration with Ninja..."
cmake -G Ninja ..

echo "🎨 Compiling shaders..."
cd ../$SHADER_DIR
glslc point.vert -o point.vert.spv
glslc point.frag -o point.frag.spv
cd ../$BUILD_DIR

echo "🚧 Building the project..."
cmake --build .

echo "✅ Build complete."

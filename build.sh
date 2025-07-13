#!/bin/bash

set -e

BUILD_DIR="cmake-build-debug"
SHADER_SRC_DIR="src/shaders"
SHADER_OUT_DIR="$BUILD_DIR/shaders"

echo "🔄 Cleaning previous build..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "🧹 Cleaning old shader outputs..."
rm -rf "$SHADER_OUT_DIR"
mkdir -p "$SHADER_OUT_DIR"

echo "🎨 Compiling shaders into build folder..."
glslc "$SHADER_SRC_DIR/point.vert" -o "$SHADER_OUT_DIR/point.vert.spv"
glslc "$SHADER_SRC_DIR/point.frag" -o "$SHADER_OUT_DIR/point.frag.spv"

echo "🛠️ Running CMake configuration with Ninja..."
cd "$BUILD_DIR"
cmake -G Ninja ..

echo "🚧 Building the project via CMake..."
cmake --build .

echo "✅ Build complete."

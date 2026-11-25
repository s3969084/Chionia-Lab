#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$repo_root"

: "${VCPKG_ROOT:?Set VCPKG_ROOT to your vcpkg checkout before building}"
: "${VULKAN_SDK:?Install the Vulkan SDK and set VULKAN_SDK before building}"

if [[ ! -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]]; then
    echo "Could not find the vcpkg CMake toolchain under VCPKG_ROOT: $VCPKG_ROOT" >&2
    exit 1
fi

if [[ ! -x "$VULKAN_SDK/bin/glslc" ]]; then
    echo "Could not find executable glslc at $VULKAN_SDK/bin/glslc" >&2
    exit 1
fi

for tool in cmake ninja; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "$tool is required and must be available on PATH" >&2
        exit 1
    fi
done

echo "Configuring Linux Release..."
cmake --preset linux-release

echo "Building Chionia Lab..."
cmake --build --preset linux-release

echo "Build complete. Run build/linux-release/Chionia_Lab from its build directory."

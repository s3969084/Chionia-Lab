
$ErrorActionPreference = "Stop"

$BUILD_DIR = "cmake-build-debug"
$VCPKG_TOOLCHAIN = "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"

Write-Host "======================================"
Write-Host "          Chionia Lab Build"
Write-Host "======================================"

# Verify dependencies
if (!(Test-Path $VCPKG_TOOLCHAIN)) {
    throw "vcpkg toolchain not found: $VCPKG_TOOLCHAIN"
}

Write-Host "Configuring CMake..."

cmake -S . -B $BUILD_DIR -G Ninja `
    -DCMAKE_BUILD_TYPE=Debug `
    "-DCMAKE_TOOLCHAIN_FILE=$VCPKG_TOOLCHAIN"

if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed."
}

Write-Host "Building project..."

cmake --build $BUILD_DIR

if ($LASTEXITCODE -ne 0) {
    throw "Build failed."
}

Write-Host "======================================"
Write-Host "          Build complete!"
Write-Host "======================================"

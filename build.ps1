$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $repoRoot

try {
    if ([string]::IsNullOrWhiteSpace($env:VCPKG_ROOT)) {
        throw "Set VCPKG_ROOT to your vcpkg checkout before building."
    }

    $vcpkgToolchain = Join-Path $env:VCPKG_ROOT "scripts/buildsystems/vcpkg.cmake"
    if (!(Test-Path $vcpkgToolchain)) {
        throw "Could not find the vcpkg CMake toolchain at $vcpkgToolchain."
    }

    if ([string]::IsNullOrWhiteSpace($env:VULKAN_SDK)) {
        throw "Install the Vulkan SDK and set VULKAN_SDK before building."
    }

    $glslc = Join-Path $env:VULKAN_SDK "Bin/glslc.exe"
    if (!(Test-Path $glslc)) {
        throw "Could not find glslc at $glslc."
    }

    foreach ($tool in @("cmake", "ninja")) {
        if (!(Get-Command $tool -ErrorAction SilentlyContinue)) {
            throw "$tool is required and must be available on PATH."
        }
    }

    Write-Host "Configuring Windows Release..."
    cmake --preset windows-release
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed."
    }

    Write-Host "Building Chionia Lab..."
    cmake --build --preset windows-release
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed."
    }

    Write-Host "Build complete. Run build/windows-release/Chionia_Lab.exe from its build directory."
}
finally {
    Pop-Location
}

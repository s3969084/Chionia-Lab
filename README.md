# Chionia Lab

Chionia Lab is a Vulkan point-cloud shape viewer. It includes a shape library, animated morphing, particle vibration, a ground guide with a moving light, and an optional floor reflection.

## Licensing

The [draft Chionia Lab Proprietary Non-Commercial Evaluation License](LICENSE) describes the intended terms: non-commercial evaluation is allowed, while commercial use requires a separate written license with the applicable rightsholder(s) and payment of the agreed fee before use begins. Confirm ownership and contributor authority before treating it as an active license offer. Third-party dependencies and materials remain under their own licenses. See [commercial licensing notes](docs/COMMERCIAL_LICENSING.md) for details.

If adopted, this policy would make the project source-available rather than open-source under the Open Source Definition, because it restricts commercial use.

## Requirements

- Git
- CMake 3.25 or newer
- Ninja
- A C++17 compiler (Visual Studio 2022 on Windows, GCC or Clang on Linux)
- The Vulkan SDK, including `glslc`
- [vcpkg](https://github.com/microsoft/vcpkg)

The vcpkg manifest installs GLFW, GLM, and nlohmann-json at the pinned registry baseline. CMake fetches the pinned Dear ImGui source and compiles the shaders as part of the normal build.

To use this branch, clone it with:

```bash
git clone --branch codex/reproducible-build https://github.com/s3969084/Chionia-Lab.git
cd Chionia-Lab
```

Install vcpkg for your platform and set `VCPKG_ROOT` to its checkout. Install the Vulkan SDK; its installer normally sets `VULKAN_SDK` and includes `glslc`.

## Windows

Open a Visual Studio 2022 Developer PowerShell so the MSVC compiler is available. If you have not installed vcpkg yet, bootstrap it once:

```powershell
git clone https://github.com/microsoft/vcpkg.git "$env:USERPROFILE\vcpkg"
& "$env:USERPROFILE\vcpkg\bootstrap-vcpkg.bat"
```

Set `VCPKG_ROOT` to your vcpkg checkout and build:

```powershell
$env:VCPKG_ROOT = "$env:USERPROFILE\vcpkg"
.\build.ps1
```

To run the app, use the build directory as the working directory so the generated shaders and copied data are found:

```powershell
Push-Location build\windows-release
.\Chionia_Lab.exe
Pop-Location
```

## Linux

Install a C++ compiler, CMake, Ninja, and the Vulkan SDK. If vcpkg is not installed yet, bootstrap it once and set `VCPKG_ROOT`:

```bash
git clone https://github.com/microsoft/vcpkg.git "$HOME/vcpkg"
"$HOME/vcpkg/bootstrap-vcpkg.sh"
export VCPKG_ROOT="$HOME/vcpkg"
```

Make sure `glslc` is available under `$VULKAN_SDK/bin`, then build:

```bash
./build.sh
```

Run the app from its build directory:

```bash
cd build/linux-release
./Chionia_Lab
```

## Tests

The default presets build the scene test executable. Run it with:

```powershell
ctest --preset windows-release
```

or on Linux:

```bash
ctest --preset linux-release
```

## Controls

- Left-drag orbits the camera.
- Right-drag rotates the selected shape.
- Scroll zooms.
- **Auto morph** cycles through the library shapes every five seconds while **Morphing** is enabled.

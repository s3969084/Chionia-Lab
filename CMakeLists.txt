
cmake_minimum_required(VERSION 3.20)

project(Chionia_Lab VERSION 0.1.0 LANGUAGES CXX)

# ============================================================
# Project configuration
# ============================================================

option(CHIONIA_DEBUG "Enable Vulkan validation layers and debug utils" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CHIONIA_DEBUG)
    add_compile_definitions(CHIONIA_DEBUG)
endif()

# ============================================================
# Dependencies
# ============================================================

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Dear ImGui is built from its pinned upstream source so its C++ ABI matches
# whichever compiler the project uses (including CLion's MinGW toolchain).
include(FetchContent)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

FetchContent_Declare(
        dear_imgui
        URL "https://github.com/ocornut/imgui/archive/refs/tags/v1.93.0.tar.gz"
        URL_HASH SHA512=de41ef101f37d09685bf9e69bacefe06a0c010ecd0e6e7ed1e4efbff82fde27a9c3ae36ed436db5bcde4c61de10da984dfb74d6098586a3f78d8a716864460c2
)

FetchContent_MakeAvailable(dear_imgui)

add_library(Chionia_ImGui STATIC
        "${dear_imgui_SOURCE_DIR}/imgui.cpp"
        "${dear_imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${dear_imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${dear_imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${dear_imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
        "${dear_imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp"
)

target_include_directories(Chionia_ImGui PUBLIC
        "${dear_imgui_SOURCE_DIR}"
        "${dear_imgui_SOURCE_DIR}/backends"
)

target_link_libraries(Chionia_ImGui PUBLIC
        glfw
        Vulkan::Vulkan
)

target_compile_features(Chionia_ImGui PUBLIC cxx_std_11)

# ============================================================
# Source files
# ============================================================

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

add_executable(Chionia_Lab ${SOURCES}
        src/scene/shapes/shape_sampling_config.hpp)

target_compile_definitions(Chionia_Lab PRIVATE
        CHIONIA_VERSION="${PROJECT_VERSION}"
)

target_include_directories(Chionia_Lab PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# ============================================================
# Link libraries
# ============================================================

target_link_libraries(Chionia_Lab PRIVATE
        Vulkan::Vulkan
        glfw
        glm::glm
        nlohmann_json::nlohmann_json
        Chionia_ImGui
)

include(CTest)

if(BUILD_TESTING)
    add_executable(Chionia_Scene_Tests
            tests/scene_tests.cpp
            src/core/resources/point_cloud.cpp
            src/graphics/conversion/point_vertex_converter.cpp
            src/lesson/solar_system_lesson.cpp
            src/scene/scene.cpp
            src/scene/scene_object.cpp
            src/scene/procedural/infinity_generator.cpp
            src/scene/transform.cpp
    )

    target_include_directories(Chionia_Scene_Tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    target_link_libraries(Chionia_Scene_Tests PRIVATE
            glm::glm
    )

    add_test(
            NAME chionia_scene_tests
            COMMAND Chionia_Scene_Tests
    )
endif()

# MinGW executables need their GCC runtime DLLs beside the executable when
# launched outside CLion. Copy the compiler's runtime for both the app and
# the standalone test binary.
if(MINGW)
    get_filename_component(
            CHIONIA_MINGW_RUNTIME_DIR
            "${CMAKE_CXX_COMPILER}"
            DIRECTORY
    )

    set(CHIONIA_RUNTIME_TARGETS Chionia_Lab)
    if(BUILD_TESTING)
        list(APPEND CHIONIA_RUNTIME_TARGETS Chionia_Scene_Tests)
    endif()

    foreach(TARGET_NAME IN LISTS CHIONIA_RUNTIME_TARGETS)
        foreach(RUNTIME_DLL
                libwinpthread-1.dll
                libgcc_s_seh-1.dll
                libstdc++-6.dll)
            set(RUNTIME_DLL_SOURCE
                    "${CHIONIA_MINGW_RUNTIME_DIR}/${RUNTIME_DLL}")

            if(EXISTS "${RUNTIME_DLL_SOURCE}")
                add_custom_command(
                        TARGET ${TARGET_NAME}
                        POST_BUILD
                        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        "${RUNTIME_DLL_SOURCE}"
                        "$<TARGET_FILE_DIR:${TARGET_NAME}>"
                        COMMENT "Copying ${RUNTIME_DLL} for ${TARGET_NAME}"
                        VERBATIM
                )
            endif()
        endforeach()
    endforeach()
endif()

# ============================================================
# Platform configuration
# ============================================================

if(WIN32)

    message(STATUS "Configuring Chionia for Windows")

    target_compile_definitions(Chionia_Lab PRIVATE
            VK_USE_PLATFORM_WIN32_KHR
            NOMINMAX
            WIN32_LEAN_AND_MEAN
    )

    target_link_libraries(Chionia_Lab PRIVATE
            dwmapi
    )

elseif(APPLE)

    message(STATUS "Configuring Chionia for macOS")

    find_library(MOLTENVK_LIBRARY
            NAMES MoltenVK
            PATHS "$ENV{VULKAN_SDK}/lib"
            REQUIRED
    )

    target_link_libraries(Chionia_Lab PRIVATE
            "${MOLTENVK_LIBRARY}"
    )

else()

    message(STATUS "Configuring Chionia for Linux")

endif()

# ============================================================
# Shader compilation
# ============================================================

file(GLOB SHADER_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.frag"
)

if(WIN32)

    find_program(GLSLC_EXECUTABLE glslc
            HINTS "$ENV{VULKAN_SDK}/Bin"
            REQUIRED
    )

else()

    find_program(GLSLC_EXECUTABLE glslc
            HINTS "$ENV{VULKAN_SDK}/bin"
            REQUIRED
    )

endif()

set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

file(MAKE_DIRECTORY "${SHADER_BINARY_DIR}")

set(COMPILED_SHADERS)

foreach(SHADER_FILE IN LISTS SHADER_FILES)

    get_filename_component(FILE_NAME "${SHADER_FILE}" NAME)

    set(SPV "${SHADER_BINARY_DIR}/${FILE_NAME}.spv")

    add_custom_command(
            OUTPUT "${SPV}"
            COMMAND "${GLSLC_EXECUTABLE}" "${SHADER_FILE}" -o "${SPV}"
            DEPENDS "${SHADER_FILE}"
            COMMENT "Compiling shader: ${FILE_NAME}"
            VERBATIM
    )

    list(APPEND COMPILED_SHADERS "${SPV}")

endforeach()

add_custom_target(Shaders ALL
        DEPENDS ${COMPILED_SHADERS}
)

add_dependencies(Chionia_Lab Shaders)

# ============================================================
# Copy application data
# ============================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")

    add_custom_command(
            TARGET Chionia_Lab POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:Chionia_Lab>/data"
            COMMENT "Copying application data"
    )

endif()

message(STATUS "Chionia CMake configuration complete")

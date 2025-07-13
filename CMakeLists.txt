cmake_minimum_required(VERSION 3.20)
project(Chionia_Lab VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Vulkan SDK Paths ===
set(VULKAN_SDK "/Users/angeloschristou/VulkanSDK/1.4.313.1/macOS")
set(VULKAN_INCLUDE_DIR "${VULKAN_SDK}/include")
set(VULKAN_LIBRARY "${VULKAN_SDK}/lib/libvulkan.1.dylib")
set(MOLTENVK_LIBRARY "${VULKAN_SDK}/lib/libMoltenVK.dylib")

# === External Dependencies ===
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h PATHS /opt/homebrew/include /usr/local/include)
find_library(GLFW_LIBRARY NAMES glfw PATHS /opt/homebrew/lib /usr/local/lib)

if(NOT GLFW_INCLUDE_DIR OR NOT GLFW_LIBRARY)
    message(FATAL_ERROR "GLFW not found. Install it via: brew install glfw")
endif()

find_path(GLM_INCLUDE_DIR glm/glm.hpp PATHS /opt/homebrew/include /usr/local/include)
if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "GLM not found. Install it via: brew install glm")
endif()

# === Global Include Paths ===
include_directories(
        ${VULKAN_INCLUDE_DIR}
        ${GLFW_INCLUDE_DIR}
        ${GLM_INCLUDE_DIR}
)

# === Shader Compilation ===
file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/src/shaders/*.vert" "${CMAKE_SOURCE_DIR}/src/shaders/*.frag")
set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

find_program(GLSLC_EXECUTABLE glslc HINTS "${VULKAN_SDK}/bin")
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Ensure the Vulkan SDK is installed and accessible.")
endif()

foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(FILE_NAME ${SHADER_FILE} NAME)
    set(SPV "${SHADER_BINARY_DIR}/${FILE_NAME}.spv")

    add_custom_command(
            OUTPUT ${SPV}
            COMMAND ${GLSLC_EXECUTABLE} ${SHADER_FILE} -o ${SPV}
            DEPENDS ${SHADER_FILE}
            COMMENT "🔹 Compiling shader: ${SHADER_FILE}"
            VERBATIM
    )

    list(APPEND COMPILED_SHADERS ${SPV})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})

# === Subdirectories ===
add_subdirectory(engine)
add_subdirectory(src)

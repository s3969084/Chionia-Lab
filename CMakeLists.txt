cmake_minimum_required(VERSION 3.20)
project(Chionia_Lab VERSION 0.1 LANGUAGES CXX)

# === Compiler Settings ===
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Vulkan SDK Paths ===
set(VULKAN_SDK "/Users/angeloschristou/VulkanSDK/1.4.313.1/macOS")
set(VULKAN_INCLUDE_DIR "${VULKAN_SDK}/include")
set(VULKAN_LIBRARY "${VULKAN_SDK}/lib/libvulkan.1.dylib")
set(MOLTENVK_LIBRARY "${VULKAN_SDK}/lib/libMoltenVK.dylib")

# === GLFW (via Homebrew) ===
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h PATHS /opt/homebrew/include /usr/local/include)
find_library(GLFW_LIBRARY NAMES glfw PATHS /opt/homebrew/lib /usr/local/lib)

if(NOT GLFW_INCLUDE_DIR OR NOT GLFW_LIBRARY)
    message(FATAL_ERROR "GLFW not found. Install it via: brew install glfw")
endif()

# === GLM (via Homebrew) ===
find_path(GLM_INCLUDE_DIR glm/glm.hpp PATHS /opt/homebrew/include /usr/local/include)

if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "GLM not found. Install it via: brew install glm")
endif()

# === Collect Source Files ===
file(GLOB_RECURSE SOURCES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_coordinator/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_coordinator/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/core/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/core/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_instance/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_instance/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_surface/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_surface/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_physical_device/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_physical_device/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_logical_device/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_logical_device/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_swapchain/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_swapchain/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_render_pass/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_render_pass/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_frame_buffer/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_frame_buffer/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_command_pool/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_command_pool/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_command_buffers/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_command_buffers/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_sync_objects/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_sync_objects/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_pipeline_builder/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_pipeline_builder/*.hpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_renderer/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/vulkan_backend/vulkan_renderer/*.hpp



)

# === Add Executable Target ===
add_executable(${PROJECT_NAME} ${SOURCES})

# === Include Directories ===
target_include_directories(${PROJECT_NAME} PRIVATE
        ${VULKAN_INCLUDE_DIR}
        ${GLFW_INCLUDE_DIR}
        ${GLM_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/engine
)

# === Preprocessor Definitions ===
target_compile_definitions(${PROJECT_NAME} PRIVATE CHIONIA_DEBUG)

# === Link External Libraries ===
target_link_libraries(${PROJECT_NAME} PRIVATE
        ${GLFW_LIBRARY}
        ${VULKAN_LIBRARY}
        ${MOLTENVK_LIBRARY}
)

# === macOS Frameworks ===
if(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            VK_USE_PLATFORM_MACOS_MVK
            GLFW_INCLUDE_VULKAN
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework Metal"
            "-framework QuartzCore"
    )
endif()

# === Shader Compilation ===
find_program(GLSLC_EXECUTABLE glslc HINTS "${VULKAN_SDK}/bin")
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Ensure the Vulkan SDK is installed and accessible.")
endif()

set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

set(SHADER_FILES
        point.vert
        point.frag
)

set(COMPILED_SHADERS "")
foreach(SHADER_FILE ${SHADER_FILES})
    set(SRC "${SHADER_SOURCE_DIR}/${SHADER_FILE}")
    set(SPV "${SHADER_BINARY_DIR}/${SHADER_FILE}.spv")

    add_custom_command(
            OUTPUT ${SPV}
            COMMAND ${GLSLC_EXECUTABLE} ${SRC} -o ${SPV}
            DEPENDS ${SRC}
            COMMENT "🔹 Compiling shader: ${SHADER_FILE}"
            VERBATIM
    )

    list(APPEND COMPILED_SHADERS ${SPV})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})
add_dependencies(${PROJECT_NAME} Shaders)

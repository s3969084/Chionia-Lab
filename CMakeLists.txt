cmake_minimum_required(VERSION 3.20)
project(Chionia_Lab VERSION 0.1 LANGUAGES CXX)

option(CHIONIA_DEBUG "Enable Vulkan validation layers and debug utils" ON)

if(CHIONIA_DEBUG)
    add_compile_definitions(CHIONIA_DEBUG)
endif()


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Vulkan SDK ===
set(VULKAN_SDK "/Users/angeloschristou/VulkanSDK/1.4.313.1/macOS")
set(VULKAN_INCLUDE_DIR "${VULKAN_SDK}/include")
set(VULKAN_LIBRARY "${VULKAN_SDK}/lib/libvulkan.1.dylib")
set(MOLTENVK_LIBRARY "${VULKAN_SDK}/lib/libMoltenVK.dylib")

# === Dependencies ===
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h PATHS /opt/homebrew/include /usr/local/include)
find_library(GLFW_LIBRARY NAMES glfw PATHS /opt/homebrew/lib /usr/local/lib)

find_path(GLM_INCLUDE_DIR glm/glm.hpp PATHS /opt/homebrew/include /usr/local/include)
find_package(nlohmann_json REQUIRED)

if(NOT GLFW_INCLUDE_DIR OR NOT GLFW_LIBRARY)
    message(FATAL_ERROR "GLFW not found. Install it with: brew install glfw")
endif()

if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "GLM not found. Install it with: brew install glm")
endif()

# === Include Paths ===
include_directories(
        ${VULKAN_INCLUDE_DIR}
        ${GLFW_INCLUDE_DIR}
        ${GLM_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/src

)

# === Shader Compilation ===
file(GLOB SHADER_FILES
        "${CMAKE_SOURCE_DIR}/src/shaders/*.vert"
        "${CMAKE_SOURCE_DIR}/src/shaders/*.frag"
)

set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

find_program(GLSLC_EXECUTABLE glslc HINTS "${VULKAN_SDK}/bin")
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Ensure Vulkan SDK is installed and in your PATH.")
endif()

foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(FILE_NAME ${SHADER_FILE} NAME)
    set(SPV "${SHADER_BINARY_DIR}/${FILE_NAME}.spv")

    add_custom_command(
            OUTPUT ${SPV}
            COMMAND ${GLSLC_EXECUTABLE} ${SHADER_FILE} -o ${SPV}
            DEPENDS ${SHADER_FILE}
            COMMENT "Compiling shader: ${FILE_NAME}"
            VERBATIM
    )

    list(APPEND COMPILED_SHADERS ${SPV})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})

# === Source Files ===
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.hpp
)

# === Executable ===
add_executable(Chionia_Lab ${SOURCES})

# === Link Libraries ===
target_link_libraries(Chionia_Lab
        PRIVATE
        ${GLFW_LIBRARY}
        ${VULKAN_LIBRARY}
        ${MOLTENVK_LIBRARY}
        nlohmann_json::nlohmann_json
)

# === Copy data/ folder to build directory ===
add_custom_command(
        TARGET Chionia_Lab POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/data
        COMMENT "Copying data/ folder to build directory"
)

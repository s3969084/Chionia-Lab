
cmake_minimum_required(VERSION 3.24)

project(Chionia_Lab VERSION 0.1 LANGUAGES CXX)

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

# ============================================================
# Source files
# ============================================================

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/*.hpp"
)

add_executable(Chionia_Lab ${SOURCES})

target_include_directories(Chionia_Lab PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
)

# ============================================================
# Link libraries
# ============================================================

target_link_libraries(Chionia_Lab PRIVATE
        Vulkan::Vulkan
        glfw
        glm::glm
        nlohmann_json::nlohmann_json
)

# ============================================================
# Platform-specific configuration
# ============================================================

if(WIN32)
    message(STATUS "Configuring Chionia for Windows")

    target_compile_definitions(Chionia_Lab PRIVATE
            VK_USE_PLATFORM_WIN32_KHR
            NOMINMAX
            WIN32_LEAN_AND_MEAN
    )

elseif(APPLE)
    message(STATUS "Configuring Chionia for macOS")

    find_library(MOLTENVK_LIBRARY
            NAMES MoltenVK
            PATHS "${VULKAN_SDK}/lib"
            REQUIRED
    )

    target_link_libraries(Chionia_Lab PRIVATE
            "${MOLTENVK_LIBRARY}"
    )
endif()

# ============================================================
# Shader compilation
# ============================================================

file(GLOB SHADER_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/shaders/*.vert"
        "${CMAKE_SOURCE_DIR}/src/shaders/*.frag"
)

find_program(GLSLC_EXECUTABLE glslc
        HINTS
        "$ENV{VULKAN_SDK}/Bin"
        "$ENV{VULKAN_SDK}/bin"
        REQUIRED
)

set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders")

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

add_custom_command(
        TARGET Chionia_Lab POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/data"
        "$<TARGET_FILE_DIR:Chionia_Lab>/data"
        COMMENT "Copying application data"
)

message(STATUS "Chionia CMake configuration complete")

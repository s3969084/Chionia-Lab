# Find the GLSLC compiler from the Vulkan SDK
find_program(GLSLC_EXECUTABLE glslc HINTS "${VULKAN_SDK}/bin")
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Ensure the Vulkan SDK is installed and accessible.")
endif()

# Search for shaders relative to this directory
file(GLOB SHADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.vert ${CMAKE_CURRENT_SOURCE_DIR}/*.frag)

# Output compiled shaders to the build directory under /src/shaders
set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

# Compile each shader file to .spv
foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(FILE_NAME ${SHADER_FILE} NAME)
    set(SPV "${SHADER_BINARY_DIR}/${FILE_NAME}.spv")

    add_custom_command(
            OUTPUT ${SPV}
            COMMAND ${GLSLC_EXECUTABLE} ${SHADER_FILE} -o ${SPV}
            DEPENDS ${SHADER_FILE}
            COMMENT "🔹 Compiling shader: ${FILE_NAME}"
            VERBATIM
    )

    list(APPEND COMPILED_SHADERS ${SPV})
endforeach()

# Add a custom build target to compile all shaders
add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})

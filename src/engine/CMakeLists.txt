# Collect all .cpp and .hpp files under engine/, including subfolders like vulkan/
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

# Create a static library for the engine
add_library(chionia_engine STATIC ${ENGINE_SOURCES})

# Allow external targets (like main executable) to include engine headers
target_include_directories(chionia_engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}    # Includes headers in engine/
        ${CMAKE_CURRENT_SOURCE_DIR}/vulkan   # Includes vulkan headers
)

# Internal includes (e.g., accessing src/ for point cloud loading or app-level resources)
target_include_directories(chionia_engine PRIVATE
        ${CMAKE_SOURCE_DIR}/src  # Include src for accessing external resources
)

# Enable global debug macro
target_compile_definitions(chionia_engine PUBLIC
        CHIONIA_DEBUG
)

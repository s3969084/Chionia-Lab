# engine/CMakeLists.txt

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

add_library(chionia_engine STATIC ${ENGINE_SOURCES})

# Public include for modules that depend on engine
target_include_directories(chionia_engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Allow engine to include from src/ if needed (e.g., point_cloud_loader)
target_include_directories(chionia_engine PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(chionia_engine PUBLIC CHIONIA_DEBUG)

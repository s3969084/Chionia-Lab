# Collect all .cpp and .hpp files under utils/
file(GLOB UTILS_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

# Create a static library for utils
add_library(chionia_utils STATIC ${UTILS_SOURCES})

# Ensure we can find the nlohmann_json package
find_package(nlohmann_json REQUIRED)

# Link nlohmann_json to our utils library
target_link_libraries(chionia_utils PUBLIC nlohmann_json::nlohmann_json)

# Include the current source directory for utils
target_include_directories(chionia_utils PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

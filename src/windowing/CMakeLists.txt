# Collect all .cpp and .hpp files under windowing/
file(GLOB WINDOWING_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

# Create a static library for windowing
add_library(chionia_windowing STATIC ${WINDOWING_SOURCES})

# Include headers for windowing
target_include_directories(chionia_windowing PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link against the necessary libraries (e.g., engine)
target_link_libraries(chionia_windowing PUBLIC chionia_engine)

# Add subdirectories for each module in the source folder
add_subdirectory(engine)   # Engine includes Vulkan setup now
add_subdirectory(utils)    # For utils like point_cloud_loader.cpp
add_subdirectory(windowing)  # Windowing system (AppWindow)

# Include paths needed by the main app (including engine, utils, windowing, etc.)
# This is done in the root CMakeLists.txt now
# target_include_directories(Chionia_Lab PRIVATE
#        ${CMAKE_CURRENT_SOURCE_DIR}       # src/
#        ${CMAKE_CURRENT_SOURCE_DIR}/engine  # Includes headers for engine and Vulkan
#        ${CMAKE_CURRENT_SOURCE_DIR}/utils  # Includes utils (like point_cloud_loader)
#        ${CMAKE_CURRENT_SOURCE_DIR}/windowing  # Includes headers for windowing (AppWindow)
# )

# Optional: define debug macros
# target_compile_definitions(Chionia_Lab PRIVATE
#        CHIONIA_DEBUG
# )

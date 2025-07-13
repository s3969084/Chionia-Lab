file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS *.cpp)

add_executable(Chionia_Lab ${SRC_FILES})

target_link_libraries(Chionia_Lab
        PRIVATE
        chionia_engine
        ${GLFW_LIBRARY}
        ${VULKAN_LIBRARY}
        ${MOLTENVK_LIBRARY}
)

if(APPLE)
    target_compile_definitions(Chionia_Lab PRIVATE
            VK_USE_PLATFORM_MACOS_MVK
            GLFW_INCLUDE_VULKAN
    )
    target_link_libraries(Chionia_Lab PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework Metal"
            "-framework QuartzCore"
    )
endif()

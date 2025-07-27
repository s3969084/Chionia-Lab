#pragma once

#include "engine/vulkan_api/vulkan_coordinator.hpp"
#include "utils/terminal_window.hpp"
#include "utils/camera_controller.hpp"
#include "engine/transform/camera.hpp"

namespace chionia {

    class Application {
    public:
        Application();  // Needed to initialize controller_ with camera_
        ~Application();

        void run();  // Run everything here

    private:
        VulkanCoordinator vk_;
        TerminalWindow terminal_;

        Camera camera_;
        CameraController controller_;
    };

}

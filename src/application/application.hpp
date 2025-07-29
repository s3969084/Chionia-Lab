
#pragma once

#include "backend/gpu_api/vulkan/VulkanBackend.hpp"
#include "ui/terminal_window.hpp"
#include "scene/camera_controller.hpp"
#include "scene/camera.hpp"

namespace chionia {

    class Application {
    public:
        Application();
        ~Application();

        void run();



    private:
        VulkanBackend vk_;
        TerminalWindow terminal_;

        Camera camera_;
        CameraController controller_{camera_};



    };

}

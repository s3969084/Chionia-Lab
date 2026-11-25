
#pragma once

#include "backend/gpu_api/vulkan/VulkanBackend.hpp"
#include "ui/terminal_window.hpp"
#include "scene/camera_controller.hpp"
#include "scene/camera.hpp"

#include <atomic>

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

        // Notifies the main thread when a manual shape is loaded.
        std::atomic<unsigned int> manualLoadVersion_{0};
        std::atomic<int> manualShapeIndex_{-1};
    };

}

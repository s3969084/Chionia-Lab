#pragma once

#include "engine/vulkan/vulkan_coordinator.hpp"
#include "utils/terminal_window.hpp"

namespace chionia {
    class Application {
    public:
        Application() = default;
        ~Application();

        void run();  // Run everything here

    private:
        VulkanCoordinator vk_;
        TerminalWindow terminal_;
    };
}

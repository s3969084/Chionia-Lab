#include "engine/vulkan/vulkan_coordinator.hpp"
#include <iostream>


int main() {
    chionia::VulkanCoordinator vulkan_Coordinator;

    try {
        vulkan_Coordinator.init();
        vulkan_Coordinator.run();
        vulkan_Coordinator.cleanup();
    } catch (const std::exception e) {
        std::cerr << "❌ Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}

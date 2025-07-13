#include "vulkan_coordinator.hpp"
#include <iostream>


int main() {
    chionia::VulkanCoordinator coordinator;

    try {
        coordinator.init();
        coordinator.run();
        coordinator.cleanup();
    } catch (const std::exception e) {
        std::cerr << "❌ Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}

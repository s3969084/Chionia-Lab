#include "vulkan_surface.hpp"
#include <stdexcept>
#include <iostream>

namespace chionia {

    VulkanSurface::~VulkanSurface() {
        // No-op here, destruction is manual via destroy()
    }

    void VulkanSurface::create(GLFWwindow* window, VkInstance instance) {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vulkan surface!");
        }
        std::cout << "✅ Vulkan Surface created successfully.\n";
    }

    void VulkanSurface::destroy(VkInstance instance) {
        if (surface_ != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance, surface_, nullptr);
            surface_ = VK_NULL_HANDLE;
        }
    }

    VkSurfaceKHR VulkanSurface::get() const {
        return surface_;
    }


}

#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace chionia {

    class VulkanSurface {
    public:
        VulkanSurface() = default;
        ~VulkanSurface();

        void create(GLFWwindow* window, VkInstance instance);
        void destroy(VkInstance instance);

        VkSurfaceKHR get() const;

    private:
        VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    };
}
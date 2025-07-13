#pragma once

#include <vulkan/vulkan.h>
#include <vector>


namespace chionia {

    class VulkanPhysicalDevice {
    public:
        VulkanPhysicalDevice() = default;
        ~VulkanPhysicalDevice() = default;

        void pick(VkInstance instance, VkSurfaceKHR surface);
        VkPhysicalDevice get() const;

        uint32_t findGraphicsQueueFamily(VkInstance instance) const;

    private:
        VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;

        bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    };
}
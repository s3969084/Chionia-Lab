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
        const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const { return memoryProperties_;}

    private:
        VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
        VkPhysicalDeviceMemoryProperties memoryProperties_;

        bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    };
}
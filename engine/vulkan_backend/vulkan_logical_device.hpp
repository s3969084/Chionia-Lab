#pragma once

#include "vulkan/vulkan.h"
#include <optional>

namespace chionia {
    class VulkanLogicalDevice {
    public:
        VulkanLogicalDevice() = default;
        ~VulkanLogicalDevice();

        void create(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
        void destroy();

        VkDevice get() const;
        VkQueue getGraphicsQueue() const;
        VkQueue getPresentQueue() const;
        uint32_t getGraphicsQueueFamily() const;
        uint32_t getPresentQueueFamily() const;

    private:
        VkDevice device_ = VK_NULL_HANDLE;
        VkQueue graphicsQueue_ = VK_NULL_HANDLE;
        VkQueue presentQueue_ = VK_NULL_HANDLE;

        uint32_t graphicsQueueFamily_ = UINT32_MAX;
        uint32_t presentQueueFamily_ = UINT32_MAX;

        void findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    };


}
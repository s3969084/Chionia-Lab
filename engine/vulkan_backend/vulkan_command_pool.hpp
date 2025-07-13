#pragma once
#include <vulkan/vulkan.h>

namespace chionia {

    class VulkanCommandPool {
    public:
        VulkanCommandPool() = default;
        ~VulkanCommandPool() = default;

        void create(VkDevice logicalDevice, uint32_t queueFamilyIndex);
        void destroy(VkDevice logicalDevice);

        VkCommandPool get() const;

    private:
        VkCommandPool commandPool_{VK_NULL_HANDLE};
    };
}
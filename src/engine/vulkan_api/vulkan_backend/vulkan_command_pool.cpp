#include "vulkan_command_pool.hpp"
#include <stdexcept>
#include <iostream>

namespace chionia {

    void VulkanCommandPool::create(VkDevice logicalDevice, uint32_t queueFamilyIndex) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to create command pool!");
        }

        std::cout << "✅ Vulkan command pool created.\n";
    }

    void VulkanCommandPool::destroy(VkDevice logicalDevice) {
        if (commandPool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(logicalDevice, commandPool_, nullptr);
            commandPool_ = VK_NULL_HANDLE;
        }
    }

    VkCommandPool VulkanCommandPool::get() const {
        return commandPool_;
    }

}
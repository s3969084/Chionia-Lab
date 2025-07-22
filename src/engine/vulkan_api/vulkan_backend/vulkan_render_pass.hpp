#pragma once

#include <vulkan/vulkan.h>


namespace chionia {

    class VulkanRenderPass {
    public:
        VulkanRenderPass() = default;
        ~VulkanRenderPass() = default;

        void create(VkDevice logicalDevice, VkFormat swapchainImageFormat);
        void destroy(VkDevice logicalDevice);

        VkRenderPass get() const {
            return renderPass_;
        }

    private:
        VkRenderPass renderPass_ = VK_NULL_HANDLE;
    };
}
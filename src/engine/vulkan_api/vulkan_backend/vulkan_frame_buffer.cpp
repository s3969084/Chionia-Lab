#include "vulkan_frame_buffer.hpp"
#include <stdexcept>
#include <iostream>


namespace chionia {

    void VulkanFramebuffer::create(
        VkDevice logicalDevice,
        VkRenderPass renderPass,
        const std::vector<VkImageView>& swapchainImageViews,
        VkExtent2D extent) {
        framebuffers_.resize(swapchainImageViews.size());

        for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
            VkImageView attachments[] = {
                swapchainImageViews[i]
                };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }

        std::cout << "Vulkan framebuffers created (" << framebuffers_.size() << ").\n";
    }

    void VulkanFramebuffer::destroy(VkDevice logicalDevice) {
        for (auto framebuffer : framebuffers_) {
            if (framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
            }
        }
        framebuffers_.clear();
    }



}
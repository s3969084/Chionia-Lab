#pragma once

#include <vulkan/vulkan.h>
#include <vector>


namespace chionia {

    class VulkanFramebuffer {
    public:
        VulkanFramebuffer() = default;
        ~VulkanFramebuffer() = default;

        void create(
            VkDevice logicalDevice,
            VkRenderPass renderPass,
            const std::vector<VkImageView>& swapchainImageViews,
            VkExtent2D extent
            );

        void destroy(VkDevice logicalDevice);

        const std::vector<VkFramebuffer>& getAll() const { return framebuffers_;}

        const VkFramebuffer& get(size_t index) const {
            return framebuffers_.at(index); // `.at` throws it out of range, safer than []
        }

    private:
        std::vector<VkFramebuffer> framebuffers_;
    };

}
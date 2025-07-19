#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace chionia {

    class VulkanCommandBuffers {
    public:
        VulkanCommandBuffers() = default;
        ~VulkanCommandBuffers() = default;

        void allocate(VkDevice logicalDevice, VkCommandPool commandPool, uint32_t count);
        void free(VkDevice logicalDevice, VkCommandPool commandPool);
        void record(
            VkRenderPass renderPass,
            const std::vector<VkFramebuffer>& framebuffers,
            VkExtent2D extent,
            VkPipeline pipeline,
            VkPipelineLayout layout,
            VkBuffer vertexBuffer,
            uint32_t vertexCount
            );

        const std::vector<VkCommandBuffer>& getAll() const { return commandBuffers_; }
        VkCommandBuffer get(size_t index) const { return commandBuffers_.at(index);}

    private:

        std::vector<VkCommandBuffer> commandBuffers_;


    };
}
#include "vulkan_command_buffers.hpp"
#include <stdexcept>
#include <iostream>

namespace chionia {

    void VulkanCommandBuffers::allocate(VkDevice logicalDevice, VkCommandPool commandPool, uint32_t count) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = count;

        commandBuffers_.resize(count);
        if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to allocate command buffers!");
        }

        std::cout << "✅ Vulkan command buffers allocated (" << count << ").\n";
    }

    void VulkanCommandBuffers::free(VkDevice logicalDevice, VkCommandPool commandPool) {
        if (!commandBuffers_.empty()) {
            vkFreeCommandBuffers(logicalDevice,  commandPool, static_cast<uint32_t>(commandBuffers_.size()), commandBuffers_.data());
            commandBuffers_.clear();
        }
    }

    void VulkanCommandBuffers::record(VkRenderPass renderPass,
        const std::vector<VkFramebuffer>& framebuffers,
        VkExtent2D extent,
        VkPipeline pipeline,
        VkPipelineLayout layout,
        VkBuffer vertexBuffer,
        uint32_t vertexCount,
        const std::vector<VkDescriptorSet>& descriptorSets
        ) {


        for (size_t i = 0; i < commandBuffers_.size(); ++i) {
            VkCommandBuffer cmdBuffer = commandBuffers_[i];

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;


            if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("❌ Failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = framebuffers[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = extent;

            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

            // Bind the Descriptor Sets
            vkCmdBindDescriptorSets(
                cmdBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                layout,
                0, 1,
                &descriptorSets[i], // one per frame
                0,
                nullptr
                );

            // Bind the Vertex Buffer
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);

            // Draw based on actual vertex count
            vkCmdDraw(cmdBuffer, vertexCount, 1, 0, 0);


            vkCmdEndRenderPass(cmdBuffer);

            if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
                throw std::runtime_error("❌ Failed to record command buffer!");
            }
        }

       // std::cout << "✅ Vulkan command buffers recorded.\n";

    }

}
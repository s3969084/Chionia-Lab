
#include "vulkan_command_buffers.hpp"

#include <array>
#include <stdexcept>

namespace chionia {

    void VulkanCommandBuffers::allocate(
        VkDevice logicalDevice,
        VkCommandPool commandPool,
        uint32_t count
    ) {
        if (count == 0) {
            throw std::invalid_argument(
                "Cannot allocate zero Vulkan command buffers."
            );
        }

        if (!commandBuffers_.empty()) {
            throw std::logic_error(
                "Vulkan command buffers have already been allocated."
            );
        }

        commandBuffers_.resize(count);

        VkCommandBufferAllocateInfo allocateInfo{};

        allocateInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        allocateInfo.commandPool = commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = count;

        if (
            vkAllocateCommandBuffers(
                logicalDevice,
                &allocateInfo,
                commandBuffers_.data()
            ) != VK_SUCCESS
        ) {
            commandBuffers_.clear();

            throw std::runtime_error(
                "Failed to allocate Vulkan command buffers."
            );
        }
    }

    void VulkanCommandBuffers::free(
        VkDevice logicalDevice,
        VkCommandPool commandPool
    ) {
        if (commandBuffers_.empty()) {
            return;
        }

        vkFreeCommandBuffers(
            logicalDevice,
            commandPool,
            static_cast<uint32_t>(commandBuffers_.size()),
            commandBuffers_.data()
        );

        commandBuffers_.clear();
    }

    void VulkanCommandBuffers::record(
        VkRenderPass renderPass,
        const std::vector<VkFramebuffer>& framebuffers,
        VkExtent2D extent,
        VkPipeline pipeline,
        VkPipelineLayout layout,
        VkBuffer vertexBuffer,
        uint32_t vertexCount,
        const std::vector<VkDescriptorSet>& descriptorSets,
        bool skipDrawing
    ) {
        if (
            framebuffers.size() != commandBuffers_.size() ||
            descriptorSets.size() != commandBuffers_.size()
        ) {
            throw std::invalid_argument(
                "Framebuffer, descriptor set and command buffer counts must match."
            );
        }

        const bool drawParticles =
            !skipDrawing &&
            pipeline != VK_NULL_HANDLE &&
            layout != VK_NULL_HANDLE &&
            vertexBuffer != VK_NULL_HANDLE &&
            vertexCount > 0;

        for (
            size_t i = 0;
            i < commandBuffers_.size();
            ++i
        ) {
            VkCommandBuffer commandBuffer =
                commandBuffers_[i];

            VkCommandBufferBeginInfo beginInfo{};

            beginInfo.sType =
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            beginInfo.flags =
                VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

            if (
                vkBeginCommandBuffer(
                    commandBuffer,
                    &beginInfo
                ) != VK_SUCCESS
            ) {
                throw std::runtime_error(
                    "Failed to begin recording Vulkan command buffer."
                );
            }

            std::array<VkClearValue, 1> clearValues{};

            clearValues[0].color = {
                {0.0f, 0.0f, 0.0f, 1.0f}
            };

            VkRenderPassBeginInfo renderPassInfo{};

            renderPassInfo.sType =
                VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = framebuffers[i];

            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = extent;

            renderPassInfo.clearValueCount =
                static_cast<uint32_t>(clearValues.size());

            renderPassInfo.pClearValues =
                clearValues.data();

            vkCmdBeginRenderPass(
                commandBuffer,
                &renderPassInfo,
                VK_SUBPASS_CONTENTS_INLINE
            );

            // Draw the existing particle cloud.
            if (drawParticles) {
                vkCmdBindPipeline(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline
                );

                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    layout,
                    0,
                    1,
                    &descriptorSets[i],
                    0,
                    nullptr
                );

                VkDeviceSize offset = 0;

                vkCmdBindVertexBuffers(
                    commandBuffer,
                    0,
                    1,
                    &vertexBuffer,
                    &offset
                );

                vkCmdDraw(
                    commandBuffer,
                    vertexCount,
                    1,
                    0,
                    0
                );
            }

            vkCmdEndRenderPass(commandBuffer);

            if (
                vkEndCommandBuffer(commandBuffer) != VK_SUCCESS
            ) {
                throw std::runtime_error(
                    "Failed to record Vulkan command buffer."
                );
            }
        }
    }

}

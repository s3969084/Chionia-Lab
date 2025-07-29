#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "../../../graphics/resources/ubo.hpp"
#include "buffer.hpp"

namespace chionia {

    class UniformBuffer {
    public:
        void create(
            VkDevice device,
            const VkPhysicalDeviceMemoryProperties& memProps,
            size_t swapchainImageCount
        );

        void createDescriptorSets(
            VkDevice device,
            VkDescriptorPool pool,
            VkDescriptorSetLayout layout,
            size_t swapchainImageCount
        );

        void update(
            VkDevice device,
            size_t imageIndex,
            const UniformBufferObject& ubo
        );

        void destroy(VkDevice device);

        const std::vector<VkDescriptorSet>& getDescriptorSets() const { return descriptorSets_; }

    private:
        std::vector<std::unique_ptr<Buffer>> buffers_;
        std::vector<VkDescriptorSet> descriptorSets_;
        VkDeviceSize bufferSize_ = sizeof(UniformBufferObject);
    };
}

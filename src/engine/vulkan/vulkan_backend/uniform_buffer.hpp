#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

namespace chionia {

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };

    class UniformBuffer {
    public:
        void create(VkDevice logicalDevice, VkPhysicalDeviceMemoryProperties memoryProperties, size_t swapchainImageCount);
        void destroy(VkDevice logicalDevice);
        void update(VkDevice logicalDevice, size_t currentFrame, const UniformBufferObject& ubo);

        void createDescriptorSets(VkDevice logicalDevice, VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout, size_t swapchainImageCount);

        const std::vector<VkDescriptorSet>& getDescriptorSets() const {
            return descriptorSets_;
        }

        const std::vector<VkBuffer>& getBuffers() const { return buffers_; }
        const std::vector<VkDeviceMemory>& getMemories() const { return memories_; }

    private:
        std::vector<VkBuffer> buffers_;
        std::vector<VkDeviceMemory> memories_;

        std::vector<VkDescriptorSet> descriptorSets_;
    };
}
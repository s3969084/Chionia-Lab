#include "vulkan_uniform_buffer.hpp"
#include <stdexcept>
#include <cstring>

namespace chionia {

    void UniformBuffer::create(
        VkDevice logicalDevice,
        const VkPhysicalDeviceMemoryProperties& memoryProperties,
        size_t swapchainImageCount) {
        buffers_.resize(swapchainImageCount);
        for (size_t i = 0; i < swapchainImageCount; ++i) {
            buffers_[i] = std::make_unique<Buffer>(
                logicalDevice,
                memoryProperties,
                bufferSize_,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
    }

    void UniformBuffer::createDescriptorSets(
        VkDevice logicalDevice,
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout layout,
        size_t swapchainImageCount) {

        descriptorSets_.resize(swapchainImageCount);
        std::vector<VkDescriptorSetLayout> layouts(swapchainImageCount, layout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchainImageCount);
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets_.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets for UBO.");
        }

        for (size_t i = 0; i < swapchainImageCount; ++i) {
            VkDescriptorBufferInfo bufferInfo = buffers_[i]->descriptorInfo();

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets_[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);

        }
    }

    void UniformBuffer::update(
        VkDevice logicalDevice,
        size_t imageIndex,
        const UniformBufferObject& ubo) {
        buffers_[imageIndex]->writeBuffer(&ubo, bufferSize_);
    }

    void UniformBuffer::destroy(VkDevice logicalDevice) {
        for (auto& buffer : buffers_) {
            if (buffer) buffer->destroy();
        }
        buffers_.clear();
        descriptorSets_.clear();
    }



}
#include "uniform_buffer.hpp"
#include <stdexcept>
#include <iostream>

namespace chionia {

    namespace {
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, const VkPhysicalDeviceMemoryProperties& memoryProperties) {
            for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
                if ((typeFilter & (1 << i)) &&
                    (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                    return i;
                }
            }
            throw std::runtime_error("❌ Failed to find suitable memory type for UBO!");
        }
    }

    void UniformBuffer::createDescriptorSets(
        VkDevice logicalDevice,
        VkDescriptorPool pool,
        VkDescriptorSetLayout layout,
        size_t swapchainImageCount
        ) {
        std::vector<VkDescriptorSetLayout> layouts(swapchainImageCount, layout);

        VkDescriptorSetAllocateInfo allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchainImageCount);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets_.resize(swapchainImageCount);
        if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets_.data()) != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to allocate descriptor sets for UBO!");
        }

        for (size_t i = 0; i < swapchainImageCount; ++i) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = buffers_[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets_[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(logicalDevice,1,  &descriptorWrite, 0, nullptr);


        }
    }

    void UniformBuffer::create(VkDevice logicalDevice, VkPhysicalDeviceMemoryProperties memoryProperties, size_t swapchainImageCount) {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        buffers_.resize(swapchainImageCount);
        memories_.resize(swapchainImageCount);

        for (size_t i = 0; i < swapchainImageCount; ++i) {
            // Create buffer
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = bufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffers_[i]) != VK_SUCCESS) {
                throw std::runtime_error("❌ Failed to create uniform buffer!");
            }

            // Allocate memory
            VkMemoryRequirements memoryRequirements{};
            vkGetBufferMemoryRequirements(logicalDevice, buffers_[i], &memoryRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memoryRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(
                memoryRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                memoryProperties
                );

            if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &memories_[i]) != VK_SUCCESS) {
                throw std::runtime_error("❌ Failed to allocate uniform buffer memory!");
            }

            vkBindBufferMemory(logicalDevice, buffers_[i], memories_[i], 0);
        }
    }

    void UniformBuffer::update(VkDevice logicalDevice, size_t currentFrame, const UniformBufferObject& ubo) {
        void* data;
        vkMapMemory(logicalDevice, memories_[currentFrame], 0, sizeof(ubo), 0, &data);
        std::memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(logicalDevice, memories_[currentFrame]);
    }



    void UniformBuffer::destroy(VkDevice logicalDevice) {
        for (size_t i = 0; i < buffers_.size(); ++i) {
            vkDestroyBuffer(logicalDevice, buffers_[i], nullptr);
            vkFreeMemory(logicalDevice, memories_[i], nullptr);
        }
    }
}
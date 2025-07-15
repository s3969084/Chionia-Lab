#include "vulkan_backend/vulkan_vertex_buffer.hpp"
#include <stdexcept>
#include <iostream>

namespace chionia {

    VulkanVertexBuffer::VulkanVertexBuffer() = default;

    VulkanVertexBuffer::~VulkanVertexBuffer() = default;

    void VulkanVertexBuffer::init(VkPhysicalDevice physicalDevice, VkDevice logicalDevice) {
        // Hardcoded test Points (Phase 1)
        vertices_ = {
            { glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },  // Red
            { glm::vec3( 0.5f,  0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) }   // Green
        };
        vertexCount_ = static_cast<uint32_t>(vertices_.size());
        create(physicalDevice, logicalDevice);
    }

    void VulkanVertexBuffer::destroy(VkDevice logicalDevice) {
        if (vertexBuffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(logicalDevice, vertexBuffer_, nullptr);
        }
        if (vertexMemory_ != VK_NULL_HANDLE) {
            vkFreeMemory(logicalDevice, vertexMemory_, nullptr);
        }
        vertexBuffer_ = VK_NULL_HANDLE;
        vertexMemory_ = VK_NULL_HANDLE;
    }

    void VulkanVertexBuffer::bind(VkCommandBuffer commandBuffer) const {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer_, &offset);
    }

    uint32_t VulkanVertexBuffer::getVertexCount() const {
        return vertexCount_;
    }

    void VulkanVertexBuffer::create(VkPhysicalDevice physicalDevice, VkDevice logicalDevice) {
        VkDeviceSize bufferSize = sizeof(vertices_[0]) * vertices_.size();

        // 1. Create the buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &vertexBuffer_) != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to create vertex buffer!");
        }

        // 2. Allocate memory
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logicalDevice, vertexBuffer_, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(
            physicalDevice,
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );

        if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &vertexMemory_) != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to allocate vertex buffer memory!");
        }

        // 3. Map and copy data
        void* data;
        vkMapMemory(logicalDevice, vertexMemory_, 0, bufferSize, 0, &data);
        std::memcpy(data, vertices_.data(), (size_t)bufferSize);
        vkUnmapMemory(logicalDevice, vertexMemory_);

        // 4. Bind memory to buffer
        vkBindBufferMemory(logicalDevice, vertexBuffer_, vertexMemory_, 0);
    }

    uint32_t VulkanVertexBuffer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
                }
        }

        throw std::runtime_error("❌ Failed to find suitable memory type!");
    }





}

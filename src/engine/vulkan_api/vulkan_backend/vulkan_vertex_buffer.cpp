#include "vulkan_vertex_buffer.hpp"
#include <stdexcept>
#include <cstring>
#include <iostream>

namespace chionia {

    void VulkanVertexBuffer::create(VkDevice logicalDevice, VkPhysicalDeviceMemoryProperties memProperties, const std::vector<Vertex>& vertices) {

        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // Create the vertex buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer_) != VK_SUCCESS) {
           throw std::runtime_error("❌ Failed to create vertex buffer.");
        }

        // Get memory requirements
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logicalDevice, buffer_, &memRequirements);

        // Allocate memory
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            memProperties);

        if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to allocate vertex buffer memory.");
        }

        // Bind memory
        vkBindBufferMemory(logicalDevice, buffer_, memory_, 0);

        // Copy vertex data
        void* data = nullptr;
        vkMapMemory(logicalDevice, memory_, 0, bufferSize, 0, &data);
        std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(logicalDevice, memory_);

        //std::cout << "✅ Vertex buffer created successfully with " << vertices.size() << " vertices.\n";
    }

    void VulkanVertexBuffer::destroy(VkDevice logicalDevice) {
        if (memory_ != VK_NULL_HANDLE) {
            vkFreeMemory(logicalDevice, memory_, nullptr);
            memory_ = VK_NULL_HANDLE;
        }

        if (buffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(logicalDevice, buffer_, nullptr);
            buffer_ = VK_NULL_HANDLE;
        }
    }

    uint32_t VulkanVertexBuffer::findMemoryType(
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties,
        const VkPhysicalDeviceMemoryProperties& memProperties) {

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("❌ Failed to find suitable memory type for vertex buffer.");

    }

    VkVertexInputBindingDescription VulkanVertexBuffer::getBindingDescription() const {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;

    }

    std::array<VkVertexInputAttributeDescription, 2> VulkanVertexBuffer::getAttributeDescriptions() const {
        std::array<VkVertexInputAttributeDescription, 2> attributesDescription{};
        attributesDescription[0].binding = 0;
        attributesDescription[0].location = 0;
        attributesDescription[0].format = VK_FORMAT_R32_UINT;
        attributesDescription[0].offset = offsetof(Vertex, id);

        attributesDescription[1].binding = 0;
        attributesDescription[1].location = 1;
        attributesDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributesDescription[1].offset = offsetof(Vertex, position);

        return attributesDescription;
    }



}
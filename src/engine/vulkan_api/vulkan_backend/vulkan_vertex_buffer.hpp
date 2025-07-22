// src/engine/vulkan/vulkan_backend.hpp

#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

#include "engine/buffer_management/vertex_buffer.hpp"

namespace chionia {

    class VulkanVertexBuffer {
    public:
        void create(VkDevice logicalDevice, VkPhysicalDeviceMemoryProperties memProperties, const std::vector<Vertex>& vertices);
        void destroy(VkDevice logicalDevice);

        VkVertexInputBindingDescription  getBindingDescription() const;
        std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() const;

        void updateData(VkDevice logicalDevice, VkPhysicalDeviceMemoryProperties memProperties, const std::vector<Vertex>& newVertices) {

            // Destroy old buffer and memory
            destroy(logicalDevice);
            // Save new vertices
            vertices_ = newVertices;
            //Re-create the buffer with new data
            create(logicalDevice, memProperties, vertices_);
        }

        VkBuffer getBuffer() const { return buffer_;}
        uint32_t getSize() const { return static_cast<uint32_t>(vertices_.size()); }



    private:
        VkBuffer buffer_{VK_NULL_HANDLE};
        VkDeviceMemory memory_{VK_NULL_HANDLE};
        std::vector<Vertex> vertices_;

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, const VkPhysicalDeviceMemoryProperties& memProperties);

    };


}
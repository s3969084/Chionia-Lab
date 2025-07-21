#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

namespace chionia {

    struct Vertex {
        uint32_t id;
        glm::vec3 position;

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription binding{};
            binding.binding = 0;
            binding.stride = sizeof(Vertex);
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return binding;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attributes{};

            attributes[0].binding = 0;
            attributes[0].location = 0;
            attributes[0].format = VK_FORMAT_R32_UINT;
            attributes[0].offset = offsetof(Vertex, id);

            attributes[1].binding = 0;
            attributes[1].location = 1;
            attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributes[1].offset = offsetof(Vertex, position);

            return attributes;


        }
    };

    class VulkanVertexBuffer {
    public:
        void create(VkDevice logicalDevice, VkPhysicalDeviceMemoryProperties memProperties, const std::vector<Vertex>& vertices);
        void destroy(VkDevice logicalDevice);

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

        VkVertexInputBindingDescription getBindingDescription() const {
            return Vertex::getBindingDescription();
        }

        std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() const {
            return Vertex::getAttributeDescriptions();
        }

    private:
        VkBuffer buffer_{VK_NULL_HANDLE};
        VkDeviceMemory memory_{VK_NULL_HANDLE};
        std::vector<Vertex> vertices_;

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, const VkPhysicalDeviceMemoryProperties& memProperties);

    };


}
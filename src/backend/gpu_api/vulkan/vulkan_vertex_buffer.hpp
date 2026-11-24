
#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include "buffer.hpp"
#include "../../../graphics/resources/vertex.hpp"

namespace chionia {

    class VulkanVertexBuffer {
    public:
        VulkanVertexBuffer() = default;
        ~VulkanVertexBuffer();

        void create(
            VkDevice logicalDevice,
            const VkPhysicalDeviceMemoryProperties& memoryProperties,
            VkCommandPool commandPool,
            VkQueue graphicsQueue,
            const std::vector<Vertex>& vertices
        );

        void update(
            VkDevice logicalDevice,
            const VkPhysicalDeviceMemoryProperties& memoryProperties,
            VkCommandPool commandPool,
            VkQueue graphicsQueue,
            const std::vector<Vertex>& vertices
        );

        void destroy();

        VkBuffer getBuffer() const;

        std::size_t getVertexCount() const;

        VkDeviceSize getCapacityBytes() const;

        static VkVertexInputBindingDescription getBindingDescription();

        static std::array<VkVertexInputAttributeDescription, 3>
        getAttributeDescriptions();

    private:
        std::unique_ptr<Buffer> deviceBuffer_;

        // Number of vertices currently containing valid data.
        std::size_t vertexCount_ = 0;
    };

}

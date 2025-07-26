#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "engine/buffer_management/buffer.hpp"  // Uses Buffer abstraction
#include "engine/buffer_management/vertex.hpp"

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

        void destroy();

        VkBuffer getBuffer() const;
        size_t getVertexCount() const;

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();

    private:
        std::unique_ptr<Buffer> deviceBuffer_;
        size_t vertexCount_ = 0;
        VkDevice device_ = VK_NULL_HANDLE;
    };

}

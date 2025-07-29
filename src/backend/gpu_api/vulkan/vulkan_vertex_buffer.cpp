#include "vulkan_vertex_buffer.hpp"
#include <stdexcept>
#include <memory>

namespace chionia {

    VulkanVertexBuffer::~VulkanVertexBuffer() {
        destroy();
    }

    void VulkanVertexBuffer::create(
        VkDevice logicalDevice,
        const VkPhysicalDeviceMemoryProperties& memoryProperties,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        const std::vector<Vertex>& vertices
    ) {
        device_ = logicalDevice;
        vertexCount_ = vertices.size();
        VkDeviceSize bufferSize = sizeof(Vertex) * vertexCount_;

        // 1. Create staging buffer (CPU-visible)
        Buffer stagingBuffer(
            logicalDevice,
            memoryProperties,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        stagingBuffer.map();
        stagingBuffer.writeBuffer(vertices.data(), bufferSize);
        stagingBuffer.unmap();

        // 2. Create device-local GPU buffer
        deviceBuffer_ = std::make_unique<Buffer>(
            logicalDevice,
            memoryProperties,
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        // 3. Copy from staging to device-local
        stagingBuffer.copyBuffer(*deviceBuffer_, commandPool, graphicsQueue);
    }

    void VulkanVertexBuffer::destroy() {
        if (deviceBuffer_) {
            deviceBuffer_->destroy();
            deviceBuffer_.reset();
        }
    }

    VkBuffer VulkanVertexBuffer::getBuffer() const {
        return deviceBuffer_ ? deviceBuffer_->get() : VK_NULL_HANDLE;
    }

    size_t VulkanVertexBuffer::getVertexCount() const {
        return vertexCount_;
    }

    VkVertexInputBindingDescription VulkanVertexBuffer::getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    std::array<VkVertexInputAttributeDescription, 2> VulkanVertexBuffer::getAttributeDescriptions() {
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

}

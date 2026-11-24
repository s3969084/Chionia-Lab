
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
        if (vertices.empty()) {
            throw std::invalid_argument(
                "Cannot create a vertex buffer with no vertices."
            );
        }

        if (deviceBuffer_) {
            throw std::logic_error(
                "Vertex buffer has already been created."
            );
        }

        const VkDeviceSize bufferSize =
            sizeof(Vertex) * vertices.size();

        // 1. Create CPU-visible staging buffer.
        Buffer stagingBuffer(
            logicalDevice,
            memoryProperties,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        stagingBuffer.writeBuffer(
            vertices.data(),
            bufferSize
        );

        // 2. Create persistent device-local GPU buffer.
        deviceBuffer_ = std::make_unique<Buffer>(
            logicalDevice,
            memoryProperties,
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        // 3. Upload initial vertex data.
        stagingBuffer.copyBuffer(
            *deviceBuffer_,
            commandPool,
            graphicsQueue
        );

        vertexCount_ = vertices.size();
    }

    void VulkanVertexBuffer::update(
        VkDevice logicalDevice,
        const VkPhysicalDeviceMemoryProperties& memoryProperties,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        const std::vector<Vertex>& vertices
    ) {
        if (vertices.empty()) {
            throw std::invalid_argument(
                "Cannot update vertex buffer with no vertices."
            );
        }

        // If the GPU buffer does not exist yet, create it.
        if (!deviceBuffer_) {
            create(
                logicalDevice,
                memoryProperties,
                commandPool,
                graphicsQueue,
                vertices
            );
            return;
        }

        const VkDeviceSize requiredSize =
            sizeof(Vertex) * vertices.size();

        // Do not overwrite an allocation that is too small.
        if (requiredSize > deviceBuffer_->getSize()) {
            throw std::runtime_error(
                "Vertex update exceeds allocated GPU buffer capacity."
            );
        }

        // 1. Create temporary CPU-visible staging buffer.
        Buffer stagingBuffer(
            logicalDevice,
            memoryProperties,
            requiredSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        // 2. Write the new vertex data into staging memory.
        stagingBuffer.writeBuffer(
            vertices.data(),
            requiredSize
        );

        // 3. Copy into the existing GPU buffer.
        // The destination VkBuffer handle remains unchanged.
        stagingBuffer.copyBuffer(
            *deviceBuffer_,
            commandPool,
            graphicsQueue
        );

        vertexCount_ = vertices.size();
    }

    void VulkanVertexBuffer::destroy() {
        if (deviceBuffer_) {
            deviceBuffer_->destroy();
            deviceBuffer_.reset();
        }

        vertexCount_ = 0;
    }

    VkBuffer VulkanVertexBuffer::getBuffer() const {
        return deviceBuffer_
            ? deviceBuffer_->get()
            : VK_NULL_HANDLE;
    }

    size_t VulkanVertexBuffer::getVertexCount() const {
        return vertexCount_;
    }

    VkVertexInputBindingDescription
    VulkanVertexBuffer::getBindingDescription() {

        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    std::array<VkVertexInputAttributeDescription, 3>
    VulkanVertexBuffer::getAttributeDescriptions() {

        std::array<VkVertexInputAttributeDescription, 3> attributes{};

        // Attribute 0: Particle ID.
        attributes[0].binding = 0;
        attributes[0].location = 0;
        attributes[0].format = VK_FORMAT_R32_UINT;
        attributes[0].offset = offsetof(Vertex, id);

        // Attribute 1: Starting position.
        attributes[1].binding = 0;
        attributes[1].location = 1;
        attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[1].offset = offsetof(Vertex, position);

        // Attribute 2: Destination position.
        attributes[2].binding = 0;
        attributes[2].location = 2;
        attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[2].offset = offsetof(Vertex, targetPosition);

        return attributes;
    }

}

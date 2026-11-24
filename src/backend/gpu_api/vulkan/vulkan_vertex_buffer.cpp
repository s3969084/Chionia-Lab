
#include "vulkan_vertex_buffer.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>

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

        if (
            vertices.size() >
            std::numeric_limits<VkDeviceSize>::max() / sizeof(Vertex)
        ) {
            throw std::overflow_error(
                "Vertex buffer size exceeds Vulkan device limits."
            );
        }

        const VkDeviceSize bufferSize =
            sizeof(Vertex) * vertices.size();

        // Create CPU-visible staging buffer.
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

        // Create a temporary GPU allocation.
        auto newDeviceBuffer = std::make_unique<Buffer>(
            logicalDevice,
            memoryProperties,
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        // Upload before making the new allocation active.
        stagingBuffer.copyBuffer(
            *newDeviceBuffer,
            commandPool,
            graphicsQueue
        );

        deviceBuffer_ = std::move(newDeviceBuffer);
        vertexCount_ = vertices.size();
    }

    void VulkanVertexBuffer::update(
        VkDevice logicalDevice,
        const VkPhysicalDeviceMemoryProperties& memoryProperties,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        const std::vector<Vertex>& vertices
    ) {
        // An empty update means there are no active vertices.
        // Retain the allocation for possible future reuse.
        if (vertices.empty()) {
            vertexCount_ = 0;
            return;
        }

        if (
            vertices.size() >
            std::numeric_limits<VkDeviceSize>::max() / sizeof(Vertex)
        ) {
            throw std::overflow_error(
                "Vertex update size exceeds Vulkan device limits."
            );
        }

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

        const VkDeviceSize currentCapacity =
            deviceBuffer_->getSize();

        // Determine whether the current allocation is large enough.
        if (requiredSize > currentCapacity) {

            // Grow geometrically to reduce repeated reallocations.
            VkDeviceSize newCapacity = currentCapacity;

            if (
                newCapacity >
                std::numeric_limits<VkDeviceSize>::max() / 2
            ) {
                newCapacity = requiredSize;
            } else {
                newCapacity = std::max(
                    requiredSize,
                    newCapacity * 2
                );
            }

            Buffer stagingBuffer(
                logicalDevice,
                memoryProperties,
                requiredSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );

            stagingBuffer.writeBuffer(
                vertices.data(),
                requiredSize
            );

            // Create a replacement allocation without destroying
            // the currently active GPU buffer.
            auto replacementBuffer = std::make_unique<Buffer>(
                logicalDevice,
                memoryProperties,
                newCapacity,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );

            // Upload the new data before replacing the old buffer.
            stagingBuffer.copyBuffer(
                *replacementBuffer,
                commandPool,
                graphicsQueue
            );

            // The old allocation is released when its unique_ptr
            // is replaced. The caller must have waited for GPU use.
            deviceBuffer_ = std::move(replacementBuffer);

        } else {

            // Existing allocation has sufficient capacity.
            Buffer stagingBuffer(
                logicalDevice,
                memoryProperties,
                requiredSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );

            stagingBuffer.writeBuffer(
                vertices.data(),
                requiredSize
            );

            stagingBuffer.copyBuffer(
                *deviceBuffer_,
                commandPool,
                graphicsQueue
            );
        }

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

    std::size_t VulkanVertexBuffer::getVertexCount() const {
        return vertexCount_;
    }

    VkDeviceSize VulkanVertexBuffer::getCapacityBytes() const {
        return deviceBuffer_
            ? deviceBuffer_->getSize()
            : 0;
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

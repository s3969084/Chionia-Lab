
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

    std::array<VkVertexInputAttributeDescription, 14>
    VulkanVertexBuffer::getAttributeDescriptions() {

        std::array<VkVertexInputAttributeDescription, 14> attributes{};

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

        // Attribute 3: Per-point colour.
        attributes[3].binding = 0;
        attributes[3].location = 3;
        attributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[3].offset = offsetof(Vertex, color);

        // Attribute 13: Destination per-point colour for morph transitions.
        attributes[13].binding = 0;
        attributes[13].location = 13;
        attributes[13].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[13].offset = offsetof(Vertex, targetColor);

        attributes[4].binding = 0;
        attributes[4].location = 4;
        attributes[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[4].offset = offsetof(Vertex, orbitMotion) +
            offsetof(PointOrbitMotion, center);

        attributes[5].binding = 0;
        attributes[5].location = 5;
        attributes[5].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[5].offset = offsetof(Vertex, orbitMotion) +
            offsetof(PointOrbitMotion, axis);

        attributes[6].binding = 0;
        attributes[6].location = 6;
        attributes[6].format = VK_FORMAT_R32_SFLOAT;
        attributes[6].offset = offsetof(Vertex, orbitMotion) +
            offsetof(PointOrbitMotion, angularSpeed);

        attributes[7].binding = 0;
        attributes[7].location = 7;
        attributes[7].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[7].offset = offsetof(Vertex, orbitMotion) +
            offsetof(PointOrbitMotion, secondaryCenter);

        attributes[8].binding = 0;
        attributes[8].location = 8;
        attributes[8].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[8].offset = offsetof(Vertex, orbitMotion) +
            offsetof(PointOrbitMotion, secondaryAxis);

        attributes[9].binding = 0;
        attributes[9].location = 9;
        attributes[9].format = VK_FORMAT_R32_SFLOAT;
        attributes[9].offset = offsetof(Vertex, orbitMotion) +
            offsetof(PointOrbitMotion, secondaryAngularSpeed);

        attributes[10].binding = 0;
        attributes[10].location = 10;
        attributes[10].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[10].offset = offsetof(Vertex, orbitMotion) +
            offsetof(PointOrbitMotion, spinCenter);

        attributes[11].binding = 0;
        attributes[11].location = 11;
        attributes[11].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[11].offset = offsetof(Vertex, orbitMotion) +
            offsetof(PointOrbitMotion, spinAxis);

        attributes[12].binding = 0;
        attributes[12].location = 12;
        attributes[12].format = VK_FORMAT_R32_SFLOAT;
        attributes[12].offset = offsetof(Vertex, orbitMotion) +
            offsetof(PointOrbitMotion, spinAngularSpeed);

        return attributes;
    }

}


#include "vulkan_point_cloud_renderer.hpp"

#include "graphics/conversion/point_vertex_converter.hpp"

#include <stdexcept>

namespace chionia {

    void VulkanPointCloudRenderer::initialize(
        VkDevice logicalDevice,
        const VkPhysicalDeviceMemoryProperties& memoryProperties,
        VkCommandPool commandPool,
        VkQueue graphicsQueue
    ) {

        if (initialized_) {
            throw std::logic_error(
                "VulkanPointCloudRenderer is already initialized."
            );
        }

        if (
            logicalDevice == VK_NULL_HANDLE ||
            commandPool == VK_NULL_HANDLE ||
            graphicsQueue == VK_NULL_HANDLE
        ) {
            throw std::invalid_argument(
                "VulkanPointCloudRenderer received invalid Vulkan resources."
            );
        }

        logicalDevice_ = logicalDevice;
        memoryProperties_ = memoryProperties;
        commandPool_ = commandPool;
        graphicsQueue_ = graphicsQueue;

        initialized_ = true;
    }


    void VulkanPointCloudRenderer::synchronize(
        const PointCloud& pointCloud
    ) {

        if (!initialized_) {
            throw std::logic_error(
                "VulkanPointCloudRenderer must be initialized before synchronization."
            );
        }

        // Convert the renderer-independent core resource
        // into the current Vulkan vertex representation.
        const std::vector<Vertex> vertices =
            PointVertexConverter::convertPointCloudToVertices(
                pointCloud
            );

        // Update handles both initial allocation and
        // subsequent buffer updates.
        vertexBuffer_.update(
            logicalDevice_,
            memoryProperties_,
            commandPool_,
            graphicsQueue_,
            vertices
        );
    }


    void VulkanPointCloudRenderer::release() {

        if (!initialized_) {
            return;
        }

        vertexBuffer_.destroy();

        logicalDevice_ = VK_NULL_HANDLE;
        commandPool_ = VK_NULL_HANDLE;
        graphicsQueue_ = VK_NULL_HANDLE;

        initialized_ = false;
    }


    std::size_t
    VulkanPointCloudRenderer::getResidentPointCount() const {

        return vertexBuffer_.getVertexCount();
    }


    VkBuffer VulkanPointCloudRenderer::getBuffer() const {

        return vertexBuffer_.getBuffer();
    }


    VkDeviceSize
    VulkanPointCloudRenderer::getCapacityBytes() const {

        return vertexBuffer_.getCapacityBytes();
    }


    VkVertexInputBindingDescription
    VulkanPointCloudRenderer::getBindingDescription() {

        return VulkanVertexBuffer::getBindingDescription();
    }

}

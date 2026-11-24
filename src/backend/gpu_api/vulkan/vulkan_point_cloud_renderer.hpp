
#pragma once

#include "graphics/interface/point_cloud_renderer.hpp"
#include "vulkan_vertex_buffer.hpp"

#include <vulkan/vulkan.h>

#include <cstddef>

namespace chionia {

    class VulkanPointCloudRenderer final : public PointCloudRenderer {
    public:
        VulkanPointCloudRenderer() = default;
        ~VulkanPointCloudRenderer() override = default;

        // Initialise the renderer with the Vulkan resources
        // supplied by VulkanBackend.
        void initialize(
            VkDevice logicalDevice,
            const VkPhysicalDeviceMemoryProperties& memoryProperties,
            VkCommandPool commandPool,
            VkQueue graphicsQueue
        );

        // Convert the core PointCloud into GPU vertices
        // and synchronize the Vulkan buffer.
        void synchronize(
            const PointCloud& pointCloud
        ) override;

        // Release GPU resources.
        void release() override;

        // Number of points currently resident on the GPU.
        std::size_t getResidentPointCount() const override;

        // Vulkan-specific accessors used by the backend.
        VkBuffer getBuffer() const;

        VkDeviceSize getCapacityBytes() const;

        static VkVertexInputBindingDescription
        getBindingDescription();

        static auto getAttributeDescriptions() {
            return VulkanVertexBuffer::getAttributeDescriptions();
        }

    private:
        VulkanVertexBuffer vertexBuffer_;

        VkDevice logicalDevice_ = VK_NULL_HANDLE;

        VkPhysicalDeviceMemoryProperties memoryProperties_{};

        VkCommandPool commandPool_ = VK_NULL_HANDLE;

        VkQueue graphicsQueue_ = VK_NULL_HANDLE;

        bool initialized_ = false;
    };

}

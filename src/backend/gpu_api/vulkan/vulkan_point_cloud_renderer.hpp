
#pragma once

#include "graphics/interface/point_cloud_renderer.hpp"
#include "vulkan_vertex_buffer.hpp"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <vector>

namespace chionia {

    class VulkanPointCloudRenderer final : public PointCloudRenderer {
    public:
        VulkanPointCloudRenderer() = default;
        ~VulkanPointCloudRenderer() override = default;

        void initialize(
            VkDevice logicalDevice,
            const VkPhysicalDeviceMemoryProperties& memoryProperties,
            VkCommandPool commandPool,
            VkQueue graphicsQueue
        );

        void synchronize(
            const PointCloud& pointCloud
        ) override;

        // Transitional path for existing vertex-based animation.
        void synchronizeVertices(
            const std::vector<Vertex>& vertices
        );

        void release() override;

        std::size_t getResidentPointCount() const override;

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

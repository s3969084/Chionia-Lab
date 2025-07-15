#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

namespace chionia {

    class VulkanVertexBuffer {
    public:
        struct Point {
            glm::vec3 position;
            glm::vec3 color;
        };

        VulkanVertexBuffer();
        ~VulkanVertexBuffer();

        void init(VkPhysicalDevice physicalDevice, VkDevice logicalDevice);
        void destroy(VkDevice logicalDevice);

        void bind(VkCommandBuffer commandBuffer) const;
        uint32_t getVertexCount() const;

    private:
        void create(VkPhysicalDevice physicalDevice, VkDevice logicalDevice);
        uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

        std::vector<Point> vertices_;
        VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory_ = VK_NULL_HANDLE;

        uint32_t vertexCount_ = 0;

    private:


    };
}
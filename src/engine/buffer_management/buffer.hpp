#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace chionia {
    class Buffer {
    public:
        Buffer(
            VkDevice logicalDevice,
            const VkPhysicalDeviceMemoryProperties& memoryProperties,
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties
            );
        
        ~Buffer();

        void map();
        void unmap();
        void writeBuffer(const void* data, VkDeviceSize size);
        void copyBuffer(Buffer& dstBuffer, VkCommandPool commandPool, VkQueue queue);

        VkBuffer get() const { return buffer_;}
        VkDescriptorBufferInfo descriptorInfo() const;

        VkDeviceSize getSize() const { return size_; }

        void destroy();

    private:
        VkDevice logicalDevice_;
        VkBuffer buffer_ = VK_NULL_HANDLE;
        VkDeviceMemory memory_ = VK_NULL_HANDLE;
        VkDeviceSize size_{0};
        void* mappedData_ = nullptr;
        VkPhysicalDeviceMemoryProperties memoryProperties_{};

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        
    };
}
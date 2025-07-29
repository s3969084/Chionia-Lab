#include "buffer.hpp"
#include <stdexcept>
#include <cstring>

namespace chionia {

    Buffer::Buffer(
        VkDevice logicalDevice,
        const VkPhysicalDeviceMemoryProperties& memoryProperties,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties)  : logicalDevice_(logicalDevice), size_(size), memoryProperties_(memoryProperties)
    {

        // Create buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size_;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logicalDevice_, &bufferInfo, nullptr, &buffer_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer.");
        }

        // Get memory requirements
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(logicalDevice_, buffer_, &memoryRequirements);

        // Allocate the memory
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memoryRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(logicalDevice_, &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate buffer memory.");
        }

        // Bind memory to buffer
        vkBindBufferMemory(logicalDevice_, buffer_, memory_,0);
    }

    Buffer::~Buffer() {
        destroy();
    }

    void Buffer::map() {
        if (mappedData_ == nullptr) {
            vkMapMemory(logicalDevice_, memory_, 0, size_, 0, &mappedData_);

        }
    }

    void Buffer::unmap() {
        if (mappedData_) {
            vkUnmapMemory(logicalDevice_, memory_);
            mappedData_ = nullptr;
        }
    }

    void Buffer::writeBuffer(const void* data, VkDeviceSize size) {
        map();  // ensures mapping
        std::memcpy(mappedData_, data, static_cast<size_t>(size));
        unmap();  // optionally keep mapped if reused
    }

    void Buffer::copyBuffer(Buffer& dstBuffer, VkCommandPool commandPool, VkQueue queue) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer{};
        vkAllocateCommandBuffers(logicalDevice_, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size_;
        vkCmdCopyBuffer(commandBuffer, buffer_, dstBuffer.get(), 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(logicalDevice_, commandPool, 1, &commandBuffer);
    }

    VkDescriptorBufferInfo Buffer::descriptorInfo() const {
        VkDescriptorBufferInfo info{};
        info.buffer = buffer_;
        info.offset = 0;
        info.range = size_;
        return info;
    }

    void Buffer::destroy() {
        if (memory_ != VK_NULL_HANDLE) {
            vkFreeMemory(logicalDevice_, memory_, nullptr);
            memory_ = VK_NULL_HANDLE;
        }
        if (buffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(logicalDevice_, buffer_, nullptr);
            buffer_ = VK_NULL_HANDLE;
        }
    }

    uint32_t Buffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        for (uint32_t i = 0; i < memoryProperties_.memoryTypeCount; ++i) {
            if ((typeFilter & (1 << i)) && (memoryProperties_.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find a suitable memory type.");
    }







}
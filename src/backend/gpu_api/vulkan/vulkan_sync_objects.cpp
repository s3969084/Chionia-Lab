#include "vulkan_sync_objects.hpp"
#include <stdexcept>
#include <iostream>

namespace chionia {

    void VulkanSyncObjects::create(VkDevice logicalDevice, size_t maxFramesInFlight) {
        imageAvailableSemaphores_.resize(maxFramesInFlight);
        renderFinishedSemaphores_.resize(maxFramesInFlight);
        inFlightFences_.resize(maxFramesInFlight);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < maxFramesInFlight; ++i) {
            if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
                vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS ||
                vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create synchronization objects!");
                }
        }
        std::cout << "Vulkan synchronization objects created.\n";
    }

    void VulkanSyncObjects::destroy(VkDevice logicalDevice) {
        for (size_t i =0; i < imageAvailableSemaphores_.size(); ++i) {
            vkDestroySemaphore(logicalDevice, imageAvailableSemaphores_[i], nullptr);
            vkDestroySemaphore(logicalDevice, renderFinishedSemaphores_[i], nullptr);
            vkDestroyFence(logicalDevice, inFlightFences_[i], nullptr);
        }
        imageAvailableSemaphores_.clear();
        renderFinishedSemaphores_.clear();
        inFlightFences_.clear();
    }

    VkSemaphore VulkanSyncObjects::getImageAvailable(size_t frameIndex) const {
        return imageAvailableSemaphores_[frameIndex];
    }

    VkSemaphore VulkanSyncObjects::getRenderFinished(size_t frameIndex) const {
        return renderFinishedSemaphores_[frameIndex];
    }

    VkFence& VulkanSyncObjects::getInFlightFence(size_t frameIndex) {
        return inFlightFences_[frameIndex];
    }

    void VulkanSyncObjects::ResetFence(VkDevice device, size_t frameIndex) {
        vkWaitForFences(device, 1, &inFlightFences_[frameIndex], VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFences_[frameIndex]);
    }

    void VulkanSyncObjects::waitAllFrames(VkDevice logicalDevice) const{
        if (!inFlightFences_.empty()) {
            VkResult result = vkWaitForFences(
                logicalDevice,
                static_cast<uint32_t>(inFlightFences_.size()),
                inFlightFences_.data(),
                VK_TRUE,
                UINT64_MAX);

            if (result != VK_SUCCESS) {
                throw std::runtime_error("Failed to wait for all in-flight fences.");
            }

        }
    }


}
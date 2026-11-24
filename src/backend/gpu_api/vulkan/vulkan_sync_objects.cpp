#include "vulkan_sync_objects.hpp"

#include <stdexcept>
#include <iostream>

namespace chionia {

    void VulkanSyncObjects::create(
        VkDevice logicalDevice,
        size_t maxFramesInFlight,
        size_t swapchainImageCount
    ) {
        imageAvailableSemaphores_.resize(
            maxFramesInFlight,
            VK_NULL_HANDLE
        );

        renderFinishedSemaphores_.resize(
            swapchainImageCount,
            VK_NULL_HANDLE
        );

        inFlightFences_.resize(
            maxFramesInFlight,
            VK_NULL_HANDLE
        );

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType =
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType =
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        // Initially signalled so the first frame does not block.
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        // One image-available semaphore and fence per frame.
        for (size_t i = 0; i < maxFramesInFlight; ++i) {

            if (
                vkCreateSemaphore(
                    logicalDevice,
                    &semaphoreInfo,
                    nullptr,
                    &imageAvailableSemaphores_[i]
                ) != VK_SUCCESS
            ) {
                throw std::runtime_error(
                    "Failed to create image-available semaphore."
                );
            }

            if (
                vkCreateFence(
                    logicalDevice,
                    &fenceInfo,
                    nullptr,
                    &inFlightFences_[i]
                ) != VK_SUCCESS
            ) {
                throw std::runtime_error(
                    "Failed to create in-flight fence."
                );
            }
        }

        // One render-finished semaphore per swapchain image.
        for (size_t i = 0; i < swapchainImageCount; ++i) {

            if (
                vkCreateSemaphore(
                    logicalDevice,
                    &semaphoreInfo,
                    nullptr,
                    &renderFinishedSemaphores_[i]
                ) != VK_SUCCESS
            ) {
                throw std::runtime_error(
                    "Failed to create render-finished semaphore."
                );
            }
        }

        std::cout
            << "Vulkan synchronization objects created.\n";
    }


    void VulkanSyncObjects::destroy(VkDevice logicalDevice) {

        for (VkSemaphore semaphore : imageAvailableSemaphores_) {
            if (semaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(
                    logicalDevice,
                    semaphore,
                    nullptr
                );
            }
        }

        for (VkSemaphore semaphore : renderFinishedSemaphores_) {
            if (semaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(
                    logicalDevice,
                    semaphore,
                    nullptr
                );
            }
        }

        for (VkFence fence : inFlightFences_) {
            if (fence != VK_NULL_HANDLE) {
                vkDestroyFence(
                    logicalDevice,
                    fence,
                    nullptr
                );
            }
        }

        imageAvailableSemaphores_.clear();
        renderFinishedSemaphores_.clear();
        inFlightFences_.clear();
    }


    VkSemaphore VulkanSyncObjects::getImageAvailable(
        size_t frameIndex
    ) const {
        return imageAvailableSemaphores_.at(frameIndex);
    }


    VkSemaphore VulkanSyncObjects::getRenderFinished(
        size_t imageIndex
    ) const {
        return renderFinishedSemaphores_.at(imageIndex);
    }


    VkFence& VulkanSyncObjects::getInFlightFence(
        size_t frameIndex
    ) {
        return inFlightFences_.at(frameIndex);
    }


    void VulkanSyncObjects::waitForFence(
        VkDevice logicalDevice,
        size_t frameIndex
    ) const {

        VkResult result = vkWaitForFences(
            logicalDevice,
            1,
            &inFlightFences_.at(frameIndex),
            VK_TRUE,
            UINT64_MAX
        );

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to wait for frame fence."
            );
        }
    }


    void VulkanSyncObjects::resetFence(
        VkDevice logicalDevice,
        size_t frameIndex
    ) {

        VkResult result = vkResetFences(
            logicalDevice,
            1,
            &inFlightFences_.at(frameIndex)
        );

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to reset frame fence."
            );
        }
    }


    void VulkanSyncObjects::waitAllFrames(
        VkDevice logicalDevice
    ) const {

        if (inFlightFences_.empty()) {
            return;
        }

        VkResult result = vkWaitForFences(
            logicalDevice,
            static_cast<uint32_t>(inFlightFences_.size()),
            inFlightFences_.data(),
            VK_TRUE,
            UINT64_MAX
        );

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to wait for all in-flight fences."
            );
        }
    }

}
#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstddef>

namespace chionia {

    class VulkanSyncObjects {
    public:
        VulkanSyncObjects() = default;
        ~VulkanSyncObjects() = default;

        void create(
            VkDevice logicalDevice,
            size_t maxFramesInFlight,
            size_t swapchainImageCount
        );

        void destroy(VkDevice logicalDevice);

        void waitForFence(
            VkDevice logicalDevice,
            size_t frameIndex
        ) const;

        void resetFence(
            VkDevice logicalDevice,
            size_t frameIndex
        );

        void waitAllFrames(VkDevice logicalDevice) const;

        VkSemaphore getImageAvailable(size_t frameIndex) const;

        VkSemaphore getRenderFinished(size_t imageIndex) const;

        VkFence& getInFlightFence(size_t frameIndex);

    private:
        std::vector<VkSemaphore> imageAvailableSemaphores_;
        std::vector<VkSemaphore> renderFinishedSemaphores_;
        std::vector<VkFence> inFlightFences_;
    };

}
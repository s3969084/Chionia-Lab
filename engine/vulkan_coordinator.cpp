#include "vulkan_coordinator.hpp"
#include <stdexcept>
#include <iostream>

namespace chionia {

    void VulkanCoordinator::init() {
        instance_.create("Chionia Engine", true);
        debug_.setup(instance_.get());
        surface_.create(window_.getGLFWwindow(), instance_.get());
        physicalDevice_.pick(instance_.get(), surface_.get());
        logicalDevice_.create(physicalDevice_.get(), surface_.get());
        swapchain_.create(physicalDevice_.get(), logicalDevice_.get(), surface_.get(),
            logicalDevice_.getGraphicsQueueFamily(), logicalDevice_.getPresentQueueFamily(),
            window_.getGLFWwindow());

        renderPass_.create(logicalDevice_.get(), swapchain_.getImageFormat());
        framebuffers_.create(logicalDevice_.get(), renderPass_.get(),
            swapchain_.getImageViews(), swapchain_.getExtent());

        commandPool_.create(logicalDevice_.get(), physicalDevice_.findGraphicsQueueFamily(instance_.get()));

        renderer_.create(logicalDevice_.get(), swapchain_.getExtent(), renderPass_.get(),
            "shaders/point.vert.spv", "shaders/point.frag.spv");

        commandBuffers_.allocate(logicalDevice_.get(), commandPool_.get(), static_cast<uint32_t>(framebuffers_.getAll().size()));
        commandBuffers_.record(renderPass_.get(), framebuffers_.getAll(), swapchain_.getExtent(),
            renderer_.getPipeline(), renderer_.getPipelineLayout());

        syncObjects_.create(logicalDevice_.get(), MAX_FRAMES_IN_FLIGHT);

        std::cout << "✅ VulkanCoordinator initialized successfully.\n";
    }


    void VulkanCoordinator::run() {
        while (!window_.shouldClose()) {
            glfwPollEvents();

            vkWaitForFences(logicalDevice_.get(), 1, &syncObjects_.getInFlightFence(currentFrame_), VK_TRUE, UINT64_MAX);
            vkResetFences(logicalDevice_.get(), 1, &syncObjects_.getInFlightFence(currentFrame_));

            VkResult result = vkAcquireNextImageKHR(logicalDevice_.get(), swapchain_.get(), UINT64_MAX,
                syncObjects_.getImageAvailable(currentFrame_), VK_NULL_HANDLE, &imageIndex_);

            if (result != VK_SUCCESS) {
                throw std::runtime_error("❌ Failed to acquire swapchain image!");
            }

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = { syncObjects_.getImageAvailable(currentFrame_) };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;

            VkCommandBuffer cmdBuffer = commandBuffers_.getAll()[imageIndex_];
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmdBuffer;

            VkSemaphore signalSemaphores[] = { syncObjects_.getRenderFinished(currentFrame_) };
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            if (vkQueueSubmit(logicalDevice_.getGraphicsQueue(), 1, &submitInfo, syncObjects_.getInFlightFence(currentFrame_)) != VK_SUCCESS) {
                throw std::runtime_error("❌ Failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;
            VkSwapchainKHR swapchains[] = { swapchain_.get() };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapchains;
            presentInfo.pImageIndices = &imageIndex_;

            result = vkQueuePresentKHR(logicalDevice_.getPresentQueue(), &presentInfo);

            if (result != VK_SUCCESS) {
                throw std::runtime_error("❌ Failed to present swapchain image!");
            }

            currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        vkDeviceWaitIdle(logicalDevice_.get());
    }

    void VulkanCoordinator::cleanup() {
        renderer_.destroy(logicalDevice_.get());
        syncObjects_.destroy(logicalDevice_.get());
        commandBuffers_.free(logicalDevice_.get(), commandPool_.get());
        commandPool_.destroy(logicalDevice_.get());
        framebuffers_.destroy(logicalDevice_.get());
        renderPass_.destroy(logicalDevice_.get());
        swapchain_.destroy(logicalDevice_.get());
        logicalDevice_.destroy();
        surface_.destroy(instance_.get());
        debug_.cleanup(instance_.get());
        instance_.destroy();
    }


}
#include "vulkan_coordinator.hpp"
#include <stdexcept>
#include <iostream>
#include <chrono>

namespace chionia {

    void VulkanCoordinator::init() {
        instance_.create("Chionia Engine", true);
        debug_.setup(instance_.get());
        surface_.create(window_.getGLFWwindow(), instance_.get());
        physicalDevice_.pick(instance_.get(), surface_.get());
        logicalDevice_.create(physicalDevice_.get(), surface_.get());

        swapchain_.create(
            physicalDevice_.get(), logicalDevice_.get(), surface_.get(),
            logicalDevice_.getGraphicsQueueFamily(), logicalDevice_.getPresentQueueFamily(),
            window_.getGLFWwindow());

        renderPass_.create(logicalDevice_.get(), swapchain_.getImageFormat());
        framebuffers_.create(logicalDevice_.get(), renderPass_.get(), swapchain_.getImageViews(), swapchain_.getExtent());

        commandPool_.create(logicalDevice_.get(), physicalDevice_.findGraphicsQueueFamily(instance_.get()));
        uniformBuffer_.create(logicalDevice_.get(), physicalDevice_.getMemoryProperties(), swapchain_.getImageCount());

        VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(swapchain_.getImageCount()) };
        VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = poolSize.descriptorCount;

        if (vkCreateDescriptorPool(logicalDevice_.get(), &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool!");

        generateTestPoints();
        vertexBuffer_.create(logicalDevice_.get(), physicalDevice_.getMemoryProperties(), commandPool_.get(), logicalDevice_.getGraphicsQueue(), demoVertices);

        auto bindingDesc = vertexBuffer_.getBindingDescription();
        auto attrDescs = vertexBuffer_.getAttributeDescriptions();
        renderer_.getPipelineBuilder().setVertexInput(bindingDesc, { attrDescs.begin(), attrDescs.end() });

        renderer_.create(logicalDevice_.get(), swapchain_.getExtent(), renderPass_.get(), shaderPath_Vert, shaderPath_Frag);

        uniformBuffer_.createDescriptorSets(logicalDevice_.get(), descriptorPool_, renderer_.getDescriptorSetLayout(), swapchain_.getImageCount());

        commandBuffers_.allocate(logicalDevice_.get(), commandPool_.get(), static_cast<uint32_t>(framebuffers_.getAll().size()));
        commandBuffers_.record(renderPass_.get(), framebuffers_.getAll(), swapchain_.getExtent(),
                               renderer_.getPipeline(), renderer_.getPipelineLayout(),
                               vertexBuffer_.getBuffer(), static_cast<uint32_t>(demoVertices.size()),
                               uniformBuffer_.getDescriptorSets());

        syncObjects_.create(logicalDevice_.get(), MAX_FRAMES_IN_FLIGHT);
    }

    void VulkanCoordinator::drawFrame() {
        glfwPollEvents();

        if (window_.wasResized()) {
            vkDeviceWaitIdle(logicalDevice_.get());
            recreateSwapchain();
            return;
        }

        syncObjects_.waitAndResetFence(logicalDevice_.get(), currentFrame_);

        VkResult result = vkAcquireNextImageKHR(logicalDevice_.get(), swapchain_.get(), UINT64_MAX,
                                                syncObjects_.getImageAvailable(currentFrame_), VK_NULL_HANDLE, &imageIndex_);

        if (shouldRecreateSwapchain(result)) {
            recreateSwapchain();
            return;
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire swapchain image!");
        }

        updateUniforms(imageIndex_);
        presentFrame(imageIndex_, currentFrame_);
        currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;

        // Optional: lightweight FPS tracking
        fpsFrameCount_++;
        auto now = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float>(now - fpsLastTime_).count();
        if (elapsed >= 1.0f) {
            std::cout << "🧭 FPS: " << fpsFrameCount_ << "\n";
            fpsFrameCount_ = 0;
            fpsLastTime_ = now;
        }
    }

    void VulkanCoordinator::updateUniforms(uint32_t imageIndex) {
        UniformBufferObject ubo;
        ubo.model = glm::mat4(1.0f);
        ubo.view = camera_.getViewMatrix();
        ubo.projection = camera_.getProjectionMatrix(
            static_cast<float>(swapchain_.getExtent().width) / swapchain_.getExtent().height, true);

        uniformBuffer_.update(logicalDevice_.get(), imageIndex, ubo);
    }

    void VulkanCoordinator::presentFrame(uint32_t imageIndex, uint32_t currentFrame) {
        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        VkSemaphore waitSemaphores[] = { syncObjects_.getImageAvailable(currentFrame) };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers_.getAll()[imageIndex];
        VkSemaphore signalSemaphores[] = { syncObjects_.getRenderFinished(currentFrame) };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(logicalDevice_.getGraphicsQueue(), 1, &submitInfo, syncObjects_.getInFlightFence(currentFrame)) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit draw command buffer!");

        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapchains[] = { swapchain_.get() };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        VkResult result = vkQueuePresentKHR(logicalDevice_.getPresentQueue(), &presentInfo);
        if (shouldRecreateSwapchain(result)) {
            recreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swapchain image!");
        }
    }

    void VulkanCoordinator::generateTestPoints() {
        const int gridSize = 400;
        int id = 0;
        for (int x = -gridSize; x <= gridSize; ++x) {
            for (int y = -gridSize; y <= gridSize; ++y) {
                float xf = static_cast<float>(x) / gridSize;
                float yf = static_cast<float>(y) / gridSize;
                demoVertices.push_back({ static_cast<uint32_t>(id++), glm::vec3(xf, yf, 0.0f) });
            }
        }
    }

    void VulkanCoordinator::updateVertices(const std::vector<Vertex>& newVertices) {
        std::lock_guard<std::mutex> lock(vertexUpdateMutex_);
        pendingVertices_ = newVertices;
        vertexUpdatePending_ = true;
    }

    bool VulkanCoordinator::shouldRecreateSwapchain(VkResult result) const {
        return result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window_.wasResized();
    }

    void VulkanCoordinator::recreateSwapchain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window_.getGLFWwindow(), &width, &height);
        if (width == 0 || height == 0) return;

        vkDeviceWaitIdle(logicalDevice_.get());
        framebuffers_.destroy(logicalDevice_.get());
        swapchain_.destroy(logicalDevice_.get());
        renderPass_.destroy(logicalDevice_.get());
        commandBuffers_.free(logicalDevice_.get(), commandPool_.get());

        swapchain_.create(physicalDevice_.get(), logicalDevice_.get(), surface_.get(),
                          logicalDevice_.getGraphicsQueueFamily(), logicalDevice_.getPresentQueueFamily(),
                          window_.getGLFWwindow());

        renderPass_.create(logicalDevice_.get(), swapchain_.getImageFormat());
        renderer_.destroy(logicalDevice_.get());
        renderer_.create(logicalDevice_.get(), swapchain_.getExtent(), renderPass_.get(), shaderPath_Vert, shaderPath_Frag);

        framebuffers_.create(logicalDevice_.get(), renderPass_.get(), swapchain_.getImageViews(), swapchain_.getExtent());

        commandBuffers_.allocate(logicalDevice_.get(), commandPool_.get(), static_cast<uint32_t>(framebuffers_.getAll().size()));
        commandBuffers_.record(renderPass_.get(), framebuffers_.getAll(), swapchain_.getExtent(),
                               renderer_.getPipeline(), renderer_.getPipelineLayout(),
                               vertexBuffer_.getBuffer(), static_cast<uint32_t>(vertexBuffer_.getVertexCount()),
                               uniformBuffer_.getDescriptorSets());

        window_.resetResizeFlag();
        currentFrame_ = 0;
    }

    void VulkanCoordinator::cleanup() {
        vkDeviceWaitIdle(logicalDevice_.get());

        if (descriptorPool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(logicalDevice_.get(), descriptorPool_, nullptr);
            descriptorPool_ = VK_NULL_HANDLE;
        }

        uniformBuffer_.destroy(logicalDevice_.get());
        renderer_.destroy(logicalDevice_.get());
        syncObjects_.destroy(logicalDevice_.get());
        commandBuffers_.free(logicalDevice_.get(), commandPool_.get());
        vertexBuffer_.destroy();
        framebuffers_.destroy(logicalDevice_.get());
        renderPass_.destroy(logicalDevice_.get());
        swapchain_.destroy(logicalDevice_.get());
        commandPool_.destroy(logicalDevice_.get());
        logicalDevice_.destroy();
        surface_.destroy(instance_.get());
        debug_.cleanup(instance_.get());
        instance_.destroy();
        window_.destroy();
    }

}

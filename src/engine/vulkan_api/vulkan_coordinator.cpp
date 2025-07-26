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
            window_.getGLFWwindow()
        );

        renderPass_.create(logicalDevice_.get(), swapchain_.getImageFormat());
        framebuffers_.create(logicalDevice_.get(), renderPass_.get(), swapchain_.getImageViews(), swapchain_.getExtent());

        commandPool_.create(logicalDevice_.get(), physicalDevice_.findGraphicsQueueFamily(instance_.get()));

         uniformBuffer_.create(
            logicalDevice_.get(),
            physicalDevice_.getMemoryProperties(),
            swapchain_.getImageCount()
            );

        // Step 1. Create a descriptor pool
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(swapchain_.getImageCount());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(swapchain_.getImageCount());


        if (vkCreateDescriptorPool(logicalDevice_.get(), &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }



        generateTestPoints();

        vertexBuffer_.create(
            logicalDevice_.get(),
            physicalDevice_.getMemoryProperties(),
            commandPool_.get(),
            logicalDevice_.getGraphicsQueue(),
            demoVertices);

        auto bindingDesc = vertexBuffer_.getBindingDescription();
        auto attrDescs = vertexBuffer_.getAttributeDescriptions();
        renderer_.getPipelineBuilder().setVertexInput(bindingDesc, { attrDescs.begin(), attrDescs.end()});

        renderer_.create(logicalDevice_.get(), swapchain_.getExtent(), renderPass_.get(), shaderPath_Vert, shaderPath_Frag);

        // Step 2. Allocate & update descriptor sets
        uniformBuffer_.createDescriptorSets(
            logicalDevice_.get(),
            descriptorPool_,
            renderer_.getDescriptorSetLayout(),
            swapchain_.getImageCount()
            );


        commandBuffers_.allocate(logicalDevice_.get(), commandPool_.get(), static_cast<uint32_t>(framebuffers_.getAll().size()));
        commandBuffers_.record(renderPass_.get(), framebuffers_.getAll(), swapchain_.getExtent(),
                               renderer_.getPipeline(), renderer_.getPipelineLayout(), vertexBuffer_.getBuffer(),
                               static_cast<uint32_t>(demoVertices.size()),
                               uniformBuffer_.getDescriptorSets()
                               );




        syncObjects_.create(logicalDevice_.get(), MAX_FRAMES_IN_FLIGHT);

        std::cout << "VulkanCoordinator initialized successfully.\n";

    }

    void VulkanCoordinator::drawFrame() {
        glfwPollEvents();

        // Wait and reset fence
        syncObjects_.waitAndResetFence(logicalDevice_.get(), currentFrame_);

        // Acquire next image
        imageIndex_ = acquireNextImage(currentFrame_);

        // TODO: Implement per-frame updates if needed


        // Camera for testing

        UniformBufferObject ubo;
        ubo.model = glm::mat4(1.0f);
        ubo.view = camera_.getViewMatrix();
        ubo.projection = camera_.getProjectionMatrix(
            static_cast<float>(swapchain_.getExtent().width) / swapchain_.getExtent().height,
            true
        );

        uniformBuffer_.update(logicalDevice_.get(), imageIndex_, ubo);






        // Submit command buffers and present the frame
        presentFrame(imageIndex_, currentFrame_);

        currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;


        // --- FPS Counter ---
        fpsFrameCount_++;
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<float>(now - fpsLastTime_).count();

        if (duration >= 1.0f) {
            std::cout << "🧭 FPS: " << fpsFrameCount_ << "\n";
            fpsFrameCount_ = 0;
            fpsLastTime_ = now;
        }
    }


    uint32_t VulkanCoordinator::acquireNextImage(uint32_t currentFrame) {
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(
            logicalDevice_.get(),
            swapchain_.get(),
            UINT64_MAX,
            syncObjects_.getImageAvailable(currentFrame),
            VK_NULL_HANDLE,
            &imageIndex
        );

        if (result != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to acquire swapchain image!");
        }

        return imageIndex;
    }


    void VulkanCoordinator::presentFrame(uint32_t imageIndex, uint32_t currentFrame) {

        // Present Submit info
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // Wait on Image Available Semaphore
        VkSemaphore waitSemaphores[] = { syncObjects_.getImageAvailable(currentFrame) };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        // Specify the command buffer to submit
        VkCommandBuffer cmdBuffer = commandBuffers_.getAll()[imageIndex];
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        // Signal the render-finished semaphore
        VkSemaphore signalSemaphores[] = { syncObjects_.getRenderFinished(currentFrame) };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // Submit to Graphics Queue
        if (vkQueueSubmit(logicalDevice_.getGraphicsQueue(), 1, &submitInfo, syncObjects_.getInFlightFence(currentFrame)) != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to submit draw command buffer!");
        }

        // Present the frame
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapchains[] = { swapchain_.get() };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        VkResult result = vkQueuePresentKHR(logicalDevice_.getPresentQueue(), &presentInfo);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to present swapchain image!");
        }
    }

    void VulkanCoordinator::updateVertices(const std::vector<Vertex>& newVertices) {
        std::lock_guard<std::mutex> lock(vertexUpdateMutex_);

        // Store for deferred buffer update during the next frame
        pendingVertices_ = newVertices;
        vertexUpdatePending_ = true;
    }


    // Cleanup code
    void VulkanCoordinator::cleanup() {
        // Wait until all GPU operations are done
        vkDeviceWaitIdle(logicalDevice_.get());

        if (descriptorPool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(logicalDevice_.get(), descriptorPool_, nullptr);
            descriptorPool_ = VK_NULL_HANDLE;
        }

        // Destroy in reverse order of creation
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

        // Important: destroy the window and terminate GLFW at the very end
        window_.destroy();
    }

}

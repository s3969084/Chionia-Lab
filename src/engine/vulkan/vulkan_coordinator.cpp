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
            throw std::runtime_error("❌ Failed to create descriptor pool!");
        }




        vertexBuffer_.create(logicalDevice_.get(), physicalDevice_.getMemoryProperties(), demoVertices);

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

        std::cout << "✅ VulkanCoordinator initialized successfully.\n";
    }

    void VulkanCoordinator::drawFrame() {
        glfwPollEvents();

        static double lastTime = glfwGetTime();
        static int frameCount = 0;

        frameCount++;
        double currentTime = glfwGetTime();
        if (currentTime - lastTime >= 1.0) { // If a second has passed
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }

        // Wait and reset fence
        syncObjects_.waitAndResetFence(logicalDevice_.get(), currentFrame_);

        // --- 3D Infinity Animation --- testing
        std::vector<Vertex> newVertices;
        const int numPoints = 150;  // smoothness
        float t = glfwGetTime() * 0.10f;

        for (int i = 0; i < numPoints; ++i) {

            float offset = (float)i / numPoints * glm::two_pi<float>();
            float scale = 0.75f;  // Adjust size
            float speed = 1.0f;

            float x = scale * sin(speed * t + offset);
            float y = scale * sin((speed * t + offset) * 2.0f) * 0.5f;
            float z = scale * sin((speed * t + offset)) * cos((speed * t + offset));

            newVertices.push_back(Vertex{ 1, glm::vec3(x, y, z) });
        }

        // Trigger buffer update
            updateVertices(newVertices);


        // Testing code ends here


        // Handle pending vertex update safely
        {
            std::lock_guard<std::mutex> lock(vertexUpdateMutex_);

            if (vertexUpdatePending_) {
                vkDeviceWaitIdle(logicalDevice_.get()); // block to be sure
                vertexBuffer_.destroy(logicalDevice_.get());
                vertexBuffer_.create(logicalDevice_.get(), physicalDevice_.getMemoryProperties(), pendingVertices_);

                commandBuffers_.record(
                    renderPass_.get(),
                    framebuffers_.getAll(),
                    swapchain_.getExtent(),
                    renderer_.getPipeline(),
                    renderer_.getPipelineLayout(),
                    vertexBuffer_.getBuffer(),
                    static_cast<uint32_t>(pendingVertices_.size()),
                    uniformBuffer_.getDescriptorSets()
                    );

                vertexUpdatePending_ = false;
            }
        }

        // Update uniform buffer

        rotatingAngle_ += 0.01f;  // for testing

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1, 0, 0)); // for testing
       // ubo.model = glm::mat4(1.0f); // Identity for now
        ubo.view = glm::lookAt(glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(0,0,0), glm::vec3(0,1,0));
        ubo.projection = glm::perspective(glm::radians(45.0f),
            static_cast<float>(swapchain_.getExtent().width) / swapchain_.getExtent().height,
            0.1f, 10.0f);
        ubo.projection[1][1] *= -1; // Invert Y axis




        // Acquire next image
        acquireNextImage();

        uniformBuffer_.update(logicalDevice_.get(), currentFrame_, ubo);


        // Submit command buffers & present frame
        presentFrame();

        currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // These are helper functions
    void VulkanCoordinator::acquireNextImage() {
        // Acquire image
        VkResult result = vkAcquireNextImageKHR(
            logicalDevice_.get(), swapchain_.get(), UINT64_MAX,
            syncObjects_.getImageAvailable(currentFrame_), VK_NULL_HANDLE, &imageIndex_
        );
        if (result != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to acquire swapchain image!");
        }
    }


    void VulkanCoordinator::presentFrame() {

        // Present Submit info
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // Wait on Image Available Semaphore
        VkSemaphore waitSemaphores[] = { syncObjects_.getImageAvailable(currentFrame_) };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        // Specify the command buffer to submit
        VkCommandBuffer cmdBuffer = commandBuffers_.getAll()[imageIndex_];
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        // Signal the render-finished semaphore
        VkSemaphore signalSemaphores[] = { syncObjects_.getRenderFinished(currentFrame_) };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // Submit to Graphics Queue
        if (vkQueueSubmit(logicalDevice_.getGraphicsQueue(), 1, &submitInfo, syncObjects_.getInFlightFence(currentFrame_)) != VK_SUCCESS) {
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
        presentInfo.pImageIndices = &imageIndex_;

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
        vertexBuffer_.destroy(logicalDevice_.get());

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


#include "VulkanBackend.hpp"

#include <iostream>
#include <chrono>
#include <stdexcept>

namespace chionia {

void VulkanBackend::init() {

    std::cout << "[Init] Starting Vulkan initialization...\n";

    activeVertices.clear();

    // Start the point animation clock.
    animationStartTime_ = std::chrono::steady_clock::now();

    instance_.create("Chionia Engine", true);
    debug_.setup(instance_.get());

    surface_.create(
        window_.getGLFWwindow(),
        instance_.get()
    );

    physicalDevice_.pick(
        instance_.get(),
        surface_.get()
    );

    logicalDevice_.create(
        physicalDevice_.get(),
        surface_.get()
    );

    swapchain_.create(
        physicalDevice_.get(),
        logicalDevice_.get(),
        surface_.get(),
        logicalDevice_.getGraphicsQueueFamily(),
        logicalDevice_.getPresentQueueFamily(),
        window_.getGLFWwindow()
    );

    renderPass_.create(
        logicalDevice_.get(),
        swapchain_.getImageFormat()
    );

    framebuffers_.create(
        logicalDevice_.get(),
        renderPass_.get(),
        swapchain_.getImageViews(),
        swapchain_.getExtent()
    );

    commandPool_.create(
        logicalDevice_.get(),
        physicalDevice_.findGraphicsQueueFamily(instance_.get())
    );

    uniformBuffer_.create(
        logicalDevice_.get(),
        physicalDevice_.getMemoryProperties(),
        swapchain_.getImageCount()
    );

    // Descriptor pool.
    VkDescriptorPoolSize poolSize{
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        static_cast<uint32_t>(swapchain_.getImageCount())
    };

    VkDescriptorPoolCreateInfo poolInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO
    };

    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = poolSize.descriptorCount;

    if (
        vkCreateDescriptorPool(
            logicalDevice_.get(),
            &poolInfo,
            nullptr,
            &descriptorPool_
        ) != VK_SUCCESS
    ) {
        throw std::runtime_error(
            "Failed to create descriptor pool!"
        );
    }

    // Vertex buffer (if preloaded).
    if (!activeVertices.empty()) {

        vertexBuffer_.create(
            logicalDevice_.get(),
            physicalDevice_.getMemoryProperties(),
            commandPool_.get(),
            logicalDevice_.getGraphicsQueue(),
            activeVertices
        );
    }

    auto bindingDesc =
        vertexBuffer_.getBindingDescription();

    auto attrDescs =
        vertexBuffer_.getAttributeDescriptions();

    renderer_.getPipelineBuilder().setVertexInput(
        bindingDesc,
        { attrDescs.begin(), attrDescs.end() }
    );

    renderer_.create(
        logicalDevice_.get(),
        swapchain_.getExtent(),
        renderPass_.get(),
        shaderPath_Vert,
        shaderPath_Frag
    );

    uniformBuffer_.createDescriptorSets(
        logicalDevice_.get(),
        descriptorPool_,
        renderer_.getDescriptorSetLayout(),
        swapchain_.getImageCount()
    );

    commandBuffers_.allocate(
        logicalDevice_.get(),
        commandPool_.get(),
        static_cast<uint32_t>(
            framebuffers_.getAll().size()
        )
    );

    bool hasData =
        vertexBuffer_.getBuffer() != VK_NULL_HANDLE &&
        !activeVertices.empty();

    commandBuffers_.record(
        renderPass_.get(),
        framebuffers_.getAll(),
        swapchain_.getExtent(),
        renderer_.getPipeline(),
        renderer_.getPipelineLayout(),
        vertexBuffer_.getBuffer(),
        static_cast<uint32_t>(activeVertices.size()),
        uniformBuffer_.getDescriptorSets(),
        !hasData
    );

    // Create frame synchronization objects.
    syncObjects_.create(
     logicalDevice_.get(),
     MAX_FRAMES_IN_FLIGHT,
     swapchain_.getImageCount()
 );

    std::cout
        << "[Init] VulkanBackend initialized successfully.\n";
}


// ------------------------------------------------------------
// FRAME RENDERING
// ------------------------------------------------------------

void VulkanBackend::drawFrame(const Camera& camera) {

    glfwPollEvents();

    // Process queued render commands.
    processRenderQueue();

    // Handle window resize before acquiring another image.
    if (window_.wasResized()) {
        recreateSwapchain();
        return;
    }

    // Wait until this frame's previous GPU submission finishes.
    syncObjects_.waitForFence(
        logicalDevice_.get(),
        currentFrame_
    );

    // Acquire the next swapchain image.
    VkResult acquireResult = vkAcquireNextImageKHR(
        logicalDevice_.get(),
        swapchain_.get(),
        UINT64_MAX,
        syncObjects_.getImageAvailable(currentFrame_),
        VK_NULL_HANDLE,
        &imageIndex_
    );

    // No image was acquired.
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }

    // SUBOPTIMAL still means an image was successfully acquired.
    if (
        acquireResult != VK_SUCCESS &&
        acquireResult != VK_SUBOPTIMAL_KHR
    ) {
        throw std::runtime_error(
            "Failed to acquire swapchain image!"
        );
    }

    // Reset only after successful image acquisition.
    syncObjects_.resetFence(
        logicalDevice_.get(),
        currentFrame_
    );

    // Process pending vertex updates.
    handleVertexUpdate();

    // Update camera and animation uniforms.
    updateUniforms(imageIndex_, camera);

    // Submit GPU rendering work.
    submitFrame(
        imageIndex_,
        currentFrame_
    );

    // Present the rendered image.
    VkResult presentResult = presentFrame(
        imageIndex_,
        currentFrame_
    );

    // Determine whether the swapchain needs recreation.
    const bool needsRecreation =
        acquireResult == VK_SUBOPTIMAL_KHR ||
        presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
        presentResult == VK_SUBOPTIMAL_KHR ||
        window_.wasResized();

    if (
        presentResult != VK_SUCCESS &&
        presentResult != VK_SUBOPTIMAL_KHR &&
        presentResult != VK_ERROR_OUT_OF_DATE_KHR
    ) {
        throw std::runtime_error(
            "Failed to present swapchain image!"
        );
    }

    if (needsRecreation) {
        recreateSwapchain();
        return;
    }

    // Advance to the next frame-in-flight.
    currentFrame_ = (
        currentFrame_ + 1
    ) % MAX_FRAMES_IN_FLIGHT;
}


// ------------------------------------------------------------
// RENDER QUEUE
// ------------------------------------------------------------

void VulkanBackend::processRenderQueue() {

    while (auto cmdOpt = renderQueue_.tryDequeue()) {

        const RenderCommand& cmd = *cmdOpt;

        switch (cmd.type) {

            case RenderCommandType::UpdateVertices:

                updateVertices(
                    std::get<std::vector<Vertex>>(cmd.data)
                );

                break;

            case RenderCommandType::ReloadPipeline:

                reloadPipeline();

                break;

            default:

                std::cout
                    << "[RenderQueue] Unknown command type\n";

                break;
        }
    }
}


// ------------------------------------------------------------
// VERTEX MORPHING
// ------------------------------------------------------------

void VulkanBackend::handleVertexUpdate() {

    std::vector<Vertex> requestedVertices;

    // Safely retrieve pending vertex data.
    {
        std::lock_guard<std::mutex> lock(vertexUpdateMutex_);

        if (!vertexUpdatePending_) {
            return;
        }

        requestedVertices = std::move(pendingVertices);
        vertexUpdatePending_ = false;
    }

    if (requestedVertices.empty()) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();

    // Calculate where the existing morph currently is.
    float currentProgress = 1.0f;

    if (morphActive_) {

        const float elapsed =
            std::chrono::duration<float>(
                now - morphStartTime_
            ).count();

        currentProgress = glm::clamp(
            elapsed / morphDurationSeconds_,
            0.0f,
            1.0f
        );
    }

    // Apply smoothstep easing.
    const float easedProgress =
        currentProgress * currentProgress *
        (3.0f - 2.0f * currentProgress);

    std::vector<Vertex> newVertices;
    newVertices.reserve(requestedVertices.size());

    for (
        std::size_t i = 0;
        i < requestedVertices.size();
        ++i
    ) {

        // Incoming vertex contains the destination position.
        const glm::vec3 targetPosition =
            requestedVertices[i].targetPosition;

        glm::vec3 currentPosition = targetPosition;

        if (!activeVertices.empty()) {

            // Map the old particle index to the new particle count.
            const std::size_t oldIndex =
                (i * activeVertices.size()) /
                requestedVertices.size();

            const Vertex& oldVertex =
                activeVertices[oldIndex];

            // Calculate its actual position at this moment.
            currentPosition = glm::mix(
                oldVertex.position,
                oldVertex.targetPosition,
                easedProgress
            );
        }

        Vertex vertex;

        vertex.id = static_cast<uint32_t>(i);
        vertex.position = currentPosition;
        vertex.targetPosition = targetPosition;

        newVertices.push_back(vertex);
    }

    // Replace CPU-side vertex data.
    activeVertices = std::move(newVertices);

    // Wait for previous GPU operations to finish.
    VkResult idleResult = vkDeviceWaitIdle(
        logicalDevice_.get()
    );

    if (idleResult != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to wait for GPU before vertex update!"
        );
    }

    vertexBuffer_.destroy();

    vertexBuffer_.create(
        logicalDevice_.get(),
        physicalDevice_.getMemoryProperties(),
        commandPool_.get(),
        logicalDevice_.getGraphicsQueue(),
        activeVertices
    );

    // Re-record command buffers with the new vertex buffer.
    commandBuffers_.record(
        renderPass_.get(),
        framebuffers_.getAll(),
        swapchain_.getExtent(),
        renderer_.getPipeline(),
        renderer_.getPipelineLayout(),
        vertexBuffer_.getBuffer(),
        static_cast<uint32_t>(activeVertices.size()),
        uniformBuffer_.getDescriptorSets(),
        false
    );

    // Start a fresh two-second morph.
    morphStartTime_ = now;
    morphProgress_ = 0.0f;
    morphActive_ = true;

    std::cout
        << "[Morph] New target uploaded. Transition started.\n";
}


// ------------------------------------------------------------
// GPU SUBMISSION
// ------------------------------------------------------------

void VulkanBackend::submitFrame(
    uint32_t imageIndex,
    uint32_t currentFrame
) {

    VkSemaphore waitSemaphores[] = {
        syncObjects_.getImageAvailable(currentFrame)
    };

    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO
    };

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;

    submitInfo.pCommandBuffers =
        &commandBuffers_.getAll()[imageIndex];

    VkSemaphore signalSemaphores[] = {
        syncObjects_.getRenderFinished(imageIndex)
    };

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (
        vkQueueSubmit(
            logicalDevice_.getGraphicsQueue(),
            1,
            &submitInfo,
            syncObjects_.getInFlightFence(currentFrame)
        ) != VK_SUCCESS
    ) {
        throw std::runtime_error(
            "Failed to submit draw command buffer!"
        );
    }
}


// ------------------------------------------------------------
// PRESENTATION
// ------------------------------------------------------------

VkResult VulkanBackend::presentFrame(
    uint32_t imageIndex,
    uint32_t currentFrame
) {

    VkSemaphore signalSemaphores[] = {
        syncObjects_.getRenderFinished(imageIndex)
    };

    VkPresentInfoKHR presentInfo{
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR
    };

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {
        swapchain_.get()
    };

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    return vkQueuePresentKHR(
        logicalDevice_.getPresentQueue(),
        &presentInfo
    );
}


// ------------------------------------------------------------
// VERTEX UPDATE QUEUE
// ------------------------------------------------------------

void VulkanBackend::updateVertices(
    const std::vector<Vertex>& vertices
) {

    std::lock_guard<std::mutex> lock(vertexUpdateMutex_);

    pendingVertices = vertices;
    vertexUpdatePending_ = true;
}


// ------------------------------------------------------------
// UNIFORM BUFFER UPDATES
// ------------------------------------------------------------

void VulkanBackend::updateUniforms(
    uint32_t imageIndex,
    const Camera& camera
) {

    UniformBufferObject ubo{};

    ubo.model = glm::mat4(1.0f);
    ubo.view = camera.getViewMatrix();

    ubo.projection = camera.getProjectionMatrix(
        static_cast<float>(swapchain_.getExtent().width) /
        swapchain_.getExtent().height,
        true
    );

    const auto now = std::chrono::steady_clock::now();

    // Existing vibration animation.
    const float elapsedTime =
        std::chrono::duration<float>(
            now - animationStartTime_
        ).count();

    ubo.animation = glm::vec4(
        elapsedTime,
        vibrationAmplitude_,
        vibrationSpeed_,
        vibrationEnabled_ ? 1.0f : 0.0f
    );

    // Morph animation.
    if (morphActive_) {

        const float morphElapsed =
            std::chrono::duration<float>(
                now - morphStartTime_
            ).count();

        morphProgress_ = glm::clamp(
            morphElapsed / morphDurationSeconds_,
            0.0f,
            1.0f
        );

        if (morphProgress_ >= 1.0f) {

            morphProgress_ = 1.0f;
            morphActive_ = false;

            std::cout
                << "[Morph] Transition complete.\n";
        }
    }

    // Keep morphing enabled after completion.
    ubo.morph = glm::vec4(
        morphProgress_,
        1.0f,
        0.0f,
        0.0f
    );

    uniformBuffer_.update(
        logicalDevice_.get(),
        imageIndex,
        ubo
    );
}


// ------------------------------------------------------------
// SWAPCHAIN RECREATION
// ------------------------------------------------------------

void VulkanBackend::recreateSwapchain() {

    int width = 0;
    int height = 0;

    glfwGetFramebufferSize(
        window_.getGLFWwindow(),
        &width,
        &height
    );

    // Wait until the window has a valid framebuffer size.
    while (width == 0 || height == 0) {

        glfwWaitEvents();

        glfwGetFramebufferSize(
            window_.getGLFWwindow(),
            &width,
            &height
        );
    }

    // Ensure all GPU and presentation operations have finished.
    VkResult idleResult = vkDeviceWaitIdle(
        logicalDevice_.get()
    );

    if (idleResult != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to wait for Vulkan device to become idle!"
        );
    }

    // Destroy old synchronization objects.
    syncObjects_.destroy(
        logicalDevice_.get()
    );

    // Free command buffers before destroying resources they reference.
    commandBuffers_.free(
        logicalDevice_.get(),
        commandPool_.get()
    );

    // Destroy old swapchain-dependent resources.
    framebuffers_.destroy(
        logicalDevice_.get()
    );

    uniformBuffer_.destroy(
        logicalDevice_.get()
    );

    if (descriptorPool_ != VK_NULL_HANDLE) {

        vkDestroyDescriptorPool(
            logicalDevice_.get(),
            descriptorPool_,
            nullptr
        );

        descriptorPool_ = VK_NULL_HANDLE;
    }

    renderer_.destroy(
        logicalDevice_.get()
    );

    renderPass_.destroy(
        logicalDevice_.get()
    );

    swapchain_.destroy(
        logicalDevice_.get()
    );

    // --------------------------------------------------------
    // CREATE NEW SWAPCHAIN
    // --------------------------------------------------------

    swapchain_.create(
        physicalDevice_.get(),
        logicalDevice_.get(),
        surface_.get(),
        logicalDevice_.getGraphicsQueueFamily(),
        logicalDevice_.getPresentQueueFamily(),
        window_.getGLFWwindow()
    );

    // --------------------------------------------------------
    // CREATE NEW RENDER PASS AND RENDERER
    // --------------------------------------------------------

    renderPass_.create(
        logicalDevice_.get(),
        swapchain_.getImageFormat()
    );

    renderer_.create(
        logicalDevice_.get(),
        swapchain_.getExtent(),
        renderPass_.get(),
        shaderPath_Vert,
        shaderPath_Frag
    );

    // --------------------------------------------------------
    // CREATE NEW FRAMEBUFFERS
    // --------------------------------------------------------

    framebuffers_.create(
        logicalDevice_.get(),
        renderPass_.get(),
        swapchain_.getImageViews(),
        swapchain_.getExtent()
    );

    // --------------------------------------------------------
    // RECREATE UNIFORM BUFFERS
    // --------------------------------------------------------

    uniformBuffer_.create(
        logicalDevice_.get(),
        physicalDevice_.getMemoryProperties(),
        swapchain_.getImageCount()
    );

    // --------------------------------------------------------
    // RECREATE DESCRIPTOR POOL
    // --------------------------------------------------------

    VkDescriptorPoolSize poolSize{
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        static_cast<uint32_t>(swapchain_.getImageCount())
    };

    VkDescriptorPoolCreateInfo poolInfo{};

    poolInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets =
        static_cast<uint32_t>(swapchain_.getImageCount());

    if (
        vkCreateDescriptorPool(
            logicalDevice_.get(),
            &poolInfo,
            nullptr,
            &descriptorPool_
        ) != VK_SUCCESS
    ) {
        throw std::runtime_error(
            "Failed to recreate descriptor pool!"
        );
    }

    // --------------------------------------------------------
    // RECREATE DESCRIPTOR SETS
    // --------------------------------------------------------

    uniformBuffer_.createDescriptorSets(
        logicalDevice_.get(),
        descriptorPool_,
        renderer_.getDescriptorSetLayout(),
        swapchain_.getImageCount()
    );

    // --------------------------------------------------------
    // RECREATE COMMAND BUFFERS
    // --------------------------------------------------------

    commandBuffers_.allocate(
        logicalDevice_.get(),
        commandPool_.get(),
        static_cast<uint32_t>(
            framebuffers_.getAll().size()
        )
    );

    const bool hasData =
        vertexBuffer_.getBuffer() != VK_NULL_HANDLE &&
        !activeVertices.empty();

    commandBuffers_.record(
        renderPass_.get(),
        framebuffers_.getAll(),
        swapchain_.getExtent(),
        renderer_.getPipeline(),
        renderer_.getPipelineLayout(),
        vertexBuffer_.getBuffer(),
        static_cast<uint32_t>(activeVertices.size()),
        uniformBuffer_.getDescriptorSets(),
        !hasData
    );

    // --------------------------------------------------------
    // RECREATE SYNCHRONIZATION OBJECTS
    // --------------------------------------------------------

    syncObjects_.create(
        logicalDevice_.get(),
        MAX_FRAMES_IN_FLIGHT,
        swapchain_.getImageCount()
    );

    // Reset frame tracking.
    currentFrame_ = 0;
    imageIndex_ = 0;

    window_.resetResizeFlag();

    std::cout
        << "[Vulkan] Swapchain recreated successfully.\n";
}


// ------------------------------------------------------------
// PIPELINE RELOADING
// ------------------------------------------------------------

void VulkanBackend::reloadPipeline() {

    // Optional: shader hot-reloading.
}


// ------------------------------------------------------------
// CLEANUP
// ------------------------------------------------------------

void VulkanBackend::cleanup() {

    syncObjects_.waitAllFrames(
        logicalDevice_.get()
    );

    vkDeviceWaitIdle(
        logicalDevice_.get()
    );

    if (descriptorPool_ != VK_NULL_HANDLE) {

        vkDestroyDescriptorPool(
            logicalDevice_.get(),
            descriptorPool_,
            nullptr
        );

        descriptorPool_ = VK_NULL_HANDLE;
    }

    syncObjects_.destroy(
        logicalDevice_.get()
    );

    commandBuffers_.free(
        logicalDevice_.get(),
        commandPool_.get()
    );

    vertexBuffer_.destroy();

    uniformBuffer_.destroy(
        logicalDevice_.get()
    );

    renderer_.destroy(
        logicalDevice_.get()
    );

    commandPool_.destroy(
        logicalDevice_.get()
    );

    framebuffers_.destroy(
        logicalDevice_.get()
    );

    renderPass_.destroy(
        logicalDevice_.get()
    );

    swapchain_.destroy(
        logicalDevice_.get()
    );

    logicalDevice_.destroy();

    surface_.destroy(
        instance_.get()
    );

    debug_.cleanup(
        instance_.get()
    );

    instance_.destroy();

    window_.destroy();
}

} // namespace chionia

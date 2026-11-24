
#include "VulkanBackend.hpp"

#include <iostream>
#include <chrono>
#include <stdexcept>
#include <mutex>
#include <utility>
#include <vector>

namespace chionia {

// ------------------------------------------------------------
// INITIALIZATION
// ------------------------------------------------------------

void VulkanBackend::init() {

    std::cout << "[Init] Starting Vulkan initialization...\n";

    activeVertices.clear();

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

    // --------------------------------------------------------
    // UNIFORM BUFFER
    // --------------------------------------------------------

    uniformBuffer_.create(
        logicalDevice_.get(),
        physicalDevice_.getMemoryProperties(),
        swapchain_.getImageCount()
    );

    // --------------------------------------------------------
    // DESCRIPTOR POOL
    // --------------------------------------------------------

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount =
        static_cast<uint32_t>(swapchain_.getImageCount());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

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

    // --------------------------------------------------------
    // PARTICLE VERTEX BUFFER
    // --------------------------------------------------------

    if (!activeVertices.empty()) {

        vertexBuffer_.create(
            logicalDevice_.get(),
            physicalDevice_.getMemoryProperties(),
            commandPool_.get(),
            logicalDevice_.getGraphicsQueue(),
            activeVertices
        );
    }

    // --------------------------------------------------------
    // PARTICLE PIPELINE
    // --------------------------------------------------------

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

    // --------------------------------------------------------
    // DESCRIPTOR SETS
    // --------------------------------------------------------

    uniformBuffer_.createDescriptorSets(
        logicalDevice_.get(),
        descriptorPool_,
        renderer_.getDescriptorSetLayout(),
        swapchain_.getImageCount()
    );

    // --------------------------------------------------------
    // COMMAND BUFFERS
    // --------------------------------------------------------

    commandBuffers_.allocate(
        logicalDevice_.get(),
        commandPool_.get(),
        static_cast<uint32_t>(
            framebuffers_.getAll().size()
        )
    );

    const bool hasParticleData =
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
        !hasParticleData
    );

    // --------------------------------------------------------
    // FRAME SYNCHRONIZATION
    // --------------------------------------------------------

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

    processRenderQueue();

    if (window_.wasResized()) {
        recreateSwapchain();
        return;
    }

    syncObjects_.waitForFence(
        logicalDevice_.get(),
        currentFrame_
    );

    VkResult acquireResult = vkAcquireNextImageKHR(
        logicalDevice_.get(),
        swapchain_.get(),
        UINT64_MAX,
        syncObjects_.getImageAvailable(currentFrame_),
        VK_NULL_HANDLE,
        &imageIndex_
    );

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }

    if (
        acquireResult != VK_SUCCESS &&
        acquireResult != VK_SUBOPTIMAL_KHR
    ) {
        throw std::runtime_error(
            "Failed to acquire swapchain image!"
        );
    }

    handleVertexUpdate();

    updateUniforms(imageIndex_, camera);

    syncObjects_.resetFence(
        logicalDevice_.get(),
        currentFrame_
    );

    submitFrame(
        imageIndex_,
        currentFrame_
    );

    VkResult presentResult = presentFrame(
        imageIndex_,
        currentFrame_
    );

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

    constexpr std::size_t EXPECTED_PARTICLE_COUNT = 12000;

    std::vector<Vertex> requestedVertices;

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

    if (
        requestedVertices.size() !=
        EXPECTED_PARTICLE_COUNT
    ) {
        throw std::runtime_error(
            "Incoming shape must contain exactly 12,000 particles."
        );
    }

    if (
        !activeVertices.empty() &&
        activeVertices.size() != requestedVertices.size()
    ) {
        throw std::runtime_error(
            "Active and incoming particle populations do not match."
        );
    }

    const auto now = std::chrono::steady_clock::now();

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

        const glm::vec3 targetPosition =
            requestedVertices[i].targetPosition;

        glm::vec3 currentPosition = targetPosition;

        if (!activeVertices.empty()) {

            const Vertex& oldVertex = activeVertices[i];

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

    const bool needsInitialRecording =
        vertexBuffer_.getBuffer() == VK_NULL_HANDLE;

    syncObjects_.waitAllFrames(
        logicalDevice_.get()
    );

    vertexBuffer_.update(
        logicalDevice_.get(),
        physicalDevice_.getMemoryProperties(),
        commandPool_.get(),
        logicalDevice_.getGraphicsQueue(),
        newVertices
    );

    activeVertices = std::move(newVertices);

    if (needsInitialRecording) {

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

        std::cout
            << "[Vulkan] Particle command buffers recorded.\n";
    }

    morphStartTime_ = now;
    morphProgress_ = 0.0f;
    morphActive_ = true;

    std::cout
        << "[Morph] New 12,000-particle target uploaded. "
        << "Transition started.\n";
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

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

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

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

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

    while (width == 0 || height == 0) {

        glfwWaitEvents();

        glfwGetFramebufferSize(
            window_.getGLFWwindow(),
            &width,
            &height
        );
    }

    VkResult idleResult = vkDeviceWaitIdle(
        logicalDevice_.get()
    );

    if (idleResult != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to wait for Vulkan device to become idle!"
        );
    }

    syncObjects_.destroy(
        logicalDevice_.get()
    );

    commandBuffers_.free(
        logicalDevice_.get(),
        commandPool_.get()
    );

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
    // NEW SWAPCHAIN
    // --------------------------------------------------------

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

    // --------------------------------------------------------
    // RECREATE PARTICLE PIPELINE
    // --------------------------------------------------------

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

    // --------------------------------------------------------
    // FRAMEBUFFERS
    // --------------------------------------------------------

    framebuffers_.create(
        logicalDevice_.get(),
        renderPass_.get(),
        swapchain_.getImageViews(),
        swapchain_.getExtent()
    );

    // --------------------------------------------------------
    // UNIFORM BUFFERS
    // --------------------------------------------------------

    uniformBuffer_.create(
        logicalDevice_.get(),
        physicalDevice_.getMemoryProperties(),
        swapchain_.getImageCount()
    );

    // --------------------------------------------------------
    // DESCRIPTOR POOL
    // --------------------------------------------------------

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount =
        static_cast<uint32_t>(swapchain_.getImageCount());

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
    // DESCRIPTOR SETS
    // --------------------------------------------------------

    uniformBuffer_.createDescriptorSets(
        logicalDevice_.get(),
        descriptorPool_,
        renderer_.getDescriptorSetLayout(),
        swapchain_.getImageCount()
    );

    // --------------------------------------------------------
    // COMMAND BUFFERS
    // --------------------------------------------------------

    commandBuffers_.allocate(
        logicalDevice_.get(),
        commandPool_.get(),
        static_cast<uint32_t>(
            framebuffers_.getAll().size()
        )
    );

    const bool hasParticleData =
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
        !hasParticleData
    );

    // --------------------------------------------------------
    // SYNCHRONIZATION
    // --------------------------------------------------------

    syncObjects_.create(
        logicalDevice_.get(),
        MAX_FRAMES_IN_FLIGHT,
        swapchain_.getImageCount()
    );

    currentFrame_ = 0;
    imageIndex_ = 0;

    window_.resetResizeFlag();

    std::cout
        << "[Vulkan] Particle swapchain recreated successfully.\n";
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

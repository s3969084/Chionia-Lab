
#include "VulkanBackend.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <mutex>
#include <utility>
#include <vector>
#include <limits>

#include <glm/gtc/matrix_transform.hpp>

#include "graphics/conversion/point_vertex_converter.hpp"
#include "scene/ground_plane.hpp"

namespace chionia {

// ------------------------------------------------------------
// INITIALIZATION
// ------------------------------------------------------------

void VulkanBackend::init() {
    init({});
}

void VulkanBackend::init(
    const std::function<void(GLFWwindow*)>& beforeOverlayInitialization
) {

    std::cout << "[Init] Starting Vulkan initialization...\n";

    activeVertices.clear();

    animationStartTime_ = std::chrono::steady_clock::now();

    morphActive_ = false;
    morphProgress_ = 1.0f;

#ifdef CHIONIA_DEBUG
    constexpr bool enableValidation = true;
#else
    constexpr bool enableValidation = false;
#endif

    instance_.create("Chionia Lab", enableValidation);

#ifdef CHIONIA_DEBUG
    if (enableValidation) {
        debug_.setup(instance_.get());
    }
#endif

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
        logicalDevice_.getGraphicsQueueFamily()
    );

    pointCloudRenderer_.initialize(
        logicalDevice_.get(),
        physicalDevice_.getMemoryProperties(),
        commandPool_.get(),
        logicalDevice_.getGraphicsQueue()
    );

    uniformBuffer_.create(
        logicalDevice_.get(),
        physicalDevice_.getMemoryProperties(),
        swapchain_.getImageCount()
    );

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount =
        static_cast<uint32_t>(swapchain_.getImageCount());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
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
            "Failed to create descriptor pool!"
        );
    }

    auto bindingDesc =
        VulkanPointCloudRenderer::getBindingDescription();

    auto attrDescs =
        VulkanPointCloudRenderer::getAttributeDescriptions();

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

    if (beforeOverlayInitialization) {
        beforeOverlayInitialization(window_.getGLFWwindow());
    }

    imguiRenderer_.initialize(
        window_.getGLFWwindow(),
        instance_.get(),
        physicalDevice_.get(),
        logicalDevice_.get(),
        logicalDevice_.getGraphicsQueueFamily(),
        logicalDevice_.getGraphicsQueue(),
        renderPass_.get(),
        swapchain_.getImageCount()
    );

    commandBuffers_.allocate(
        logicalDevice_.get(),
        commandPool_.get(),
        static_cast<uint32_t>(
            framebuffers_.getAll().size()
        )
    );

    const std::size_t residentPointCount =
        pointCloudRenderer_.getResidentPointCount();

    const bool hasParticleData =
        pointCloudRenderer_.getBuffer() != VK_NULL_HANDLE &&
        residentPointCount > 0;

    commandBuffers_.record(
        renderPass_.get(),
        framebuffers_.getAll(),
        swapchain_.getExtent(),
        renderer_.getPipeline(),
        renderer_.getPipelineLayout(),
        pointCloudRenderer_.getBuffer(),
        static_cast<uint32_t>(residentPointCount),
        uniformBuffer_.getDescriptorSets(),
        !hasParticleData
    );

    syncObjects_.create(
        logicalDevice_.get(),
        MAX_FRAMES_IN_FLIGHT,
        swapchain_.getImageCount()
    );

    imagesInFlight_.assign(
        swapchain_.getImageCount(),
        VK_NULL_HANDLE
    );

    std::cout << "[Init] Vulkan initialization complete.\n";
}


// ------------------------------------------------------------
// MAIN FRAME LOOP
// ------------------------------------------------------------

void VulkanBackend::drawFrame(const Camera& camera) {
    imguiRenderer_.finalizeFrame();

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

    if (imageIndex_ >= imagesInFlight_.size()) {
        throw std::runtime_error(
            "Acquired swapchain image has no in-flight fence entry."
        );
    }

    if (imagesInFlight_[imageIndex_] != VK_NULL_HANDLE) {
        const VkResult imageWaitResult = vkWaitForFences(
            logicalDevice_.get(),
            1,
            &imagesInFlight_[imageIndex_],
            VK_TRUE,
            UINT64_MAX
        );

        if (imageWaitResult != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to wait for the acquired swapchain image."
            );
        }
    }

    imagesInFlight_[imageIndex_] =
        syncObjects_.getInFlightFence(currentFrame_);

    handleVertexUpdate();
    handlePointCloudUpdate();

    updateUniforms(
        imageIndex_,
        camera
    );

    recordFrameForImage(imageIndex_);

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

    if (
        presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
        presentResult == VK_SUBOPTIMAL_KHR ||
        window_.wasResized()
    ) {
        recreateSwapchain();
    }
    else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to present swapchain image!"
        );
    }

    currentFrame_ =
        (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanBackend::beginEditorFrame() {
    imguiRenderer_.beginFrame();
}

void VulkanBackend::recordFrameForImage(uint32_t imageIndex) {
    const std::size_t residentPointCount =
        pointCloudRenderer_.getResidentPointCount();

    const bool hasParticleData =
        pointCloudRenderer_.getBuffer() != VK_NULL_HANDLE &&
        residentPointCount > 0;

    commandBuffers_.recordForImage(
        imageIndex,
        renderPass_.get(),
        framebuffers_.getAll(),
        swapchain_.getExtent(),
        renderer_.getPipeline(),
        renderer_.getPipelineLayout(),
        pointCloudRenderer_.getBuffer(),
        static_cast<uint32_t>(residentPointCount),
        uniformBuffer_.getDescriptorSets(),
        !hasParticleData,
        [this](VkCommandBuffer commandBuffer) {
            imguiRenderer_.recordDrawData(commandBuffer);
        }
    );
}


// ------------------------------------------------------------
// RENDER QUEUE
// ------------------------------------------------------------

void VulkanBackend::processRenderQueue() {

    while (auto cmdOpt = renderQueue_.tryDequeue()) {

        RenderCommand& cmd = *cmdOpt;

        switch (cmd.type) {

            case RenderCommandType::UpdateVertices:

                updateVertices(
                    std::get<std::vector<Vertex>>(cmd.data)
                );

                break;

            case RenderCommandType::UpdatePointCloud: {

                std::lock_guard<std::mutex> lock(vertexUpdateMutex_);

                pendingPointCloud_ =
                    std::move(std::get<PointCloud>(cmd.data));

                break;
            }

            case RenderCommandType::SetModelMatrix:
                sceneModelMatrix_ = std::get<glm::mat4>(cmd.data);
                break;

            case RenderCommandType::SetAnimationSpeed:
                animationSpeed_ = std::get<float>(cmd.data);
                break;

            case RenderCommandType::SetMorphDuration:
                morphDurationSeconds_ = std::get<float>(cmd.data);
                break;

            case RenderCommandType::SetVibrationAmplitude:
                vibrationAmplitude_ = std::get<float>(cmd.data);
                break;

            case RenderCommandType::SetVibrationSpeed:
                vibrationSpeed_ = std::get<float>(cmd.data);
                break;

            case RenderCommandType::SetVibrationEnabled:
                vibrationEnabled_ = std::get<bool>(cmd.data);
                break;

            case RenderCommandType::SetMorphingEnabled:
                morphingEnabled_ = std::get<bool>(cmd.data);
                if (!morphingEnabled_ && morphActive_) {
                    morphActive_ = false;
                    morphProgress_ = 1.0f;
                }
                break;

            case RenderCommandType::SetGroundReflectionEnabled:
                groundReflectionEnabled_ = std::get<bool>(cmd.data);
                break;

            case RenderCommandType::SetPointSize:
            case RenderCommandType::ReloadPipeline:
            default:

                break;
        }
    }
}


// ------------------------------------------------------------
// LEGACY VERTEX UPDATE
// ------------------------------------------------------------

void VulkanBackend::handleVertexUpdate() {

    std::vector<Vertex> requestedVertices;

    {
        std::lock_guard<std::mutex> lock(vertexUpdateMutex_);

        if (!vertexUpdatePending_) {
            return;
        }

        requestedVertices = std::move(pendingVertices);
        vertexUpdatePending_ = false;
    }

    if (
        requestedVertices.size() >
        std::numeric_limits<uint32_t>::max()
    ) {
        throw std::overflow_error(
            "Point population exceeds Vulkan draw limits."
        );
    }

    const bool canMorph =
        morphingEnabled_ &&
        !activeVertices.empty() &&
        activeVertices.size() == requestedVertices.size();

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

        if (canMorph) {

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

    syncObjects_.waitAllFrames(
        logicalDevice_.get()
    );

    pointCloudRenderer_.synchronizeVertices(
        newVertices
    );

    activeVertices = std::move(newVertices);

    const bool hasParticleData =
        !activeVertices.empty() &&
        pointCloudRenderer_.getBuffer() != VK_NULL_HANDLE;

    commandBuffers_.record(
        renderPass_.get(),
        framebuffers_.getAll(),
        swapchain_.getExtent(),
        renderer_.getPipeline(),
        renderer_.getPipelineLayout(),
        pointCloudRenderer_.getBuffer(),
        static_cast<uint32_t>(activeVertices.size()),
        uniformBuffer_.getDescriptorSets(),
        !hasParticleData
    );

    if (canMorph && !activeVertices.empty()) {

        morphStartTime_ = now;
        morphProgress_ = 0.0f;
        morphActive_ = true;

        std::cout
            << "[Morph] Updated point cloud with "
            << activeVertices.size()
            << " points. Transition started.\n";
    }
    else {

        morphProgress_ = 1.0f;
        morphActive_ = false;

        std::cout
            << "[Vulkan] Point cloud updated: "
            << activeVertices.size()
            << " points.\n";
    }
}


// ------------------------------------------------------------
// CORE POINT CLOUD UPDATE
// ------------------------------------------------------------

void VulkanBackend::handlePointCloudUpdate() {

    std::optional<PointCloud> requestedPointCloud;

    {
        std::lock_guard<std::mutex> lock(vertexUpdateMutex_);

        if (!pendingPointCloud_) {
            return;
        }

        requestedPointCloud = std::move(pendingPointCloud_);
        pendingPointCloud_.reset();
    }

    const std::size_t pointCount =
        requestedPointCloud->getPointCount();

    if (
        pointCount >
        std::numeric_limits<uint32_t>::max() / 2U
    ) {
        throw std::overflow_error(
            "Point cloud and reflection exceed Vulkan draw limits."
        );
    }

    std::vector<Vertex> requestedVertices =
        PointVertexConverter::convertPointCloudToVertices(
            *requestedPointCloud
        );

    const bool canMorph =
        morphingEnabled_ &&
        !activeVertices.empty() &&
        activeVertices.size() == requestedVertices.size();

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

        if (canMorph) {

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

    std::vector<Vertex> renderVertices;
    renderVertices.reserve(newVertices.size() * 2U);
    renderVertices.insert(
        renderVertices.end(),
        newVertices.begin(),
        newVertices.end()
    );

    for (const Vertex& vertex : newVertices) {
        Vertex reflectedVertex = vertex;
        reflectedVertex.id |= 0x80000000U;
        renderVertices.push_back(reflectedVertex);
    }

    syncObjects_.waitAllFrames(
        logicalDevice_.get()
    );

    pointCloudRenderer_.synchronizeVertices(
        renderVertices
    );

    activeVertices = std::move(newVertices);

    const bool hasPointData =
        !activeVertices.empty() &&
        pointCloudRenderer_.getBuffer() != VK_NULL_HANDLE;

    commandBuffers_.record(
        renderPass_.get(),
        framebuffers_.getAll(),
        swapchain_.getExtent(),
        renderer_.getPipeline(),
        renderer_.getPipelineLayout(),
        pointCloudRenderer_.getBuffer(),
        static_cast<uint32_t>(renderVertices.size()),
        uniformBuffer_.getDescriptorSets(),
        !hasPointData
    );

    if (canMorph && !activeVertices.empty()) {

        morphStartTime_ = now;
        morphProgress_ = 0.0f;
        morphActive_ = true;

        std::cout
            << "[Morph] PointCloud transition started: "
            << activeVertices.size()
            << " points.\n";
    }
    else {

        morphProgress_ = 1.0f;
        morphActive_ = false;

        std::cout
            << "[Vulkan] PointCloud synchronized: "
            << activeVertices.size()
            << " points resident on GPU.\n";
    }
}


// ------------------------------------------------------------
// FRAME SUBMISSION
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

    VkSemaphore signalSemaphores[] = {
        syncObjects_.getRenderFinished(imageIndex)
    };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers =
        &commandBuffers_.getAll()[imageIndex];

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
// FRAME PRESENTATION
// ------------------------------------------------------------

VkResult VulkanBackend::presentFrame(
    uint32_t imageIndex,
    uint32_t currentFrame
) {

    VkSemaphore waitSemaphores[] = {
        syncObjects_.getRenderFinished(imageIndex)
    };

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSemaphores;

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

    const auto now = std::chrono::steady_clock::now();

    const float elapsedTime =
        std::chrono::duration<float>(
            now - animationStartTime_
        ).count();

    const float animationTime = elapsedTime * animationSpeed_;

    ubo.model = sceneModelMatrix_;

    ubo.view = camera.getViewMatrix();

    ubo.projection = camera.getProjectionMatrix(
        static_cast<float>(swapchain_.getExtent().width) /
        swapchain_.getExtent().height,
        true
    );

    ubo.animation = glm::vec4(
        animationTime,
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

    float lowestWorldY = std::numeric_limits<float>::infinity();
    const float smoothMorphProgress =
        morphProgress_ * morphProgress_ *
        (3.0f - 2.0f * morphProgress_);

    for (const Vertex& vertex : activeVertices) {
        const glm::vec3 localPosition = glm::mix(
            vertex.position,
            vertex.targetPosition,
            smoothMorphProgress
        );
        const glm::vec4 worldPosition =
            sceneModelMatrix_ * glm::vec4(localPosition, 1.0f);
        lowestWorldY = std::min(lowestWorldY, worldPosition.y);
    }

    float objectGroundClearance = 0.0f;
    if (std::isfinite(lowestWorldY)) {
        const float vibrationMargin = vibrationEnabled_
            ? vibrationAmplitude_
            : 0.0f;
        objectGroundClearance = std::max(
            0.0f,
            lowestWorldY - SCENE_GROUND_HEIGHT - vibrationMargin
        );
    }

    ubo.ground = glm::vec4(
        SCENE_GROUND_HEIGHT,
        groundReflectionEnabled_ ? 1.0f : 0.0f,
        objectGroundClearance,
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

    imguiRenderer_.releaseVulkanBackend();

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

    auto bindingDesc =
        VulkanPointCloudRenderer::getBindingDescription();

    auto attrDescs =
        VulkanPointCloudRenderer::getAttributeDescriptions();

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

    imguiRenderer_.initializeVulkanBackend(
        renderPass_.get(),
        swapchain_.getImageCount()
    );

    framebuffers_.create(
        logicalDevice_.get(),
        renderPass_.get(),
        swapchain_.getImageViews(),
        swapchain_.getExtent()
    );

    uniformBuffer_.create(
        logicalDevice_.get(),
        physicalDevice_.getMemoryProperties(),
        swapchain_.getImageCount()
    );

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount =
        static_cast<uint32_t>(swapchain_.getImageCount());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

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

    const std::size_t residentPointCount =
        pointCloudRenderer_.getResidentPointCount();

    const bool hasParticleData =
        pointCloudRenderer_.getBuffer() != VK_NULL_HANDLE &&
        residentPointCount > 0;

    commandBuffers_.record(
        renderPass_.get(),
        framebuffers_.getAll(),
        swapchain_.getExtent(),
        renderer_.getPipeline(),
        renderer_.getPipelineLayout(),
        pointCloudRenderer_.getBuffer(),
        static_cast<uint32_t>(residentPointCount),
        uniformBuffer_.getDescriptorSets(),
        !hasParticleData
    );

    syncObjects_.create(
        logicalDevice_.get(),
        MAX_FRAMES_IN_FLIGHT,
        swapchain_.getImageCount()
    );

    imagesInFlight_.assign(
        swapchain_.getImageCount(),
        VK_NULL_HANDLE
    );

    currentFrame_ = 0;
    imageIndex_ = 0;

    window_.resetResizeFlag();

    std::cout
        << "[Vulkan] Point cloud swapchain recreated successfully.\n";
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

    imguiRenderer_.shutdown();
    imagesInFlight_.clear();

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

    pointCloudRenderer_.release();

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

#include "VulkanBackend.hpp"
#include <iostream>
#include <chrono>

namespace chionia {

void VulkanBackend::init() {
    std::cout << "[Init] Starting Vulkan initialization...\n";

    activeVertices.clear();

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

    if (!activeVertices.empty()) {
        vertexBuffer_.create(
            logicalDevice_.get(),
            physicalDevice_.getMemoryProperties(),
            commandPool_.get(),
            logicalDevice_.getGraphicsQueue(),
            activeVertices);
    }

    auto bindingDesc = vertexBuffer_.getBindingDescription();
    auto attrDescs = vertexBuffer_.getAttributeDescriptions();
    renderer_.getPipelineBuilder().setVertexInput(bindingDesc, { attrDescs.begin(), attrDescs.end() });

    renderer_.create(logicalDevice_.get(), swapchain_.getExtent(), renderPass_.get(), shaderPath_Vert, shaderPath_Frag);
    uniformBuffer_.createDescriptorSets(logicalDevice_.get(), descriptorPool_, renderer_.getDescriptorSetLayout(), swapchain_.getImageCount());

    commandBuffers_.allocate(logicalDevice_.get(), commandPool_.get(), static_cast<uint32_t>(framebuffers_.getAll().size()));

    bool hasData = vertexBuffer_.getBuffer() != VK_NULL_HANDLE && !activeVertices.empty();
    commandBuffers_.record(renderPass_.get(), framebuffers_.getAll(), swapchain_.getExtent(),
                           renderer_.getPipeline(), renderer_.getPipelineLayout(),
                           vertexBuffer_.getBuffer(), static_cast<uint32_t>(activeVertices.size()),
                           uniformBuffer_.getDescriptorSets(), !hasData);

    syncObjects_.create(logicalDevice_.get(), MAX_FRAMES_IN_FLIGHT);
    std::cout << "[Init] VulkanBackend initialized successfully.\n";
}

void VulkanBackend::drawFrame(const Camera& camera) {
    glfwPollEvents();

    // 🔁 Handle queued render commands
    while (auto cmdOpt = renderQueue_.tryDequeue()) {
        const RenderCommand& cmd = *cmdOpt;
        switch (cmd.type) {
            case RenderCommandType::UpdateVertices:
                updateVertices(std::get<std::vector<Vertex>>(cmd.data));
                break;
            case RenderCommandType::ReloadPipeline:
                reloadPipeline();
                break;
            default:
                std::cout << "[RenderQueue] Unknown command type\n";
                break;
        }
    }

    if (window_.wasResized()) {
        syncObjects_.waitAllFrames(logicalDevice_.get());
        recreateSwapchain();
        return;
    }

    syncObjects_.ResetFence(logicalDevice_.get(), currentFrame_);

    VkResult result = vkAcquireNextImageKHR(
        logicalDevice_.get(), swapchain_.get(), UINT64_MAX,
        syncObjects_.getImageAvailable(currentFrame_), VK_NULL_HANDLE, &imageIndex_);

    if (shouldRecreateSwapchain(result)) {
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swapchain image!");
    }

    if (vertexUpdatePending_) {
        std::lock_guard<std::mutex> lock(vertexUpdateMutex_);
        activeVertices = pendingVertices;
        vertexUpdatePending_ = false;

        vkDeviceWaitIdle(logicalDevice_.get());

        vertexBuffer_.destroy();
        vertexBuffer_.create(
            logicalDevice_.get(), physicalDevice_.getMemoryProperties(),
            commandPool_.get(), logicalDevice_.getGraphicsQueue(),
            activeVertices);

        commandBuffers_.record(
            renderPass_.get(), framebuffers_.getAll(), swapchain_.getExtent(),
            renderer_.getPipeline(), renderer_.getPipelineLayout(),
            vertexBuffer_.getBuffer(), static_cast<uint32_t>(activeVertices.size()),
            uniformBuffer_.getDescriptorSets(), false);

        std::cout << "[Vertex Reload] New vertex data loaded and command buffers re-recorded.\n";
    }

    updateUniforms(imageIndex_, camera);

    VkSemaphore waitSemaphores[] = { syncObjects_.getImageAvailable(currentFrame_) };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_.getAll()[imageIndex_];
    VkSemaphore signalSemaphores[] = { syncObjects_.getRenderFinished(currentFrame_) };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(logicalDevice_.getGraphicsQueue(), 1, &submitInfo, syncObjects_.getInFlightFence(currentFrame_)) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer!");

    VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapchains[] = { swapchain_.get() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex_;

    result = vkQueuePresentKHR(logicalDevice_.getPresentQueue(), &presentInfo);
    if (shouldRecreateSwapchain(result)) {
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image!");
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanBackend::updateVertices(const std::vector<Vertex>& vertices) {
    std::lock_guard<std::mutex> lock(vertexUpdateMutex_);
    pendingVertices = vertices;
    vertexUpdatePending_ = true;
}

void VulkanBackend::updateUniforms(uint32_t imageIndex, const Camera& camera) {
    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = camera.getViewMatrix();
    ubo.projection = camera.getProjectionMatrix(
        static_cast<float>(swapchain_.getExtent().width) / swapchain_.getExtent().height, true);
    uniformBuffer_.update(logicalDevice_.get(), imageIndex, ubo);
}

bool VulkanBackend::shouldRecreateSwapchain(VkResult result) const {
    return result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window_.wasResized();
}

void VulkanBackend::recreateSwapchain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_.getGLFWwindow(), &width, &height);
    if (width == 0 || height == 0) return;

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

    bool hasData = vertexBuffer_.getBuffer() != VK_NULL_HANDLE && !activeVertices.empty();
    commandBuffers_.record(renderPass_.get(), framebuffers_.getAll(), swapchain_.getExtent(),
                           renderer_.getPipeline(), renderer_.getPipelineLayout(),
                           vertexBuffer_.getBuffer(), static_cast<uint32_t>(vertexBuffer_.getVertexCount()),
                           uniformBuffer_.getDescriptorSets(), !hasData);

    window_.resetResizeFlag();
    currentFrame_ = 0;
}

void VulkanBackend::reloadPipeline() {
    // Optional: implement shader hot-reloading here
}

void VulkanBackend::cleanup() {
    // Wait and clear the buffers before destroying
    syncObjects_.waitAllFrames(logicalDevice_.get());
    vkDeviceWaitIdle(logicalDevice_.get());

    vkDestroyDescriptorPool(logicalDevice_.get(), descriptorPool_, nullptr);
    syncObjects_.destroy(logicalDevice_.get());
    commandBuffers_.free(logicalDevice_.get(), commandPool_.get());
    vertexBuffer_.destroy();
    uniformBuffer_.destroy(logicalDevice_.get());
    renderer_.destroy(logicalDevice_.get());
    commandPool_.destroy(logicalDevice_.get());
    framebuffers_.destroy(logicalDevice_.get());
    renderPass_.destroy(logicalDevice_.get());
    swapchain_.destroy(logicalDevice_.get());
    logicalDevice_.destroy();
    surface_.destroy(instance_.get());
    debug_.cleanup(instance_.get());
    instance_.destroy();
    window_.destroy();
}


} // namespace chionia

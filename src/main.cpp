#include "core/app_window.hpp"
#include "vulkan_backend/vulkan_instance.hpp"
#include "vulkan_backend/VulkanDebugMessenger.hpp"
#include "vulkan_backend/vulkan_surface.hpp"
#include "vulkan_backend/vulkan_physical_device.hpp"
#include "vulkan_backend/vulkan_logical_device.hpp"
#include "vulkan_backend/vulkan_swapchain.hpp"
#include "vulkan_backend/vulkan_render_pass.hpp"
#include "vulkan_backend/vulkan_frame_buffer.hpp"
#include "vulkan_backend/vulkan_command_pool.hpp"
#include "vulkan_backend/vulkan_command_buffers.hpp"
#include "vulkan_backend/vulkan_sync_objects.hpp"
#include "vulkan_backend/vulkan_pipeline_builder.hpp"

#include <iostream>
#include <fstream>

int main() {
    constexpr int MAX_FRAMES_IN_FLIGHT = 3;
    size_t currentFrame = 0;
    uint32_t imageIndex = 0;

    try {
        chionia::AppWindow tempWindow(800, 600, "Vulkan Test");

        chionia::VulkanInstance instance;
        instance.create("Vulkan Test", true);

        chionia::VulkanDebugMessenger debug;
        debug.setup(instance.get());
        std::cout << "✅ Vulkan instance and debug messenger created successfully!\n";

        chionia::VulkanSurface surface;
        surface.create(tempWindow.getGLFWwindow(), instance.get());

        chionia::VulkanPhysicalDevice physicalDevice;
        physicalDevice.pick(instance.get(), surface.get());

        chionia::VulkanLogicalDevice logicalDevice;
        logicalDevice.create(physicalDevice.get(), surface.get());

        chionia::VulkanSwapchain swapchain;
        swapchain.create(
            physicalDevice.get(), logicalDevice.get(), surface.get(),
            logicalDevice.getGraphicsQueueFamily(), logicalDevice.getPresentQueueFamily(),
            tempWindow.getGLFWwindow());

        chionia::VulkanRenderPass renderPass;
        renderPass.create(logicalDevice.get(), swapchain.getImageFormat());

        chionia::VulkanFramebuffer framebuffers;
        framebuffers.create(
            logicalDevice.get(), renderPass.get(),
            swapchain.getImageViews(), swapchain.getExtent());

        chionia::VulkanCommandPool commandPool;
        commandPool.create(logicalDevice.get(), physicalDevice.findGraphicsQueueFamily(instance.get()));

        auto readShader = [](const std::string& path, VkDevice logicalDevice) -> VkShaderModule {
            return chionia::VulkanPipelineBuilder::createShaderModule(logicalDevice, path);
        };

        VkShaderModule vertShader = readShader("shaders/point.vert.spv", logicalDevice.get());
        VkShaderModule fragShader = readShader("shaders/point.frag.spv", logicalDevice.get());

        VkPipelineLayout pipelineLayout;
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        if (vkCreatePipelineLayout(logicalDevice.get(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to create pipeline layout!");
        }

        chionia::VulkanPipelineBuilder pipelineBuilder;
        pipelineBuilder.setShaderStages(vertShader, fragShader);
        pipelineBuilder.setInputAssembly(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        pipelineBuilder.setViewportAndScissor(swapchain.getExtent());
        pipelineBuilder.setPipelineLayout(pipelineLayout);
        pipelineBuilder.setRenderPass(renderPass.get());

        VkPipeline graphicsPipeline = pipelineBuilder.build(logicalDevice.get());

        chionia::VulkanCommandBuffers commandBuffers;
        commandBuffers.allocate(logicalDevice.get(), commandPool.get(), static_cast<uint32_t>(framebuffers.getAll().size()));
        commandBuffers.record(renderPass.get(), framebuffers.getAll(), swapchain.getExtent(), graphicsPipeline, pipelineLayout);

        std::cout << "✅ Vulkan command pool and buffers set up.\n";

        chionia::VulkanSyncObjects syncObjects;
        syncObjects.create(logicalDevice.get(), MAX_FRAMES_IN_FLIGHT);

        while (!tempWindow.shouldClose()) {
            glfwPollEvents();

            vkWaitForFences(logicalDevice.get(), 1, &syncObjects.getInFlightFence(currentFrame), VK_TRUE, UINT64_MAX);
            vkResetFences(logicalDevice.get(), 1, &syncObjects.getInFlightFence(currentFrame));

            VkResult result = vkAcquireNextImageKHR(
                logicalDevice.get(), swapchain.get(), UINT64_MAX,
                syncObjects.getImageAvailable(currentFrame), VK_NULL_HANDLE, &imageIndex);

            if (result != VK_SUCCESS) {
                throw std::runtime_error("❌ Failed to acquire swapchain image!");
            }

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = { syncObjects.getImageAvailable(currentFrame) };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;

            VkCommandBuffer cmdBuffer = commandBuffers.getAll()[imageIndex];
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmdBuffer;

            VkSemaphore signalSemaphores[] = { syncObjects.getRenderFinished(currentFrame) };
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            if (vkQueueSubmit(logicalDevice.getGraphicsQueue(), 1, &submitInfo, syncObjects.getInFlightFence(currentFrame)) != VK_SUCCESS) {
                throw std::runtime_error("❌ Failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapchains[] = { swapchain.get() };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapchains;
            presentInfo.pImageIndices = &imageIndex;

            result = vkQueuePresentKHR(logicalDevice.getPresentQueue(), &presentInfo);

            if (result != VK_SUCCESS) {
                throw std::runtime_error("❌ Failed to present swapchain image!");
            }

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        vkDeviceWaitIdle(logicalDevice.get());

        // Cleanup
        vkDestroyPipeline(logicalDevice.get(), graphicsPipeline, nullptr);
        vkDestroyShaderModule(logicalDevice.get(), vertShader, nullptr);
        vkDestroyShaderModule(logicalDevice.get(), fragShader, nullptr);
        vkDestroyPipelineLayout(logicalDevice.get(), pipelineLayout, nullptr);
        syncObjects.destroy(logicalDevice.get());
        commandBuffers.free(logicalDevice.get(), commandPool.get());
        commandPool.destroy(logicalDevice.get());
        framebuffers.destroy(logicalDevice.get());
        renderPass.destroy(logicalDevice.get());
        swapchain.destroy(logicalDevice.get());
        logicalDevice.destroy();
        surface.destroy(instance.get());
        debug.cleanup(instance.get());
        instance.destroy();

    } catch (const std::exception& e) {
        std::cerr << "❌ Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

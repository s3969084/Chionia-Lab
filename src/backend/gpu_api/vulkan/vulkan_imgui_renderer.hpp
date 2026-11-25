#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

struct GLFWwindow;

namespace chionia {

    class VulkanImGuiRenderer {
    public:
        VulkanImGuiRenderer() = default;
        ~VulkanImGuiRenderer();

        VulkanImGuiRenderer(const VulkanImGuiRenderer&) = delete;
        VulkanImGuiRenderer& operator=(const VulkanImGuiRenderer&) = delete;

        void initialize(
            GLFWwindow* window,
            VkInstance instance,
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            uint32_t queueFamily,
            VkQueue queue,
            VkRenderPass renderPass,
            uint32_t imageCount
        );

        void releaseVulkanBackend();
        void initializeVulkanBackend(
            VkRenderPass renderPass,
            uint32_t imageCount
        );

        void beginFrame();
        void finalizeFrame();
        void recordDrawData(VkCommandBuffer commandBuffer);
        void shutdown();

    private:
        void createVulkanBackend(
            VkRenderPass renderPass,
            uint32_t imageCount
        );
        void shutdownVulkanBackend();

        VkInstance instance_ = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
        VkDevice device_ = VK_NULL_HANDLE;
        uint32_t queueFamily_ = 0;
        VkQueue queue_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;

        bool contextCreated_ = false;
        bool glfwBackendInitialized_ = false;
        bool vulkanBackendInitialized_ = false;
        bool frameStarted_ = false;
        bool drawDataReady_ = false;
    };

}

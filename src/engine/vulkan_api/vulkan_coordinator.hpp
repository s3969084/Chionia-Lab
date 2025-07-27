#pragma once

#include <mutex>
#include <vector>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "windowing/app_window.hpp"
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
#include "vulkan_backend/vulkan_renderer.hpp"
#include "vulkan_backend/vulkan_vertex_buffer.hpp"
#include "vulkan_backend/vulkan_uniform_buffer.hpp"
#include "engine/transform/camera.hpp"

namespace chionia {

    class VulkanCoordinator {
    public:
        void init();
        void drawFrame(const Camera& camera);;
        void cleanup();

        const AppWindow& getWindow() const { return window_; }
        void updateVertices(const std::vector<Vertex>& vertices);

    private:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 3;
        size_t currentFrame_ = 0;

        // Core Vulkan components
        AppWindow window_{800, 600, "Chionia Engine"};
        VulkanInstance instance_;
        VulkanDebugMessenger debug_;
        VulkanSurface surface_;
        VulkanPhysicalDevice physicalDevice_;
        VulkanLogicalDevice logicalDevice_;
        VulkanSwapchain swapchain_;
        VulkanRenderPass renderPass_;
        VulkanFramebuffer framebuffers_;
        VulkanCommandPool commandPool_;
        VulkanCommandBuffers commandBuffers_;
        VulkanSyncObjects syncObjects_;
        VulkanRenderer renderer_;
        VulkanVertexBuffer vertexBuffer_;
        UniformBuffer uniformBuffer_;
        VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;

        // Shader paths
        const std::string shaderPath_Vert = "shaders/point.vert.spv";
        const std::string shaderPath_Frag = "shaders/point.frag.spv";

        // State
        Camera camera_;
        uint32_t imageIndex_ = 0;
        std::vector<Vertex> activeVertices;

        // Synchronization
        std::mutex vertexUpdateMutex_;
        bool vertexUpdatePending_ = false;
        std::vector<Vertex> pendingVertices_;

        // Timing
        std::chrono::time_point<std::chrono::high_resolution_clock> fpsLastTime_ = std::chrono::high_resolution_clock::now();
        int fpsFrameCount_ = 0;

        // Internal helpers
        bool shouldRecreateSwapchain(VkResult result) const;
        void recreateSwapchain();
        void updateUniforms(uint32_t imageIndex, const Camera& camera);
        void presentFrame(uint32_t imageIndex, uint32_t currentFrame);


    };

}

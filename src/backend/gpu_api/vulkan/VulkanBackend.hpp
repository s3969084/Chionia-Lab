#pragma once

// --- Core Engine Interfaces ---
#include "graphics/interface/IRenderBackend.hpp"
#include "windowing/app_window.hpp"

// --- Vulkan Subsystems ---
#include "vulkan_instance.hpp"
#include "VulkanDebugMessenger.hpp"
#include "vulkan_surface.hpp"
#include "vulkan_physical_device.hpp"
#include "vulkan_logical_device.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_frame_buffer.hpp"
#include "vulkan_command_pool.hpp"
#include "vulkan_command_buffers.hpp"
#include "vulkan_sync_objects.hpp"
#include "vulkan_renderer.hpp"
#include "vulkan_vertex_buffer.hpp"
#include "vulkan_uniform_buffer.hpp"

// --- Scene + Commands ---
#include "scene/camera.hpp"
#include "core/commands/render_queue.hpp"

namespace chionia {

    class VulkanBackend : public IRenderBackend {
    public:
        VulkanBackend(int width, int height, const std::string& title)
            : window_(width, height, title) {}

        // --- Lifecycle ---
        void init() override;
        void cleanup() override;

        // --- Main Loop ---
        void drawFrame(const Camera& camera) override;
        bool shouldClose() const override { return window_.shouldClose(); }

        // --- Command Processing ---
        void updateVertices(const std::vector<Vertex>& vertices) override;
        void reloadPipeline() override;
        RenderQueue& getRenderQueue();

        // --- Render Flow ---
        void submitFrame(uint32_t imageIndex, uint32_t currentFrame);
        void presentFrame(uint32_t imageIndex, uint32_t currentFrame);

        // --- Window Access ---
        inline const AppWindow& getWindow() const { return window_; }
        GLFWwindow* getGLFWwindow() const { return window_.getGLFWwindow(); }

    private:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 3;

        // --- Core Subsystems ---
        AppWindow window_;
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

        // --- Shader Paths ---
        const std::string shaderPath_Vert = "shaders/point.vert.spv";
        const std::string shaderPath_Frag = "shaders/point.frag.spv";

        // --- Frame Management ---
        VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
        uint32_t imageIndex_ = 0;
        uint32_t currentFrame_ = 0;

        // --- Vertex Data ---
        std::vector<Vertex> activeVertices;
        std::vector<Vertex> pendingVertices;
        std::mutex vertexUpdateMutex_;
        bool vertexUpdatePending_ = false;

        // --- Scene + Commands ---
        Camera camera_;
        RenderQueue renderQueue_;

        // --- Internal Helpers ---
        bool shouldRecreateSwapchain(VkResult result) const;
        void recreateSwapchain();
        void updateUniforms(uint32_t imageIndex, const Camera& camera);
        void handleVertexUpdate();
        void processRenderQueue();
    };

} // namespace chionia

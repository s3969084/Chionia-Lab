#pragma once


// Window and Vulkan backend includes
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

namespace chionia {

    class VulkanCoordinator {
    public:
        void init();
        void drawFrame();   // <- new: draw a single frame
        void cleanup();

        const AppWindow& getWindow() const { return window_; } // <- new: access GLFW window



    private:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 3;

        size_t currentFrame_ = 0;
        uint32_t imageIndex_ = 0;

        // Core components
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

        // Shader paths
        const std::string shaderPath_Vert = "shaders/point.vert.spv";
        const std::string shaderPath_Frag = "shaders/point.frag.spv";

        std::vector<Vertex> demoVertices = {
            {0, glm::vec3(-0.5f, -0.5f, 0.0f)},
            {1, glm::vec3(0.5f, 0.5f, 0.0f)}
        };

        VulkanVertexBuffer vertexBuffer_;
    };

}

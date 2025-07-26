#pragma once

// Thread safety
#include <mutex>
#include <vector>

// Math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
#include "vulkan_backend/vulkan_uniform_buffer.hpp"

#include "engine/transform/camera.hpp"

namespace chionia {

    class VulkanCoordinator {
    public:
        void init();
        void drawFrame();
        void cleanup();

        const AppWindow& getWindow() const { return window_; }

        void updateVertices(const std::vector<Vertex>& vertices);




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
        VulkanVertexBuffer vertexBuffer_;
        UniformBuffer uniformBuffer_;

        // Testing
        Camera camera_;
        std::vector<Vertex> demoVertices;
        void generateTestPoints() {
            const int gridSize = 400;  // value x value = gridSize
            int id = 0;
            for (int x = -gridSize; x <= gridSize; ++x) {
                for (int y = -gridSize; y <= gridSize; ++y) {
                    float xf = static_cast<float>(x) / gridSize;
                    float yf = static_cast<float>(y) / gridSize;
                    demoVertices.push_back({ static_cast<uint32_t>(id++), glm::vec3(xf, yf, 0.0f) });

                }
            }
        }
        // End of testing

        // Shader paths
        const std::string shaderPath_Vert = "shaders/point.vert.spv";
        const std::string shaderPath_Frag = "shaders/point.frag.spv";


        uint32_t acquireNextImage(uint32_t currentFrame);
        void presentFrame(uint32_t imageIndex, uint32_t currentFrame);

        // Thread safety
        std::mutex vertexUpdateMutex_;
        bool vertexUpdatePending_ = false;
        std::vector<Vertex> pendingVertices_;

        VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;

        // --- Timing (for FPS) ---
        std::chrono::time_point<std::chrono::high_resolution_clock> fpsLastTime_ = std::chrono::high_resolution_clock::now();
        int fpsFrameCount_ = 0;


    };

}

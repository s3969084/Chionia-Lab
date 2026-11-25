#include "vulkan_imgui_renderer.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <stdexcept>

namespace chionia {

    VulkanImGuiRenderer::~VulkanImGuiRenderer() {
        shutdown();
    }

    void VulkanImGuiRenderer::initialize(
        GLFWwindow* window,
        VkInstance instance,
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        uint32_t queueFamily,
        VkQueue queue,
        VkRenderPass renderPass,
        uint32_t imageCount
    ) {
        if (contextCreated_) {
            throw std::logic_error(
                "VulkanImGuiRenderer is already initialized."
            );
        }

        if (
            window == nullptr ||
            instance == VK_NULL_HANDLE ||
            physicalDevice == VK_NULL_HANDLE ||
            device == VK_NULL_HANDLE ||
            queue == VK_NULL_HANDLE
        ) {
            throw std::invalid_argument(
                "VulkanImGuiRenderer received invalid Vulkan resources."
            );
        }

        instance_ = instance;
        physicalDevice_ = physicalDevice;
        device_ = device;
        queueFamily_ = queueFamily;
        queue_ = queue;

        try {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            contextCreated_ = true;

            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

#if defined(_WIN32)
            constexpr const char* systemFontPath =
                "C:/Windows/Fonts/segoeui.ttf";
#elif defined(__APPLE__)
            constexpr const char* systemFontPath =
                "/System/Library/Fonts/SFNS.ttf";
#else
            constexpr const char* systemFontPath =
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif
            ImFontConfig fontConfig{};
            fontConfig.OversampleH = 2;
            fontConfig.OversampleV = 2;
            if (ImFont* systemFont = io.Fonts->AddFontFromFileTTF(
                systemFontPath,
                17.0f,
                &fontConfig
            )) {
                io.FontDefault = systemFont;
            }

            if (!ImGui_ImplGlfw_InitForVulkan(window, true)) {
                throw std::runtime_error(
                    "Failed to initialize the ImGui GLFW backend."
                );
            }
            glfwBackendInitialized_ = true;

            createVulkanBackend(renderPass, imageCount);
            ImGui::StyleColorsDark();

            io.FontGlobalScale = 1.0f;
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 12.0f;
            style.ChildRounding = 9.0f;
            style.FrameRounding = 7.0f;
            style.GrabRounding = 7.0f;
            style.ScrollbarRounding = 8.0f;
            style.WindowBorderSize = 1.0f;
            style.ChildBorderSize = 1.0f;
            style.FrameBorderSize = 1.0f;
            style.ItemSpacing = ImVec2(9.0f, 7.0f);
            style.Colors[ImGuiCol_Text] = ImVec4(0.88f, 0.94f, 1.0f, 1.0f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.46f, 0.57f, 0.69f, 1.0f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.03f, 0.29f, 0.52f, 0.68f);
            style.Colors[ImGuiCol_ChildBg] = ImVec4(0.005f, 0.018f, 0.034f, 0.72f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.018f, 0.065f, 0.12f, 0.92f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.025f, 0.16f, 0.27f, 0.98f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.025f, 0.24f, 0.39f, 1.0f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.04f, 0.67f, 0.94f, 1.0f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2f, 0.85f, 1.0f, 1.0f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.08f, 0.82f, 1.0f, 1.0f);
        }
        catch (...) {
            shutdown();
            throw;
        }
    }

    void VulkanImGuiRenderer::createVulkanBackend(
        VkRenderPass renderPass,
        uint32_t imageCount
    ) {
        if (renderPass == VK_NULL_HANDLE) {
            throw std::invalid_argument(
                "Cannot initialize ImGui with a null render pass."
            );
        }

        if (imageCount < 2) {
            throw std::invalid_argument(
                "ImGui Vulkan backend requires at least two swapchain images."
            );
        }

        const VkDescriptorPoolSize poolSizes[] = {
            {
                VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE
            },
            {
                VK_DESCRIPTOR_TYPE_SAMPLER,
                IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE
            }
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets =
            IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE +
            IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE;
        poolInfo.poolSizeCount = 2;
        poolInfo.pPoolSizes = poolSizes;

        if (
            vkCreateDescriptorPool(
                device_,
                &poolInfo,
                nullptr,
                &descriptorPool_
            ) != VK_SUCCESS
        ) {
            throw std::runtime_error(
                "Failed to create ImGui Vulkan descriptor pool."
            );
        }

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.ApiVersion = VK_API_VERSION_1_2;
        initInfo.Instance = instance_;
        initInfo.PhysicalDevice = physicalDevice_;
        initInfo.Device = device_;
        initInfo.QueueFamily = queueFamily_;
        initInfo.Queue = queue_;
        initInfo.DescriptorPool = descriptorPool_;
        initInfo.MinImageCount = 2;
        initInfo.ImageCount = imageCount;
        initInfo.PipelineInfoMain.RenderPass = renderPass;
        initInfo.PipelineInfoMain.Subpass = 0;
        initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        if (!ImGui_ImplVulkan_Init(&initInfo)) {
            vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
            descriptorPool_ = VK_NULL_HANDLE;
            throw std::runtime_error(
                "Failed to initialize the ImGui Vulkan backend."
            );
        }

        vulkanBackendInitialized_ = true;
    }

    void VulkanImGuiRenderer::releaseVulkanBackend() {
        shutdownVulkanBackend();
    }

    void VulkanImGuiRenderer::initializeVulkanBackend(
        VkRenderPass renderPass,
        uint32_t imageCount
    ) {
        createVulkanBackend(renderPass, imageCount);
    }

    void VulkanImGuiRenderer::beginFrame() {
        if (!vulkanBackendInitialized_ || !glfwBackendInitialized_) {
            throw std::logic_error(
                "VulkanImGuiRenderer must be initialized before starting a frame."
            );
        }

        drawDataReady_ = false;
        frameStarted_ = true;
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void VulkanImGuiRenderer::finalizeFrame() {
        if (!contextCreated_ || !frameStarted_) {
            return;
        }

        ImGui::Render();
        frameStarted_ = false;
        drawDataReady_ = true;
    }

    void VulkanImGuiRenderer::recordDrawData(
        VkCommandBuffer commandBuffer
    ) {
        if (!drawDataReady_ || !vulkanBackendInitialized_) {
            return;
        }

        ImGui_ImplVulkan_RenderDrawData(
            ImGui::GetDrawData(),
            commandBuffer
        );
    }

    void VulkanImGuiRenderer::shutdownVulkanBackend() {
        if (vulkanBackendInitialized_) {
            ImGui_ImplVulkan_Shutdown();
            vulkanBackendInitialized_ = false;
        }

        if (
            descriptorPool_ != VK_NULL_HANDLE &&
            device_ != VK_NULL_HANDLE
        ) {
            vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
            descriptorPool_ = VK_NULL_HANDLE;
        }

        drawDataReady_ = false;
        frameStarted_ = false;
    }

    void VulkanImGuiRenderer::shutdown() {
        shutdownVulkanBackend();

        if (glfwBackendInitialized_) {
            ImGui_ImplGlfw_Shutdown();
            glfwBackendInitialized_ = false;
        }

        if (contextCreated_) {
            ImGui::DestroyContext();
            contextCreated_ = false;
        }

        instance_ = VK_NULL_HANDLE;
        physicalDevice_ = VK_NULL_HANDLE;
        device_ = VK_NULL_HANDLE;
        queueFamily_ = 0;
        queue_ = VK_NULL_HANDLE;
    }

}

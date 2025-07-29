#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

namespace chionia {

    class VulkanSwapchain {
    public:
        VulkanSwapchain() = default;
        ~VulkanSwapchain();

        void create(VkPhysicalDevice physicalDevice,VkDevice logicalDevice, VkSurfaceKHR surface,
            uint32_t graphicsQueueFamily, uint32_t presentQueueFamily, GLFWwindow* window);

        void destroy(VkDevice logicalDevice);

        VkSwapchainKHR get() const;
        const std::vector<VkImageView>& getImageViews() const;
        VkExtent2D getExtent() const {return extent_; }
        VkFormat getImageFormat() const { return imageFormat_;}

        uint32_t getImageCount() const { return imageCount_; }

    private:
        VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
        std::vector<VkImage> images_;
        std::vector<VkImageView> imageViews_;

        VkFormat imageFormat_;
        VkExtent2D extent_;

        VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes);
        VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window_);

        uint32_t imageCount_ = 0;
    };
}
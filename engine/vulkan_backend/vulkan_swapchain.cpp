#include "vulkan_swapchain.hpp"
#include <stdexcept>
#include <iostream>


namespace chionia {
    VulkanSwapchain::~VulkanSwapchain() {
        // Don't auto-destroy; call destroy() manually
    }

    void VulkanSwapchain::create(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkSurfaceKHR surface,
        uint32_t graphicsQueueFamily, uint32_t presentQueueFamily, GLFWwindow* window) {
        // Query capabilities
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

        VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);
        VkPresentModeKHR presentMode = choosePresentMode(presentModes);
        extent_ = chooseExtent(capabilities, window);
        imageFormat_ = surfaceFormat.format;

        uint32_t  imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = imageFormat_;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent_;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = { graphicsQueueFamily, presentQueueFamily };
        if (graphicsQueueFamily != presentQueueFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vulkan swapchain");
        }

        //Retrieve swapchain images
        uint32_t actualImageCount;
        vkGetSwapchainImagesKHR(logicalDevice, swapchain_, &actualImageCount, nullptr);
        images_.resize(actualImageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapchain_, &actualImageCount, images_.data());

        // Create image views
        imageViews_.resize(images_.size());
        for (size_t i = 0; i < images_.size(); ++i) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = images_[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = imageFormat_;
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageViews_[i]) != VK_SUCCESS) {
                throw std::runtime_error("❌ Failed to create image view for swapchain!");
            }
        }

        std::cout << "✅ Vulkan swapchain and image views created.\n";
    }

    void VulkanSwapchain::destroy(VkDevice logicalDevice) {


        for (auto view : imageViews_) {
            vkDestroyImageView(logicalDevice, view, nullptr);
        }
        imageViews_.clear();

        if (swapchain_ != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(logicalDevice, swapchain_, nullptr);
            swapchain_ = VK_NULL_HANDLE;
        }
    }

    VkSwapchainKHR VulkanSwapchain::get() const {
        return swapchain_;
    }

    const std::vector<VkImageView>& VulkanSwapchain::getImageViews() const {
        return imageViews_;
    }

    VkSurfaceFormatKHR VulkanSwapchain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
        for (const auto& format : formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        return formats[0];
    }

    VkPresentModeKHR VulkanSwapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
        for (const auto& mode : modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::max(capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }



}
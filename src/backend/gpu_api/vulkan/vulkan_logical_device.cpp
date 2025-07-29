#include "vulkan_logical_device.hpp"
#include <vector>
#include <stdexcept>
#include <iostream>
#include <set>

namespace chionia {

    VulkanLogicalDevice::~VulkanLogicalDevice() {
        destroy();
    }

    void VulkanLogicalDevice::create(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        findQueueFamilies(physicalDevice, surface);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { graphicsQueueFamily_, presentQueueFamily_ };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        // For macOS + portability
        const std::vector<const char*> deviceExtensions = {
            "VK_KHR_portability_subset",
            "VK_KHR_swapchain"
        };

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

#ifdef CHIONIA_DEBUG
        const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
            };
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
#else
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;


#endif

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical Vulkan device!");
        }

        vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);
        vkGetDeviceQueue(device_, presentQueueFamily_, 0, &presentQueue_);

        std::cout << "Logical Vulkan device and queues created.\n";
    }

    void VulkanLogicalDevice::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsQueueFamily_ = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                presentQueueFamily_ = i;
            }

            if (graphicsQueueFamily_ != UINT_MAX && presentQueueFamily_ != UINT_MAX) {
                break;
            }
        }

        if (graphicsQueueFamily_ == UINT32_MAX || presentQueueFamily_ == UINT32_MAX) {
            throw std::runtime_error("Failed to find suitable queue families!");
        }
    }

    void VulkanLogicalDevice::destroy() {
        if (device_ != VK_NULL_HANDLE) {
            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
        }
    }

    VkDevice VulkanLogicalDevice::get() const {
        return device_;
    }

    VkQueue VulkanLogicalDevice::getGraphicsQueue() const {
        return graphicsQueue_;
    }

    VkQueue VulkanLogicalDevice::getPresentQueue() const {
        return presentQueue_;
    }

    uint32_t VulkanLogicalDevice::getGraphicsQueueFamily() const {
        return graphicsQueueFamily_;
    }

    uint32_t VulkanLogicalDevice::getPresentQueueFamily() const {
        return presentQueueFamily_;
    }


}

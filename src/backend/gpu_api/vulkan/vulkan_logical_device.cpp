
#include "vulkan_logical_device.hpp"

#include <vector>
#include <stdexcept>
#include <iostream>
#include <set>
#include <cstring>

namespace chionia {

    VulkanLogicalDevice::~VulkanLogicalDevice() {
        destroy();
    }

    void VulkanLogicalDevice::create(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface
    ) {

        findQueueFamilies(physicalDevice, surface);

        // ====================================================
        // Queue creation
        // ====================================================

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        std::set<uint32_t> uniqueQueueFamilies = {
            graphicsQueueFamily_,
            presentQueueFamily_
        };

        float queuePriority = 1.0f;

        for (uint32_t queueFamily : uniqueQueueFamilies) {

            VkDeviceQueueCreateInfo queueCreateInfo{};

            queueCreateInfo.sType =
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            queueCreateInfos.push_back(queueCreateInfo);
        }

        // ====================================================
        // Device features
        // ====================================================

        VkPhysicalDeviceFeatures deviceFeatures{};

        // ====================================================
        // Enumerate supported device extensions
        // ====================================================

        uint32_t extensionCount = 0;

        vkEnumerateDeviceExtensionProperties(
            physicalDevice,
            nullptr,
            &extensionCount,
            nullptr
        );

        std::vector<VkExtensionProperties> availableExtensions(
            extensionCount
        );

        vkEnumerateDeviceExtensionProperties(
            physicalDevice,
            nullptr,
            &extensionCount,
            availableExtensions.data()
        );

        auto supportsExtension =
            [&availableExtensions](const char* extensionName) {

                for (const auto& extension : availableExtensions) {

                    if (std::strcmp(
                        extension.extensionName,
                        extensionName
                    ) == 0) {
                        return true;
                    }
                }

                return false;
            };

        // ====================================================
        // Required extensions
        // ====================================================

        std::vector<const char*> deviceExtensions;

        if (!supportsExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
            throw std::runtime_error(
                "Selected GPU does not support VK_KHR_swapchain!"
            );
        }

        deviceExtensions.push_back(
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        );

        // ====================================================
        // Optional portability extension
        // ====================================================

        if (supportsExtension("VK_KHR_portability_subset")) {

            deviceExtensions.push_back(
                "VK_KHR_portability_subset"
            );

            std::cout
                << "Portability subset supported and enabled.\n";
        }
        else {

            std::cout
                << "Native Vulkan device detected. "
                << "Portability subset not required.\n";
        }

        // ====================================================
        // Logical device creation
        // ====================================================

        VkDeviceCreateInfo createInfo{};

        createInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount =
            static_cast<uint32_t>(queueCreateInfos.size());

        createInfo.pQueueCreateInfos =
            queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount =
            static_cast<uint32_t>(deviceExtensions.size());

        createInfo.ppEnabledExtensionNames =
            deviceExtensions.data();

        // ====================================================
        // Validation layers
        // ====================================================

        // Validation layers are enabled at the Vulkan instance level.
        // They must not be enabled again during logical device creation.

        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;

        // ====================================================
        // Create Vulkan device
        // ====================================================

        VkResult result = vkCreateDevice(
            physicalDevice,
            &createInfo,
            nullptr,
            &device_
        );

        if (result != VK_SUCCESS) {

            throw std::runtime_error(
                "Failed to create logical Vulkan device! "
                "VkResult: " + std::to_string(result)
            );
        }

        // ====================================================
        // Retrieve queues
        // ====================================================

        vkGetDeviceQueue(
            device_,
            graphicsQueueFamily_,
            0,
            &graphicsQueue_
        );

        vkGetDeviceQueue(
            device_,
            presentQueueFamily_,
            0,
            &presentQueue_
        );

        std::cout
            << "Logical Vulkan device and queues created.\n";
    }

    // ========================================================
    // Queue family selection
    // ========================================================

    void VulkanLogicalDevice::findQueueFamilies(
        VkPhysicalDevice device,
        VkSurfaceKHR surface
    ) {

        graphicsQueueFamily_ = UINT32_MAX;
        presentQueueFamily_ = UINT32_MAX;

        uint32_t queueFamilyCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(
            device,
            &queueFamilyCount,
            nullptr
        );

        std::vector<VkQueueFamilyProperties> queueFamilies(
            queueFamilyCount
        );

        vkGetPhysicalDeviceQueueFamilyProperties(
            device,
            &queueFamilyCount,
            queueFamilies.data()
        );

        for (uint32_t i = 0; i < queueFamilyCount; ++i) {

            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsQueueFamily_ = i;
            }

            VkBool32 presentSupport = VK_FALSE;

            VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(
                device,
                i,
                surface,
                &presentSupport
            );

            if (result == VK_SUCCESS && presentSupport) {
                presentQueueFamily_ = i;
            }

            if (
                graphicsQueueFamily_ != UINT32_MAX &&
                presentQueueFamily_ != UINT32_MAX
            ) {
                break;
            }
        }

        if (
            graphicsQueueFamily_ == UINT32_MAX ||
            presentQueueFamily_ == UINT32_MAX
        ) {

            throw std::runtime_error(
                "Failed to find suitable queue families!"
            );
        }
    }

    // ========================================================
    // Cleanup
    // ========================================================

    void VulkanLogicalDevice::destroy() {

        if (device_ != VK_NULL_HANDLE) {

            vkDestroyDevice(device_, nullptr);

            device_ = VK_NULL_HANDLE;
            graphicsQueue_ = VK_NULL_HANDLE;
            presentQueue_ = VK_NULL_HANDLE;
        }
    }

    // ========================================================
    // Getters
    // ========================================================

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

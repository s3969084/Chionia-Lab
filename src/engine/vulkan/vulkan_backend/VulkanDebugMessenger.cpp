#include "VulkanDebugMessenger.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>

namespace chionia {
    void VulkanDebugMessenger::setup(VkInstance instance) {

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;

        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

        if (func != nullptr) {
            if (func(instance, &createInfo, nullptr, &messenger_) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create debug messenger!");
            }
        } else {
            throw std::runtime_error("Could not load vkCreateDebugUtilsMessengerEXT");
        }
    }

    void VulkanDebugMessenger::cleanup(VkInstance instance) {
        if (messenger_ != VK_NULL_HANDLE) {
            auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

            if (func != nullptr) {
                func(instance, messenger_, nullptr);
            }
            messenger_ = VK_NULL_HANDLE;
        }
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugMessenger::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* /*pUserData*/) {

        std::cerr << "[Vulkan Validation]"
        << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
}



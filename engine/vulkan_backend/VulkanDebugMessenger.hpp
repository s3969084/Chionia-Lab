#pragma once

#include <vulkan/vulkan.h>

namespace chionia {

    class VulkanDebugMessenger {
    public:
        VulkanDebugMessenger() = default;
        ~VulkanDebugMessenger() = default;

        void setup(VkInstance instance);
        void cleanup(VkInstance instance);

    private:
        VkDebugUtilsMessengerEXT messenger_{VK_NULL_HANDLE};

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);
    };
}
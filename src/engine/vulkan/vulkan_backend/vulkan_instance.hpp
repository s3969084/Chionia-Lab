#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace chionia {

    class VulkanInstance {
    public:
        VulkanInstance() = default;
        ~VulkanInstance();

        void create(const std::string& appName, bool enableValidation);
        void destroy();

        VkInstance get() const;

    private:
        VkInstance instance_{VK_NULL_HANDLE};

        std::vector<const char*> getRequiredExtensions(bool enableValidation) const;
        bool checkValidationLayerSupport() const;

#ifdef CHIONIA_DEBUG
        static const std::vector<const char*> kValidationLayers;
#endif
    };

}

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace chionia {

    class VulkanRenderer {
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer() = default;

        void create(VkDevice logicalDevice,
            VkExtent2D extent,
            VkRenderPass renderPass,
            const std::string& vertShaderPath,
            const std::string& fragShaderPath);

        void destroy(VkDevice logicalDevice);

        VkPipeline getPipeline() const { return graphicsPipeline_; }
        VkPipelineLayout getPipelineLayout() const { return pipelineLayout_;}
        VkShaderModule getVertShader() const { return vertShaderModule_; }
        VkShaderModule getFragShader() const { return fragShaderModule_; }

    private:
        VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
        VkShaderModule vertShaderModule_ = VK_NULL_HANDLE;
        VkShaderModule fragShaderModule_ = VK_NULL_HANDLE;

        static VkShaderModule createShaderModule(VkDevice logicalDevice, const std::string& filename);



    };
}
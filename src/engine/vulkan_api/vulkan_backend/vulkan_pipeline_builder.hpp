#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace chionia {

    class VulkanPipelineBuilder {
    public:
        VulkanPipelineBuilder() = default;
        ~VulkanPipelineBuilder() = default;

        void setShaderStages(VkShaderModule vertShader, VkShaderModule fragShader);
        void setInputAssembly(VkPrimitiveTopology topology);
        void setViewportAndScissor(VkExtent2D extent);
        void setPipelineLayout(VkPipelineLayout layout);
        void setRenderPass(VkRenderPass renderPass);
        VkPipeline build(VkDevice logicalDevice);

        // Utility function to create shader module from file
        static VkShaderModule createShaderModule(VkDevice device, const std::string& filename);

        void setVertexInput(const VkVertexInputBindingDescription& binding, const std::vector<VkVertexInputAttributeDescription>& attributes);

    private:
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages_;
        VkPipelineInputAssemblyStateCreateInfo inputAssembly_{};
        VkViewport viewport_{};
        VkRect2D scissor_{};
        VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
        VkRenderPass renderPass_ = VK_NULL_HANDLE;

        VkVertexInputBindingDescription bindingDescription_{};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions_;
        bool vertexInputSet_ = false;


    };

}

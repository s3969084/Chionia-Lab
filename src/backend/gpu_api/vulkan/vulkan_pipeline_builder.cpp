
#include "vulkan_pipeline_builder.hpp"

#include <fstream>
#include <vector>
#include <stdexcept>
#include <cstdint>

namespace chionia {

    void VulkanPipelineBuilder::setVertexInput(
        const VkVertexInputBindingDescription& binding,
        const std::vector<VkVertexInputAttributeDescription>& attributes
    ) {
        bindingDescription_ = binding;
        attributeDescriptions_ = attributes;
        vertexInputSet_ = true;
    }

    VkShaderModule VulkanPipelineBuilder::createShaderModule(
        VkDevice logicalDevice,
        const std::string& filename
    ) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error(
                "Failed to open shader file: " + filename
            );
        }

        const size_t fileSize = static_cast<size_t>(file.tellg());

        if (fileSize == 0 || fileSize % sizeof(uint32_t) != 0) {
            throw std::runtime_error(
                "Invalid SPIR-V shader file size: " + filename
            );
        }

        std::vector<char> buffer(fileSize);

        file.seekg(0);

        if (!file.read(
            buffer.data(),
            static_cast<std::streamsize>(fileSize)
        )) {
            throw std::runtime_error(
                "Failed to read shader file: " + filename
            );
        }

        file.close();

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = buffer.size();
        createInfo.pCode =
            reinterpret_cast<const uint32_t*>(buffer.data());

        VkShaderModule shaderModule = VK_NULL_HANDLE;

        if (vkCreateShaderModule(
            logicalDevice,
            &createInfo,
            nullptr,
            &shaderModule
        ) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create shader module: " + filename
            );
        }

        return shaderModule;
    }

    void VulkanPipelineBuilder::setShaderStages(
        VkShaderModule vertShader,
        VkShaderModule fragShader
    ) {
        shaderStages_.clear();

        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertShader;
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragShader;
        fragStage.pName = "main";

        shaderStages_.push_back(vertStage);
        shaderStages_.push_back(fragStage);
    }

    void VulkanPipelineBuilder::setInputAssembly(
        VkPrimitiveTopology topology
    ) {
        inputAssembly_.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

        inputAssembly_.topology = topology;
        inputAssembly_.primitiveRestartEnable = VK_FALSE;
    }

    void VulkanPipelineBuilder::setViewportAndScissor(VkExtent2D extent) {

        viewport_.x = 0.0f;
        viewport_.y = 0.0f;
        viewport_.width = static_cast<float>(extent.width);
        viewport_.height = static_cast<float>(extent.height);
        viewport_.minDepth = 0.0f;
        viewport_.maxDepth = 1.0f;

        scissor_.offset = {0, 0};
        scissor_.extent = extent;
    }

    void VulkanPipelineBuilder::setPipelineLayout(
        VkPipelineLayout layout
    ) {
        pipelineLayout_ = layout;
    }

    void VulkanPipelineBuilder::setRenderPass(
        VkRenderPass renderPass
    ) {
        renderPass_ = renderPass;
    }

    VkPipeline VulkanPipelineBuilder::build(VkDevice logicalDevice) {

        // VIEWPORT
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport_;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor_;

        // RASTERIZATION
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // MULTISAMPLING
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // ADDITIVE PARTICLE LIGHT BLENDING
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};

        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

        colorBlendAttachment.blendEnable = VK_TRUE;

        // RGB: accumulate emitted particle light.
        colorBlendAttachment.srcColorBlendFactor =
            VK_BLEND_FACTOR_ONE;

        colorBlendAttachment.dstColorBlendFactor =
            VK_BLEND_FACTOR_ONE;

        colorBlendAttachment.colorBlendOp =
            VK_BLEND_OP_ADD;

        // Alpha: accumulate particle coverage.
        colorBlendAttachment.srcAlphaBlendFactor =
            VK_BLEND_FACTOR_ONE;

        colorBlendAttachment.dstAlphaBlendFactor =
            VK_BLEND_FACTOR_ONE;

        colorBlendAttachment.alphaBlendOp =
            VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType =
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        // VERTEX INPUT
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        vertexInputInfo.vertexBindingDescriptionCount =
            vertexInputSet_ ? 1 : 0;

        vertexInputInfo.pVertexBindingDescriptions =
            vertexInputSet_ ? &bindingDescription_ : nullptr;

        vertexInputInfo.vertexAttributeDescriptionCount =
            static_cast<uint32_t>(attributeDescriptions_.size());

        vertexInputInfo.pVertexAttributeDescriptions =
            attributeDescriptions_.data();

        // GRAPHICS PIPELINE
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType =
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        pipelineInfo.stageCount =
            static_cast<uint32_t>(shaderStages_.size());

        pipelineInfo.pStages = shaderStages_.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly_;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;

        pipelineInfo.layout = pipelineLayout_;
        pipelineInfo.renderPass = renderPass_;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        VkPipeline pipeline = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(
            logicalDevice,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &pipeline
        ) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create graphics pipeline!"
            );
        }

        return pipeline;
    }

}

#include "vulkan_backend/vulkan_renderer.hpp"
#include "vulkan_backend/vulkan_pipeline_builder.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <fstream>

namespace chionia {


    void VulkanRenderer::create(VkDevice logicalDevice,
        VkExtent2D extent,
        VkRenderPass renderPass,
        const std::string& vertShaderPath,
        const std::string& fragShaderPath) {

        // Create shader module
        vertShaderModule_ = createShaderModule(logicalDevice, vertShaderPath);
        fragShaderModule_ = createShaderModule(logicalDevice, fragShaderPath);

        // Create pipeline layout
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        if (vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to create pipeline layout!");
        }

        // Build graphical pipeline
        VulkanPipelineBuilder builder;
        builder.setShaderStages(vertShaderModule_, fragShaderModule_);
        builder.setInputAssembly(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        builder.setViewportAndScissor(extent);
        builder.setPipelineLayout(pipelineLayout_);
        builder.setRenderPass(renderPass);

        graphicsPipeline_ = builder.build(logicalDevice);

        std::cout << "✅ VulkanRenderer pipeline created successfully.\n";
    }

    void VulkanRenderer::destroy(VkDevice logicalDevice) {
        if (graphicsPipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(logicalDevice, graphicsPipeline_, nullptr);
            graphicsPipeline_ = VK_NULL_HANDLE;
        }

        if (vertShaderModule_ != VK_NULL_HANDLE) {
            vkDestroyShaderModule(logicalDevice, vertShaderModule_, nullptr);
            vertShaderModule_ = VK_NULL_HANDLE;
        }

        if (fragShaderModule_ != VK_NULL_HANDLE) {
            vkDestroyShaderModule(logicalDevice, fragShaderModule_, nullptr);
            fragShaderModule_ = VK_NULL_HANDLE;
        }

        if (pipelineLayout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(logicalDevice, pipelineLayout_, nullptr);
            pipelineLayout_ = VK_NULL_HANDLE;
        }
    }

    VkShaderModule VulkanRenderer::createShaderModule(VkDevice logicalDevice, const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("❌ Failed to open shader file: " + filename);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = buffer.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to create shader module from file: " + filename);
        }

        return shaderModule;

    }



}
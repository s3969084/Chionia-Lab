#include "vulkan_renderer.hpp"
#include "vulkan_pipeline_builder.hpp"

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
        pipelineBuilder_.setShaderStages(vertShaderModule_, fragShaderModule_);
        pipelineBuilder_.setInputAssembly(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        pipelineBuilder_.setViewportAndScissor(extent);
        pipelineBuilder_.setPipelineLayout(pipelineLayout_);
        pipelineBuilder_.setRenderPass(renderPass);

        graphicsPipeline_ = pipelineBuilder_.build(logicalDevice);

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
        if (fileSize % 4 != 0) {
            throw std::runtime_error("❌ Shader file size is not aligned: " + filename);
        }
        std::vector<uint32_t> buffer(fileSize / 4);

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        file.close();

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = fileSize;
        createInfo.pCode = buffer.data();


        VkShaderModule shaderModule;




        if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to create shader module from file: " + filename);
        }

        return shaderModule;

    }



}
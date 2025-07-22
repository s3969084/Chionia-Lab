#include "vulkan_render_pass.hpp"
#include <stdexcept>
#include <iostream>

namespace chionia {

    void VulkanRenderPass::create(VkDevice logicalDevice, VkFormat swapchainImageFormat) {
        // ---1. Color Attachment ---
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = swapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // --- 2. Attachment Reference for subpass ---
        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // --- 3. Subpass ---
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        // --- 4. Subpass Dependency ---
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // --- 5. Create Info ---
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        // --- 6. Create ---
        if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
            throw std::runtime_error("❌ Failed to create render pass!");
        }

        std::cout <<  "✅ Vulkan render pass created.\n";
    }

    void VulkanRenderPass::destroy(VkDevice logicalDevice) {
        if (renderPass_ != VK_NULL_HANDLE) {
            vkDestroyRenderPass(logicalDevice, renderPass_, nullptr);
            renderPass_ = VK_NULL_HANDLE;
        }
    }




}
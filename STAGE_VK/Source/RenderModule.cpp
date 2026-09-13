#include "RenderModule.h"
#include "VulkanModule.h"

#include <stdexcept>

RenderModule::RenderModule(VulkanModule& vulkanModule) : vulkanModule(vulkanModule)
{
}

bool RenderModule::Init()
{
    CreateRenderPass();
    CreateFramebuffers();

    vulkanModule.RegisterSwapchainListener(this);

    return true;
}

void RenderModule::Render()
{
    if (!vulkanModule.IsFrameReady())
    {
        return;
    }

    VkCommandBuffer commandBuffer = vulkanModule.GetCommandBuffer();
    uint32_t imageIndex = vulkanModule.GetCurrentImageIndex();

    VkClearValue clearValue{};
    clearValue.color = { { 0.0f, 1.0f, 1.0f, 1.0f } };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vulkanModule.GetSwapchainExtent();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Future rendering passes will record work here.

    vkCmdEndRenderPass(commandBuffer);
}

bool RenderModule::CleanUp()
{
    vulkanModule.UnregisterSwapchainListener(this);

    DestroyRenderTargets();

    return true;
}

void RenderModule::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vulkanModule.GetSwapchainImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &colorAttachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(vulkanModule.GetDevice(), &createInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan render pass.");
    }
}

void RenderModule::CreateFramebuffers()
{
    const std::vector<VkImageView>& imageViews = vulkanModule.GetSwapchainImageViews();
    const VkExtent2D extent = vulkanModule.GetSwapchainExtent();

    swapchainFramebuffers.resize(imageViews.size(), VK_NULL_HANDLE);

    for (size_t i = 0; i < imageViews.size(); ++i)
    {
        VkImageView attachments[] = { imageViews[i] };

        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = attachments;
        createInfo.width = extent.width;
        createInfo.height = extent.height;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(vulkanModule.GetDevice(), &createInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS)
        {
            DestroyRenderTargets();
            throw std::runtime_error("Failed to create Vulkan framebuffer.");
        }
    }
}

void RenderModule::DestroyRenderTargets()
{
    VkDevice device = vulkanModule.GetDevice();

    for (VkFramebuffer framebuffer : swapchainFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }

    swapchainFramebuffers.clear();

    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }
}

void RenderModule::BeforeSwapchainRecreate()
{
    DestroyRenderTargets();
}

void RenderModule::AfterSwapchainRecreate()
{
    CreateRenderPass();
    CreateFramebuffers();
}
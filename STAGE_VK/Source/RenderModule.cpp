#include "RenderModule.h"
#include "VulkanModule.h"

#include "Globals.h"
#include "Application.h"
#include "Window.h"
#include "Log.h"
#include "ProfileScope.h"

#include <stdexcept>

RenderModule::RenderModule(VulkanModule& vulkanModule) : vulkanModule(vulkanModule)
{
}

bool RenderModule::Init()
{
    CreateRenderPass();
    CreateFramebuffers();

    ImGuiPassInitInfo imguiInfo{};
    imguiInfo.window = app->GetWindow().GetNativeHandle();
    imguiInfo.apiVersion = vulkanModule.GetApiVersion();
    imguiInfo.instance = vulkanModule.GetInstance();
    imguiInfo.physicalDevice = vulkanModule.GetPhysicalDevice();
    imguiInfo.device = vulkanModule.GetDevice();
    imguiInfo.graphicsQueueFamily = vulkanModule.GetGraphicsQueueFamily();
    imguiInfo.graphicsQueue = vulkanModule.GetGraphicsQueue();
    imguiInfo.imageCount = vulkanModule.GetSwapchainImageCount();
    imguiInfo.renderPass = renderPass;

    if (!imguiPass.Init(imguiInfo))
    {
        return false;
    }

    vulkanModule.RegisterSwapchainListener(this);

    return true;
}

void RenderModule::PreRender()
{
    PROFILE_SCOPE(app->GetProfiler(), "RenderModule::PreRender");

    frameActive = false;

    if (!vulkanModule.IsFrameReady())
    {
        return;
    }

    imguiPass.StartFrame();
    frameActive = true;
}

void RenderModule::Render()
{
    PROFILE_SCOPE(app->GetProfiler(), "RenderModule::PostRender");

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

    app->GetProfiler().RecordRenderPass();

    renderPassOpen = true;
}

void RenderModule::PostRender()
{
    if (!vulkanModule.IsFrameReady() || !renderPassOpen)
    {
        return;
    }

    VkCommandBuffer commandBuffer = vulkanModule.GetCommandBuffer();

    {
        PROFILE_SCOPE(app->GetProfiler(), "ImGuiPass::Record");
        imguiPass.Record(commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);

    renderPassOpen = false;
    frameActive = false;
}

bool RenderModule::CleanUp()
{
    vulkanModule.WaitIdle();

    vulkanModule.UnregisterSwapchainListener(this);

    imguiPass.CleanUp();
    DestroyRenderTargets();

    renderPassOpen = false;
    frameActive = false;

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
    renderPassOpen = false;
    frameActive = false;

    imguiPass.BeforeSwapchainRecreate();
    DestroyRenderTargets();
}

void RenderModule::AfterSwapchainRecreate()
{
    CreateRenderPass();
    CreateFramebuffers();

    imguiPass.AfterSwapchainRecreate(renderPass, vulkanModule.GetSwapchainImageCount());
}
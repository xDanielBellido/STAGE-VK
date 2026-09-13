#pragma once

#include "Module.h"
#include "IVulkanSwapchainListener.h"

#include "ImGuiPass.h"

#include <vulkan/vulkan.h>

#include <vector>

class VulkanModule;

class RenderModule final : public Module, public IVulkanSwapchainListener
{
public:
    explicit RenderModule(VulkanModule& vulkanModule);

    bool Init() override;
    void PreRender() override;
    void Render() override;
    void PostRender() override;
    bool CleanUp() override;

    void BeforeSwapchainRecreate() override;
    void AfterSwapchainRecreate() override;

private:
    void CreateRenderPass();
    void CreateFramebuffers();
    void DestroyRenderTargets();

private:
    VulkanModule& vulkanModule;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapchainFramebuffers;

    ImGuiPass imguiPass;
    bool renderPassOpen = false;
};
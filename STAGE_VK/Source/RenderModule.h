/*
    ==========================================================================
    RenderModule
    ==========================================================================

    This class is the high-level rendering orchestrator of the engine.

    Its main responsibility is to coordinate the rendering work recorded
    during each frame:
    - create and manage the main Vulkan render pass
    - create framebuffers for the swapchain images
    - coordinate rendering passes such as ImGuiPass
    - open and close the main render pass
    - react to swapchain recreation events
    - expose whether the current frame is active

    RenderModule implements IVulkanSwapchainListener so VulkanModule can notify
    it when the swapchain is recreated without depending directly on the
    renderer layer.

    RenderModule decides WHAT rendering work is recorded and in which order.
    VulkanModule remains responsible for submitting that work to the GPU.
*/
#pragma once

#include "Module.h"
#include "IVulkanSwapchainListener.h"

#include "ImGuiPass.h"
#include "GeometryPass.h"

#include <vulkan/vulkan.h>

#include <vector>

class VulkanModule;

class RenderModule final : public Module, public IVulkanSwapchainListener
{
private:
    VulkanModule&                   vulkanModule;

    VkRenderPass                    renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer>      swapchainFramebuffers;

    ImGuiPass                       imguiPass;
    GeometryPass                    geometryPass;

    bool                            renderPassOpen = false;
    bool                            frameActive = false;

public:
                                    // Construction
    explicit                        RenderModule(VulkanModule& vulkanModule);

                                    // Module Lifetime
    bool                            Init() override;
    void                            PreRender() override;
    void                            Render() override;
    void                            PostRender() override;
    bool                            CleanUp() override;

                                    // Swapchain Listener
    void                            BeforeSwapchainRecreate() override;
    void                            AfterSwapchainRecreate() override;

                                    // Frame State
    bool                            IsFrameActive() const { return frameActive; }

private:
                                    // Render Targets
    void                            CreateRenderPass();
    void                            CreateFramebuffers();
    void                            DestroyRenderTargets();
};
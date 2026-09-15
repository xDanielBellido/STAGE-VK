/*
    ===========================================================================
    IVulkanSwapchainListener
    ===========================================================================

    A small interface used by systems that need to react when VulkanModule
    recreates the swapchain.

    VulkanModule should not depend directly on higher-level renderer classes
    such as RenderModule. Instead, interested systems implement this interface
    and register themselves as listeners.

    It keeps the Vulkan backend decoupled while still allowing renderer systems
    to safely destroy and rebuild swapchain-dependent resources before and
    after recreation.

    Current flow:

        VulkanModule
            | notifies
        IVulkanSwapchainListener
            | implemented by
        RenderModule
            | orchestrates
        swapchain-dependent renderer resources
*/

#pragma once

class IVulkanSwapchainListener
{
public:

    virtual ~IVulkanSwapchainListener() = default;

    // Called before VulkanModule destroys the current swapchain.
    // Implementations should release resources that depend on the old
    // swapchain or its image views.
    virtual void BeforeSwapchainRecreate() = 0;

    // Called after VulkanModule has successfully created the new swapchain.
    // Implementations should recreate their swapchain-dependent resources.
    virtual void AfterSwapchainRecreate() = 0;
};
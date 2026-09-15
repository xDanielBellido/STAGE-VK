/*
    ==========================================================================
    VulkanCommandContext
    ==========================================================================

    This class manages the Vulkan command resources used to record GPU work.

    A command pool owns the memory from which command buffers are allocated.
    A command buffer stores recorded Vulkan commands before they are submitted
    to a queue for execution by the GPU.

    This class is responsible for:
    - creating the graphics command pool
    - allocating the primary command buffer used by the renderer
    - exposing the command resources to VulkanModule
    - releasing command resources during shutdown

    VkDevice is provided by VulkanDevice and is not owned by this class.
*/

#pragma once

#include <vulkan/vulkan.h>

class VulkanCommandContext
{
private:
                                    // Non-owned Vulkan Handle
    VkDevice                        device = VK_NULL_HANDLE;

                                    // Command Resources
    VkCommandPool                   commandPool = VK_NULL_HANDLE;
    VkCommandBuffer                 commandBuffer = VK_NULL_HANDLE;

public:
                                    // Command Context Lifetime
    bool                            Init(VkDevice device, uint32_t graphicsQueueFamily);
    void                            CleanUp();

                                    // Command Resources Access
    VkCommandPool                   GetCommandPool() const { return commandPool; }
    VkCommandBuffer                 GetCommandBuffer() const { return commandBuffer; }

private:
                                    // Command Resource Creation
    void                            CreateCommandPool(uint32_t graphicsQueueFamily);
    void                            AllocateCommandBuffer();
};
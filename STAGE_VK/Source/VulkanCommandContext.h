/*
    ===========================================================================
    VulkanCommandContext
    ===========================================================================

    This class manages the Vulkan objects used to record GPU commands.

    A command pool manages the memory used by command buffers.
    A command buffer stores recorded GPU commands before they are submitted
    to a Vulkan queue for execution.

    This class is responsible for:
    - creating the graphics command pool
    - allocating a primary command buffer
    - releasing command resources during shutdown

    VkDevice is provided by VulkanDevice and is not owned by this class.
*/

#pragma once

#include <vulkan/vulkan.h>

class VulkanCommandContext
{
public:
    bool Init(VkDevice device, uint32_t graphicsQueueFamily);
    void CleanUp();

    VkCommandPool GetCommandPool() const { return commandPool; }
    VkCommandBuffer GetCommandBuffer() const { return commandBuffer; }

private:
    void CreateCommandPool(uint32_t graphicsQueueFamily);
    void AllocateCommandBuffer();

private:
    // Non-owned handle provided by VulkanDevice
    VkDevice device = VK_NULL_HANDLE;

    // Command resources
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
};

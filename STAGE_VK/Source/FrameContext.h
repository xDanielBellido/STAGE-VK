/*
    ===========================================================================
    FrameContext
    ===========================================================================

    This class stores the synchronization objects required for one frame.

    Vulkan uses semaphores to synchronize GPU operations and fences to
    synchronize the CPU with the GPU.

    Each frame context contains:
    - a semaphore signaled when a swapchain image becomes available
    - a semaphore signaled when rendering has finished
    - a fence used by the CPU to know when the GPU has finished the frame

    VkDevice is provided by VulkanDevice and is not owned by this class.
*/

#pragma once

#include <vulkan/vulkan.h>

class FrameContext
{
public:
    bool Init(VkDevice device);
    void CleanUp();

    VkSemaphore GetImageAvailableSemaphore() const { return imageAvailableSemaphore; }
    VkSemaphore GetRenderFinishedSemaphore() const { return renderFinishedSemaphore; }
    VkFence GetInFlightFence() const { return inFlightFence; }

private:
    VkDevice device = VK_NULL_HANDLE;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
};

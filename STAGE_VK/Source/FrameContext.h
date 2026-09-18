/*
    ==========================================================================
    FrameContext
    ==========================================================================

    This class manages the synchronization objects used by the current Vulkan
    frame lifecycle.

    Vulkan uses semaphores to synchronize GPU operations and fences to
    synchronize CPU execution with GPU completion.

    This class is responsible for:
    - creating the semaphore used to signal swapchain image availability
    - creating one render-finished semaphore per swapchain image
    - creating the fence used to track completion of submitted GPU work
    - exposing these synchronization objects to VulkanModule
    - releasing synchronization resources during shutdown

    VkDevice is provided by VulkanDevice and is not owned by this class.

    The render-finished semaphores are indexed by swapchain image because a
    presented image may still be using its semaphore when another image is
    acquired for rendering.
*/

#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class FrameContext
{
private:
                                    // Non-owned Vulkan Handle
    VkDevice                        device = VK_NULL_HANDLE;

                                    // Frame Synchronization
    std::vector<VkSemaphore>        imageAvailableSemaphores;
    std::vector<VkSemaphore>        renderFinishedSemaphores;
    std::vector<VkFence>            inFlightFences;

public:
                                    // Frame Context Lifetime
    bool                            Init(VkDevice device, uint32_t swapchainImageCount, uint32_t frameCount);
    void                            CleanUp();

                                    // Synchronization Access
    VkSemaphore                     GetImageAvailableSemaphore(uint32_t frameIndex) const { return imageAvailableSemaphores[frameIndex]; }
    VkSemaphore                     GetRenderFinishedSemaphore(uint32_t imageIndex) const { return renderFinishedSemaphores[imageIndex]; }
    VkFence                         GetInFlightFence(uint32_t frameIndex) const { return inFlightFences[frameIndex]; }
};
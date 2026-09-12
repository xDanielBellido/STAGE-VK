#include "FrameContext.h"

#include <stdexcept>

bool FrameContext::Init(VkDevice device, uint32_t swapchainImageCount)
{
    this->device = device;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // 1. Create the semaphore used when acquiring a swapchain image.
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan image available semaphore.");
    }

    // 2. Create one render-finished semaphore for each swapchain image.
    // Presentation may still be using the semaphore associated with a previously
    // presented image, so these semaphores are indexed by swapchain image.
    renderFinishedSemaphores.resize(swapchainImageCount, VK_NULL_HANDLE);

    for (VkSemaphore& semaphore : renderFinishedSemaphores)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
        {
            CleanUp();
            throw std::runtime_error("Failed to create Vulkan render finished semaphore.");
        }
    }

    // 3. Create the fence used to synchronize the CPU with GPU work.
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
    {
        CleanUp();
        throw std::runtime_error("Failed to create Vulkan in-flight fence.");
    }

    return true;
}

void FrameContext::CleanUp()
{
    if (inFlightFence != VK_NULL_HANDLE)
    {
        vkDestroyFence(device, inFlightFence, nullptr);
        inFlightFence = VK_NULL_HANDLE;
    }

    for (VkSemaphore semaphore : renderFinishedSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }

    renderFinishedSemaphores.clear();

    if (imageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        imageAvailableSemaphore = VK_NULL_HANDLE;
    }

    device = VK_NULL_HANDLE;
}
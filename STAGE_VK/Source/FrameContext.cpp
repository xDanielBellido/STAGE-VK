#include "FrameContext.h"

#include <stdexcept>

bool FrameContext::Init(VkDevice device, uint32_t swapchainImageCount, uint32_t frameCount)
{
    this->device = device;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    imageAvailableSemaphores.resize(frameCount, VK_NULL_HANDLE);

    for (VkSemaphore& semaphore : imageAvailableSemaphores)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
        {
            CleanUp();
            throw std::runtime_error("Failed to create Vulkan image available semaphore.");
        }
    }

    renderFinishedSemaphores.resize(swapchainImageCount, VK_NULL_HANDLE);

    for (VkSemaphore& semaphore : renderFinishedSemaphores)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
        {
            CleanUp();
            throw std::runtime_error("Failed to create Vulkan render finished semaphore.");
        }
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    inFlightFences.resize(frameCount, VK_NULL_HANDLE);

    for (VkFence& fence : inFlightFences)
    {
        if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
        {
            CleanUp();
            throw std::runtime_error("Failed to create Vulkan in-flight fence.");
        }
    }

    return true;
}

void FrameContext::CleanUp()
{
    for (VkFence fence : inFlightFences)
    {
        if (fence != VK_NULL_HANDLE)
            vkDestroyFence(device, fence, nullptr);
    }

    inFlightFences.clear();

    for (VkSemaphore semaphore : renderFinishedSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
            vkDestroySemaphore(device, semaphore, nullptr);
    }

    renderFinishedSemaphores.clear();

    for (VkSemaphore semaphore : imageAvailableSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
            vkDestroySemaphore(device, semaphore, nullptr);
    }

    imageAvailableSemaphores.clear();

    device = VK_NULL_HANDLE;
}
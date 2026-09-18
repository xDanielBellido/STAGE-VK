#include "VulkanCommandContext.h"

#include <stdexcept>

bool VulkanCommandContext::Init(VkDevice device, uint32_t graphicsQueueFamily, uint32_t frameCount)
{
    this->device = device;

    CreateCommandPool(graphicsQueueFamily);
    AllocateCommandBuffers(frameCount);

    return true;
}

void VulkanCommandContext::CleanUp()
{
    if (commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, commandPool, nullptr);
        commandPool = VK_NULL_HANDLE;
    }

    commandBuffers.clear();
    device = VK_NULL_HANDLE;
}

void VulkanCommandContext::CreateCommandPool(uint32_t graphicsQueueFamily)
{
    // A command pool manages the memory used to store recorded commands.
    // Command buffers allocated from this pool will later be submitted
    // to a queue belonging to the graphics queue family.

    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = graphicsQueueFamily;

    if (vkCreateCommandPool(device, &createInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan command pool.");
    }
}

void VulkanCommandContext::AllocateCommandBuffers(uint32_t frameCount)
{
    commandBuffers.resize(frameCount, VK_NULL_HANDLE);

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = frameCount;

    if (vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Vulkan command buffers.");
    }
}

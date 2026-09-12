#include "VulkanCommandContext.h"

#include <stdexcept>

bool VulkanCommandContext::Init(VkDevice device, uint32_t graphicsQueueFamily)
{
    this->device = device;

    CreateCommandPool(graphicsQueueFamily);
    AllocateCommandBuffer();

    return true;
}

void VulkanCommandContext::CleanUp()
{
    if (commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, commandPool, nullptr);
        commandPool = VK_NULL_HANDLE;
    }

    commandBuffer = VK_NULL_HANDLE;
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

void VulkanCommandContext::AllocateCommandBuffer()
{
    // A command buffer stores GPU commands while they are being recorded.
    // PRIMARY command buffers can be submitted directly to a Vulkan queue.

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Vulkan command buffer.");
    }
}

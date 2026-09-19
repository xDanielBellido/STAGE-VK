#include "VulkanUploadContext.h"

#include <stdexcept>

VulkanUploadContext::~VulkanUploadContext()
{
    CleanUp();
}

bool VulkanUploadContext::Init(VkDevice device, VkQueue queue, uint32_t queueFamily)
{
    if (this->device != VK_NULL_HANDLE)
    {
        throw std::logic_error("Vulkan upload context is already initialized.");
    }

    if (device == VK_NULL_HANDLE || queue == VK_NULL_HANDLE)
    {
        throw std::invalid_argument("Invalid Vulkan handles provided to VulkanUploadContext.");
    }

    this->device = device;
    this->queue = queue;

    try
    {
        CreateCommandResources(queueFamily);
        CreateFence();
    }
    catch (...)
    {
        CleanUp();
        throw;
    }

    return true;
}

void VulkanUploadContext::CleanUp()
{
    if (fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(device, fence, nullptr);
        fence = VK_NULL_HANDLE;
    }

    if (commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, commandPool, nullptr);
        commandPool = VK_NULL_HANDLE;
        commandBuffer = VK_NULL_HANDLE;
    }

    queue = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
}

void VulkanUploadContext::UploadBuffer(const VulkanAllocator& allocator, VulkanBuffer& destination, const void* data, VkDeviceSize dataSize, VkPipelineStageFlags destinationStageMask, VkAccessFlags destinationAccessMask, VkDeviceSize destinationOffset)
{
    if (device == VK_NULL_HANDLE || queue == VK_NULL_HANDLE || commandPool == VK_NULL_HANDLE || commandBuffer == VK_NULL_HANDLE || fence == VK_NULL_HANDLE)
    {
        throw std::logic_error("Vulkan upload context is not initialized.");
    }

    if (!allocator.IsValid())
    {
        throw std::invalid_argument("Cannot upload without a valid Vulkan allocator.");
    }

    if (!destination.IsValid())
    {
        throw std::invalid_argument("Cannot upload to an invalid Vulkan buffer.");
    }

    if ((destination.GetUsage() & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0)
    {
        throw std::invalid_argument("Vulkan upload destination must support transfer destination usage.");
    }

    if (data == nullptr || dataSize == 0)
    {
        throw std::invalid_argument("Vulkan buffer upload requires valid data and a non-zero size.");
    }

    if (destinationOffset > destination.GetSize() || dataSize > destination.GetSize() - destinationOffset)
    {
        throw std::out_of_range("Vulkan buffer upload exceeds destination buffer bounds.");
    }

    if (destinationStageMask == 0 || destinationAccessMask == 0)
    {
        throw std::invalid_argument("Vulkan buffer upload requires valid destination synchronization masks.");
    }

    VulkanBuffer stagingBuffer;
    stagingBuffer.InitHostWritable(allocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    stagingBuffer.Write(data, dataSize);

    if (vkResetFences(device, 1, &fence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to reset Vulkan upload fence.");
    }

    if (vkResetCommandPool(device, commandPool, 0) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to reset Vulkan upload command pool.");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin Vulkan upload command buffer.");
    }

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = destinationOffset;
    copyRegion.size = dataSize;

    vkCmdCopyBuffer(commandBuffer, stagingBuffer.GetBuffer(), destination.GetBuffer(), 1, &copyRegion);

    VkBufferMemoryBarrier bufferBarrier{};
    bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    bufferBarrier.dstAccessMask = destinationAccessMask;
    bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.buffer = destination.GetBuffer();
    bufferBarrier.offset = destinationOffset;
    bufferBarrier.size = dataSize;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, destinationStageMask, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to end Vulkan upload command buffer.");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit Vulkan buffer upload.");
    }

    const VkResult waitResult = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    if (waitResult != VK_SUCCESS)
    {
        vkQueueWaitIdle(queue);
        throw std::runtime_error("Failed while waiting for Vulkan buffer upload.");
    }
}

void VulkanUploadContext::CreateCommandResources(uint32_t queueFamily)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = queueFamily;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan upload command pool.");
    }

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Vulkan upload command buffer.");
    }
}

void VulkanUploadContext::CreateFence()
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan upload fence.");
    }
}
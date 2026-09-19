#include "VulkanBuffer.h"

#include <stdexcept>

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
{
    MoveFrom(other);
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept
{
    if (this != &other)
    {
        CleanUp();
        MoveFrom(other);
    }

    return *this;
}

bool VulkanBuffer::Init(const VulkanAllocator& allocatorOwner, VkDeviceSize bufferSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocationFlags)
{
    if (IsValid())
    {
        throw std::logic_error("Vulkan buffer is already initialized.");
    }

    if (!allocatorOwner.IsValid())
    {
        throw std::invalid_argument("Cannot create Vulkan buffer without a valid allocator.");
    }

    if (bufferSize == 0 || usage == 0)
    {
        throw std::invalid_argument("Vulkan buffer size and usage must be valid.");
    }

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocationInfo{};
    allocationInfo.flags = allocationFlags;
    allocationInfo.usage = memoryUsage;

    VkBuffer createdBuffer = VK_NULL_HANDLE;
    VmaAllocation createdAllocation = nullptr;

    const VmaAllocator allocatorHandle = allocatorOwner.GetHandle();

    if (vmaCreateBuffer(allocatorHandle, &bufferInfo, &allocationInfo, &createdBuffer, &createdAllocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan buffer.");
    }

    allocator = allocatorHandle;
    buffer = createdBuffer;
    allocation = createdAllocation;
    size = bufferSize;

    return true;
}

void VulkanBuffer::CleanUp()
{
    if (buffer != VK_NULL_HANDLE && allocation != nullptr && allocator != nullptr)
    {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }

    buffer = VK_NULL_HANDLE;
    allocation = nullptr;
    allocator = nullptr;
    size = 0;
}

void VulkanBuffer::MoveFrom(VulkanBuffer& other) noexcept
{
    allocator = other.allocator;
    buffer = other.buffer;
    allocation = other.allocation;
    size = other.size;

    other.allocator = nullptr;
    other.buffer = VK_NULL_HANDLE;
    other.allocation = nullptr;
    other.size = 0;
}
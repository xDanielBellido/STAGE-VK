#include "VulkanBuffer.h"

#include <stdexcept>

VulkanBuffer::~VulkanBuffer()
{
    CleanUp();
}

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

    VkMemoryPropertyFlags createdMemoryProperties = 0;
    vmaGetAllocationMemoryProperties(allocatorHandle, createdAllocation, &createdMemoryProperties);

    allocator = allocatorHandle;
    buffer = createdBuffer;
    allocation = createdAllocation;
    size = bufferSize;
    this->usage = bufferInfo.usage;
    memoryProperties = createdMemoryProperties;

    return true;
}

bool VulkanBuffer::InitHostWritable(const VulkanAllocator& allocatorOwner, VkDeviceSize bufferSize, VkBufferUsageFlags usage)
{
    Init(allocatorOwner, bufferSize, usage, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    if (!IsHostVisible())
    {
        CleanUp();
        throw std::runtime_error("Failed to create a host-visible Vulkan buffer.");
    }

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
    usage = 0;
    memoryProperties = 0;
}

void VulkanBuffer::Write(const void* data, VkDeviceSize dataSize, VkDeviceSize offset)
{
    if (!IsValid())
    {
        throw std::logic_error("Cannot write to an uninitialized Vulkan buffer.");
    }

    if (!IsHostVisible())
    {
        throw std::logic_error("Cannot write to Vulkan buffer memory that is not host-visible.");
    }

    if (data == nullptr || dataSize == 0)
    {
        throw std::invalid_argument("Vulkan buffer write requires valid data and a non-zero size.");
    }

    if (offset > size || dataSize > size - offset)
    {
        throw std::out_of_range("Vulkan buffer write exceeds buffer bounds.");
    }

    if (vmaCopyMemoryToAllocation(allocator, data, allocation, offset, dataSize) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to write data to Vulkan buffer.");
    }
}

void VulkanBuffer::MoveFrom(VulkanBuffer& other) noexcept
{
    allocator = other.allocator;
    buffer = other.buffer;
    allocation = other.allocation;
    size = other.size;
    usage = other.usage;
    memoryProperties = other.memoryProperties;

    other.allocator = nullptr;
    other.buffer = VK_NULL_HANDLE;
    other.allocation = nullptr;
    other.size = 0;
    other.usage = 0;
    other.memoryProperties = 0;
}
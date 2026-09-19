/*
    ==========================================================================
    VulkanBuffer
    ==========================================================================

    This class owns a Vulkan buffer and the memory allocation backing it.

    VulkanBuffer is responsible for:
    - creating a VkBuffer with explicit size and usage flags
    - obtaining its memory through VulkanAllocator
    - owning both the buffer and its VMA allocation
    - creating buffers that can be written from the CPU when required
    - writing data to host-visible memory with bounds validation
    - tracking the memory properties selected by VMA
    - releasing the buffer and its allocation together
    - transferring ownership through move operations

    The allocator is not owned by this class and must outlive every buffer
    created from it.

    VulkanBuffer is movable but not copyable.

    CPU writes do not synchronize against GPU usage. The caller must ensure
    that the buffer is not being accessed by the GPU while it is overwritten.

    CleanUp() must only be called once GPU work using the buffer has completed.
*/

#pragma once

#include "VulkanAllocator.h"

class VulkanBuffer
{
private:
    // Non-owned Allocator
    VmaAllocator                    allocator = nullptr;

    // Owned Buffer Resource
    VkBuffer                        buffer = VK_NULL_HANDLE;
    VmaAllocation                   allocation = nullptr;
    VkDeviceSize                    size = 0;

    // Allocation Properties
    VkMemoryPropertyFlags           memoryProperties = 0;

public:
    // Buffer Lifetime
    VulkanBuffer() = default;
    ~VulkanBuffer();

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

    bool                            Init(const VulkanAllocator& allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO, VmaAllocationCreateFlags allocationFlags = 0);
    bool                            InitHostWritable(const VulkanAllocator& allocator, VkDeviceSize size, VkBufferUsageFlags usage);
    void                            CleanUp();

    // CPU Access
    void                            Write(const void* data, VkDeviceSize dataSize, VkDeviceSize offset = 0);

    // Buffer Access
    VkBuffer                        GetBuffer() const { return buffer; }
    VkDeviceSize                    GetSize() const { return size; }
    bool                            IsValid() const { return buffer != VK_NULL_HANDLE && allocation != nullptr; }

    // Memory Properties
    bool                            IsHostVisible() const { return (memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0; }
    bool                            IsHostCoherent() const { return (memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0; }

private:
    // Ownership Transfer
    void                            MoveFrom(VulkanBuffer& other) noexcept;
};
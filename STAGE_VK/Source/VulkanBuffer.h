/*
    ==========================================================================
    VulkanBuffer
    ==========================================================================

    This class owns a Vulkan buffer and the memory allocation backing it.

    VulkanBuffer is responsible for:
    - creating a VkBuffer with explicit size and usage flags
    - obtaining its memory through VulkanAllocator
    - owning both the buffer and its VMA allocation
    - releasing both together
    - transferring ownership through move operations

    The allocator is not owned by this class and must outlive every buffer
    created from it.

    VulkanBuffer is movable but not copyable.

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

public:
                                   // Buffer Lifetime
                                    VulkanBuffer() = default;
                                    VulkanBuffer(const VulkanBuffer&) = delete;
                                    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
                                    VulkanBuffer(VulkanBuffer&& other) noexcept;
                                    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

    bool                            Init(const VulkanAllocator& allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO, VmaAllocationCreateFlags allocationFlags = 0);
    void                            CleanUp();

                                    // Buffer Access
    VkBuffer                        GetBuffer() const { return buffer; }
    VkDeviceSize                    GetSize() const { return size; }
    bool                            IsValid() const { return buffer != VK_NULL_HANDLE && allocation != nullptr; }

private:
                                    // Ownership Transfer
    void                            MoveFrom(VulkanBuffer& other) noexcept;
};

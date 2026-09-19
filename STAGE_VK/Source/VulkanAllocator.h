/*
    ==========================================================================
    VulkanAllocator
    ==========================================================================

    This class owns the Vulkan Memory Allocator used by STAGE VK.

    VMA manages Vulkan device-memory allocations while keeping the Vulkan
    resource model explicit to the engine.

    This class is responsible for:
    - creating the VMA allocator after the Vulkan device is available
    - exposing the allocator to Vulkan resource wrappers
    - destroying the allocator before the Vulkan device is destroyed

    VkInstance, VkPhysicalDevice and VkDevice are provided by VulkanModule and
    VulkanDevice. They are not owned by this class.

    All allocations created from this allocator must be released before
    CleanUp() is called.
*/

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

class VulkanAllocator
{
private:
                                    // Owned Allocator
    VmaAllocator                    allocator = nullptr;

public:
                                    // Allocator Lifetime
    bool                            Init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t apiVersion);
    void                            CleanUp();

                                    // Allocator Access
    VmaAllocator                    GetHandle() const { return allocator; }
    bool                            IsValid() const { return allocator != nullptr; }
};


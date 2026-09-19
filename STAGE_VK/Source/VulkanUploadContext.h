/*
    ==========================================================================
    VulkanUploadContext
    ==========================================================================

    This class manages the Vulkan command resources used for synchronous GPU
    buffer uploads.

    CPU data is first written to a temporary host-visible staging buffer and
    then copied into a GPU-preferred destination buffer using vkCmdCopyBuffer.

    This class is responsible for:
    - owning a dedicated upload command pool and command buffer
    - owning the fence used to wait for upload completion
    - creating and filling temporary staging buffers
    - recording buffer copy commands
    - making transfer writes visible to the destination's next GPU access
    - waiting for the upload to complete before returning

    VkDevice and VkQueue are provided by VulkanDevice and are not owned by
    this class.

    Uploads are currently synchronous and use the existing graphics queue.
*/

#pragma once

#include "VulkanAllocator.h"
#include "VulkanBuffer.h"

#include <vulkan/vulkan.h>

class VulkanUploadContext
{
private:
    // Non-owned Vulkan Handles
    VkDevice                        device = VK_NULL_HANDLE;
    VkQueue                         queue = VK_NULL_HANDLE;

    // Upload Command Resources
    VkCommandPool                   commandPool = VK_NULL_HANDLE;
    VkCommandBuffer                 commandBuffer = VK_NULL_HANDLE;

    // Upload Synchronization
    VkFence                         fence = VK_NULL_HANDLE;

public:
    // Upload Context Lifetime
    VulkanUploadContext() = default;
    ~VulkanUploadContext();

    VulkanUploadContext(const VulkanUploadContext&) = delete;
    VulkanUploadContext& operator=(const VulkanUploadContext&) = delete;
    VulkanUploadContext(VulkanUploadContext&&) = delete;
    VulkanUploadContext& operator=(VulkanUploadContext&&) = delete;

    bool                            Init(VkDevice device, VkQueue queue, uint32_t queueFamily);
    void                            CleanUp();

    // Buffer Upload
    void                            UploadBuffer(const VulkanAllocator& allocator, VulkanBuffer& destination, const void* data, VkDeviceSize dataSize, VkPipelineStageFlags destinationStageMask, VkAccessFlags destinationAccessMask, VkDeviceSize destinationOffset = 0);

private:
    // Upload Resource Creation
    void                            CreateCommandResources(uint32_t queueFamily);
    void                            CreateFence();
};
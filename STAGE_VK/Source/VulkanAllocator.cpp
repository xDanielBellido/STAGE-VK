#define VMA_IMPLEMENTATION
#include "VulkanAllocator.h"

#include <stdexcept>

VulkanAllocator::~VulkanAllocator()
{
    CleanUp();
}

bool VulkanAllocator::Init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t apiVersion)
{
    if (allocator != nullptr)
    {
        throw std::logic_error("Vulkan allocator is already initialized.");
    }

    if (instance == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || device == VK_NULL_HANDLE)
    {
        throw std::invalid_argument("Invalid Vulkan handles provided to VulkanAllocator.");
    }

    VmaAllocatorCreateInfo createInfo{};
    createInfo.instance = instance;
    createInfo.physicalDevice = physicalDevice;
    createInfo.device = device;
    createInfo.vulkanApiVersion = apiVersion;

    if (vmaCreateAllocator(&createInfo, &allocator) != VK_SUCCESS)
    {
        allocator = nullptr;
        throw std::runtime_error("Failed to create Vulkan memory allocator.");
    }

    return true;
}

void VulkanAllocator::CleanUp()
{
    if (allocator != nullptr)
    {
        vmaDestroyAllocator(allocator);
        allocator = nullptr;
    }
}
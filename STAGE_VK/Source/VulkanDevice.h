/*
    ===========================================================================
    VulkanDevice
    ===========================================================================

    This class manages the Vulkan device objects used by the engine.

    A physical device represents a Vulkan-capable GPU available on the system.
    A logical device represents the engine's interface to the selected GPU.

    This class is responsible for:
    - selecting a suitable physical device
    - discovering the required queue families
    - creating the logical device
    - retrieving the graphics and presentation queues
    - releasing the logical device during shutdown

    VkInstance and VkSurfaceKHR are provided by VulkanModule and are not
    owned by this class.
*/

#pragma once

#include <vulkan/vulkan.h>

#include <optional>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class VulkanDevice
{
public:
    bool Init(VkInstance instance, VkSurfaceKHR surface);
    void CleanUp();

    // Vulkan Handles Access
    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
    VkDevice GetDevice() const { return device; }
    VkQueue GetGraphicsQueue() const { return graphicsQueue; }
    VkQueue GetPresentQueue() const { return presentQueue; }
    const QueueFamilyIndices& GetQueueFamilies() const { return queueFamilies; }

private:
    // Physical Device
    void PickPhysicalDevice();
    bool IsDeviceSuitable(VkPhysicalDevice device) const;
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

    // Logical Device
    void CreateLogicalDevice();

    bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
    bool HasAdequateSwapchainSupport(VkPhysicalDevice device) const;

private:
    // Non-owned handles provided by VulkanModule
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    // Physical Device
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices queueFamilies;

    // Logical Device
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
};
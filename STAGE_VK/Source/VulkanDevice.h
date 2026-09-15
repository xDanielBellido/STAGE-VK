/*
    ==========================================================================
    VulkanDevice
    ==========================================================================

    This class manages GPU selection and logical device creation for STAGE VK.

    A physical device represents a Vulkan-capable GPU available on the system.
    A logical device represents the engine's interface to the selected GPU.

    This class is responsible for:
    - selecting a suitable physical device
    - discovering the required queue families
    - checking device and swapchain support
    - creating the logical device
    - retrieving the graphics and presentation queues
    - releasing the logical device during shutdown

    VkInstance and VkSurfaceKHR are provided by VulkanModule and are not owned
    by this class.

    The selected VkPhysicalDevice is also not created or destroyed by this
    class. VulkanDevice only stores the handle of the GPU selected from the
    devices reported by Vulkan.
*/

#pragma once

#include <vulkan/vulkan.h>

#include <optional>

struct QueueFamilyIndices
{
    std::optional<uint32_t>         graphicsFamily;
    std::optional<uint32_t>         presentFamily;

    bool                            IsComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class VulkanDevice
{
private:
                                    // Non-owned Vulkan Handles
    VkInstance                      instance = VK_NULL_HANDLE;
    VkSurfaceKHR                    surface = VK_NULL_HANDLE;

                                    // Selected Physical Device
    VkPhysicalDevice                physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices              queueFamilies;

                                    // Logical Device
    VkDevice                        device = VK_NULL_HANDLE;
    VkQueue                         graphicsQueue = VK_NULL_HANDLE;
    VkQueue                         presentQueue = VK_NULL_HANDLE;

public:
                                    // Device Lifetime
    bool                            Init(VkInstance instance, VkSurfaceKHR surface);
    void                            CleanUp();
    void                            WaitIdle() const;

                                    // Vulkan Handles Access
    VkPhysicalDevice                GetPhysicalDevice() const { return physicalDevice; }
    VkDevice                        GetDevice() const { return device; }
    VkQueue                         GetGraphicsQueue() const { return graphicsQueue; }
    VkQueue                         GetPresentQueue() const { return presentQueue; }

                                    // Queue Families
    const QueueFamilyIndices&       GetQueueFamilies() const { return queueFamilies; }

private:
                                    // Physical Device Selection
    void                            PickPhysicalDevice();
    bool                            IsDeviceSuitable(VkPhysicalDevice device) const;
    QueueFamilyIndices              FindQueueFamilies(VkPhysicalDevice device) const;
    bool                            CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
    bool                            HasAdequateSwapchainSupport(VkPhysicalDevice device) const;

                                    // Logical Device Creation
    void                            CreateLogicalDevice();
};
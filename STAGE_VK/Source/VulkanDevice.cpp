#include "VulkanDevice.h"

#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

bool VulkanDevice::Init(VkInstance instance, VkSurfaceKHR surface)
{
    this->instance = instance;
    this->surface = surface;

    PickPhysicalDevice();
    CreateLogicalDevice();

    return true;
}

void VulkanDevice::CleanUp()
{
    if (device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    physicalDevice = VK_NULL_HANDLE;
    graphicsQueue = VK_NULL_HANDLE;
    presentQueue = VK_NULL_HANDLE;
    queueFamilies = {};

    instance = VK_NULL_HANDLE;
    surface = VK_NULL_HANDLE;
}

// ============================================================================
// Physical Device
// ============================================================================
void VulkanDevice::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find a GPU with Vulkan support.");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const VkPhysicalDevice& device : devices)
    {
        if (IsDeviceSuitable(device))
        {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable Vulkan GPU.");
    }

    queueFamilies = FindQueueFamilies(physicalDevice);

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    std::cout << "[Vulkan] Selected GPU: " << properties.deviceName << '\n';
}

bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices = FindQueueFamilies(device);
    return indices.IsComplete();
}

QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device) const
{
    // A queue family is a group of GPU queues that support a specific set of operations,
    // such as graphics, compute, transfer, or presentation.
    //
    // This function inspects the queue families exposed by a physical device and finds
    // the ones STAGE_VK needs: one capable of executing graphics commands and one capable
    // of presenting images to the current window surface.
    //
    // The returned family indices are later used to create the logical device and obtain
    // the corresponding VkQueue handles.


    // 1. Prepare a structure where we will store the indices of the
    // queue families required by the engine
    QueueFamilyIndices indices;

    // 2. Ask Vulkan how many queue families are exposed by this GPU
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    // 3. Retrieve the properties of every queue family
    std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamiliesProperties.data());

    // 4. Inspect each queue family and check which operations it supports
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        // 5. Check whether this family supports graphics commands
        if (queueFamiliesProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        // 6. Check whether this family can present images to our window surface
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        // 7. Stop searching once both required queue families have been found
        if (indices.IsComplete())
        {
            break;
        }
    }

    // 8. Return the discovered queue family indices
    return indices;
}

// ============================================================================
// Logical Device
// ============================================================================
void VulkanDevice::CreateLogicalDevice()
{
    // A logical device (VkDevice) is the interface that the engine uses to
    // communicate with the selected physical GPU.
    //
    // During its creation we must specify which queue families we want to use.
    // After the device is created, we can retrieve the actual VkQueue handles
    // that will later receive graphics and presentation work.

    // 1. Collect the queue families required by the engine.
    // A set avoids requesting the same family twice when graphics and
    // presentation are supported by the same queue family.
    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilies.graphicsFamily.value(),
        queueFamilies.presentFamily.value()
    };

    // 2. Prepare one queue creation description for each required queue family.
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    // Queue priority ranges from 0.0 to 1.0.
    // We currently request the highest priority for our queues.
    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    // 3. Specify which optional GPU features the logical device should enable.
    // For now STAGE_VK does not require any additional features.
    VkPhysicalDeviceFeatures deviceFeatures{};

    // 4. Describe how the logical device should be created.
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    // 5. Create the logical device from the selected physical GPU.
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan logical device.");
    }

    // 6. Retrieve the queue handles created for graphics and presentation.
    // Queue index 0 is used because we requested one queue from each family.
    vkGetDeviceQueue(device, queueFamilies.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFamilies.presentFamily.value(), 0, &presentQueue);
}
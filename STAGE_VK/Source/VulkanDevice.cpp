#include "VulkanDevice.h"
#include "Log.h"

#include <set>
#include <stdexcept>
#include <vector>
#include <string>

namespace
{
    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
}

bool VulkanDevice::Init(VkInstance instance, VkSurfaceKHR surface)
{
    // VulkanModule owns these handles. VulkanDevice only keeps them because
    // they are required during physical-device selection and device creation.
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
        vkDeviceWaitIdle(device);
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

void VulkanDevice::WaitIdle() const
{
    if (device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(device);
    }
}

// ============================================================================
// Physical Device Selection
// ============================================================================

void VulkanDevice::PickPhysicalDevice()
{
    // Vulkan may expose multiple GPUs. STAGE VK inspects them and selects the
    // first device that satisfies the current renderer requirements.
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

    // Store the queue families belonging to the selected GPU so they can be
    // used later when creating the logical device.
    queueFamilies = FindQueueFamilies(physicalDevice);

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    LOG_INFO("Selected GPU: {}", properties.deviceName);
}

bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device) const
{
    // A GPU is currently suitable if it provides the queue families required
    // by STAGE VK, supports the swapchain extension and can present through
    // the current window surface.
    QueueFamilyIndices indices = FindQueueFamilies(device);
    bool extensionsSupported = CheckDeviceExtensionSupport(device);
    bool swapchainAdequate = extensionsSupported && HasAdequateSwapchainSupport(device);

    return indices.IsComplete() && extensionsSupported && swapchainAdequate;
}

QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device) const
{
    /*
        A queue family groups queues capable of specific types of GPU work.

        STAGE VK currently requires:
        - a graphics-capable queue family
        - a queue family capable of presenting to the current surface

        Both capabilities may be provided by the same queue family.
    */

    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamiliesProperties.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamiliesProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.IsComplete())
        {
            break;
        }
    }

    return indices;
}

bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
{
    // Device extensions expose optional Vulkan functionality.
    // STAGE VK currently requires VK_KHR_swapchain for presentation.
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool VulkanDevice::HasAdequateSwapchainSupport(VkPhysicalDevice device) const
{
    // Supporting VK_KHR_swapchain is not enough by itself. The GPU and current
    // surface must also provide at least one usable format and present mode.
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    return formatCount > 0 && presentModeCount > 0;
}

// ============================================================================
// Logical Device Creation
// ============================================================================

void VulkanDevice::CreateLogicalDevice()
{
    /*
        VkPhysicalDevice identifies the GPU selected by the engine.

        VkDevice is the logical interface used by STAGE VK to access that GPU.
        During its creation we request the queue families and device extensions
        required by the renderer.
    */

    // Graphics and presentation may use the same queue family. Using a set
    // prevents requesting the same family twice.
    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilies.graphicsFamily.value(),
        queueFamilies.presentFamily.value()
    };

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
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

    // STAGE VK does not require additional optional GPU features yet.
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan logical device.");
    }

    // Queue index 0 is used because one queue was requested from each family.
    vkGetDeviceQueue(device, queueFamilies.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFamilies.presentFamily.value(), 0, &presentQueue);
}
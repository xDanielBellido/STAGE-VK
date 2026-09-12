#include "Globals.h"
#include "VulkanModule.h"

#include "Application.h"
#include "Window.h"

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <cstring>
#include <iostream>
#include <set>

namespace
{
    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef _DEBUG
    constexpr bool enableValidationLayers = true;
#else
    constexpr bool enableValidationLayers = false;
#endif
}

// ============================================================================
// Module Lifetime
// ============================================================================
bool VulkanModule::Init()
{
    CreateInstance();
    CreateDebugMessenger();
    CreateSurface(app->GetWindow().GetNativeHandle());
    PickPhysicalDevice();
    CreateLogicalDevice();

    return true;
}

void VulkanModule::PreRender()
{
}

void VulkanModule::PostRender()
{
}

void VulkanModule::OnResize(int width, int height)
{
    // De momento no hacemos nada aquí.
    // Cuando exista la swapchain, aquí o en el renderer
    // se recrearán los recursos dependientes del tamaño.
    (void)width;
    (void)height;
}

bool VulkanModule::CleanUp()
{

    if (device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    if (surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }

    DestroyDebugMessenger();

    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }


    return true;
}

// ============================================================================
// Instance
// ============================================================================
void VulkanModule::CreateInstance()
{
    // Creates the main Vulkan instance used by the engine.
    // It first checks that Vulkan is supported and that the requested validation
    // layers are available, then describes the application, gathers the required
    // instance extensions, configures optional validation layers, and finally
    // creates the VkInstance.

    // 1. Check that Vulkan is available on the current system.
    // GLFW performs the platform-specific check for us.
    if (!glfwVulkanSupported())
    {
        throw std::runtime_error("GLFW reports that Vulkan is not supported on this system.");
    }

    // 2. In Debug builds, make sure the requested validation layers
    // are installed before trying to enable them.
    if (enableValidationLayers && !CheckValidationLayerSupport())
    {
        throw std::runtime_error("Validation layers requested, but not available.");
    }

    // 3. Describe our application to Vulkan.
    // This information mainly identifies the application, engine
    // and Vulkan API version that we intend to use.
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "STAGE_VK";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "STAGE_VK Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    // 4. Ask GLFW which Vulkan instance extensions are required
    // to interact with the current windowing platform.
    const auto extensions = GetRequiredExtensions();

    // 5. Describe how the Vulkan instance must be created.
    // Here we provide application info and required extensions.
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // 6. Enable validation layers only in builds where they are requested
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
    }

    // 7. Create the Vulkan instance.
    // The instance is the application's main connection to the Vulkan API.
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }
}

std::vector<const char*> VulkanModule::GetRequiredExtensions() const
{
    // 1. Ask GLFW how many Vulkan instance extensions are required 
    // for the current windowing system.
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // 2. Make sure GLFW returned a valid list of required extensions.
    if (!glfwExtensions || glfwExtensionCount == 0)
    {
        throw std::runtime_error("Failed to get required GLFW Vulkan extensions.");
    }

    // 3. Copy the GLFW-required extensions into our own list.
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // 4. In Debug builds, add the debug utilities extension required
    // by the Vulkan Debug Messenger.
    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // 5. Return the complete list of instance extensions that must
    // be enabled when creating the VkInstance.
    return extensions;
}

bool VulkanModule::CheckValidationLayerSupport() const
{
    // 1. Ask Vulkan how many instance layers are available on the system
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // 2. Allocate enough space and retrieve the properties of all available layers
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // 3. Check every validation layer requested by STAGE_VK
    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        // 4. Search for the requested layer in the list reported by Vulkan
        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        // 5. If any requested layer is missing, validation cannot be enabled
        if (!layerFound)
        {
            return false;
        }
    }

    // 6. Every requested validation layer is available
    return true;
}

// ============================================================================
// Debug
// ============================================================================
void VulkanModule::CreateDebugMessenger()
{
    if (!enableValidationLayers)
    {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;

    auto createFunction = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    if (!createFunction || createFunction(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan debug messenger.");
    }
}

void VulkanModule::DestroyDebugMessenger()
{
    if (debugMessenger == VK_NULL_HANDLE)
    {
        return;
    }

    auto destroyFunction = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    if (destroyFunction)
    {
        destroyFunction(instance, debugMessenger, nullptr);
        debugMessenger = VK_NULL_HANDLE;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanModule::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
{
    std::cerr << "[Vulkan Validation] " << callbackData->pMessage << '\n';
    return VK_FALSE;
}

// ============================================================================
// Surface
// ============================================================================
void VulkanModule::CreateSurface(GLFWwindow* window)
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan window surface.");
    }
}

// ============================================================================
// Physical Device
// ============================================================================
void VulkanModule::PickPhysicalDevice()
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

bool VulkanModule::IsDeviceSuitable(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices = FindQueueFamilies(device);
    return indices.IsComplete();
}

QueueFamilyIndices VulkanModule::FindQueueFamilies(VkPhysicalDevice device) const
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
void VulkanModule::CreateLogicalDevice()
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
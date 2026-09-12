#include "Globals.h"
#include "VulkanModule.h"

#include "Application.h"
#include "Window.h"

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <cstring>
#include <iostream>

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

    if (!vulkanDevice.Init(instance, surface))
    {
        return false;
    }

    int width = 0;
    int height = 0;
    app->GetWindow().GetFramebufferSize(width, height);

    if (!vulkanSwapchain.Init(vulkanDevice.GetPhysicalDevice(), vulkanDevice.GetDevice(), surface, vulkanDevice.GetQueueFamilies(), static_cast<uint32_t>(width), static_cast<uint32_t>(height)))
    {
        return false;
    }

    if (!commandContext.Init(vulkanDevice.GetDevice(), vulkanDevice.GetQueueFamilies().graphicsFamily.value()))
    {
        return false;
    }

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
    commandContext.CleanUp();
    vulkanSwapchain.CleanUp();
    vulkanDevice.CleanUp();

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


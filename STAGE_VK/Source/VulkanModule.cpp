#include "Globals.h"
#include "VulkanModule.h"
#include "Log.h"
#include "Application.h"
#include "Window.h"
#include "ProfileScope.h"

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <cstring>
#include <algorithm>

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
    // Create the top-level Vulkan objects first.
    CreateInstance();
    CreateDebugMessenger();
    CreateSurface(app->GetWindow().GetNativeHandle());

    // Select a physical device, create the logical device and retrieve queues.
    if (!vulkanDevice.Init(instance, surface))
    {
        return false;
    }

    // The swapchain must match the current framebuffer size.
    int width = 0;
    int height = 0;
    app->GetWindow().GetFramebufferSize(width, height);

    if (!vulkanSwapchain.Init(vulkanDevice.GetPhysicalDevice(), vulkanDevice.GetDevice(), surface, vulkanDevice.GetQueueFamilies(), static_cast<uint32_t>(width), static_cast<uint32_t>(height)))
    {
        return false;
    }

    // Create the command pool and primary command buffer used to record frames.
    if (!commandContext.Init(vulkanDevice.GetDevice(), vulkanDevice.GetQueueFamilies().graphicsFamily.value()))
    {
        return false;
    }

    if (!gpuProfiler.Init(vulkanDevice.GetPhysicalDevice(), vulkanDevice.GetDevice(), vulkanDevice.GetQueueFamilies().graphicsFamily.value()))
    {
        return false;
    }

    // Create the synchronization objects required by the frame lifecycle.
    if (!frameContext.Init(vulkanDevice.GetDevice(), vulkanSwapchain.GetImageCount()))
    {
        return false;
    }

    return true;
}

void VulkanModule::PreRender()
{
    PROFILE_SCOPE(app->GetProfiler(), "VulkanModule::PreRender");

    if (swapchainNeedsRecreation)
    {
        RecreateSwapchain();

        if (swapchainNeedsRecreation)
        {
            return;
        }
    }

    VkDevice device = vulkanDevice.GetDevice();
    VkFence inFlightFence = frameContext.GetInFlightFence();

    {
        PROFILE_SCOPE(app->GetProfiler(), "WaitForPreviousFrame");
        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    }

    gpuProfiler.ResolvePreviousFrame(app->GetProfiler());

    VkResult result = VK_SUCCESS;

    {
        PROFILE_SCOPE(app->GetProfiler(), "AcquireSwapchainImage");
        result = vkAcquireNextImageKHR(device, vulkanSwapchain.GetSwapchain(), UINT64_MAX, frameContext.GetImageAvailableSemaphore(), VK_NULL_HANDLE, &currentImageIndex);
    }

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        swapchainNeedsRecreation = true;
        frameReady = false;
        return;
    }

    if (result == VK_SUBOPTIMAL_KHR)
    {
        swapchainNeedsRecreation = true;
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to acquire Vulkan swapchain image.");
    }

    vkResetFences(device, 1, &inFlightFence);

    VkCommandBuffer commandBuffer = commandContext.GetCommandBuffer();

    {
        PROFILE_SCOPE(app->GetProfiler(), "BeginCommandBuffer");

        if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to reset Vulkan command buffer.");
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording Vulkan command buffer.");
        }
    }

    if (app->GetProfiler().IsRecording())
    {
        gpuProfiler.BeginFrame(commandBuffer);
    }

    frameReady = true;
}

void VulkanModule::PostRender()
{
    PROFILE_SCOPE(app->GetProfiler(), "VulkanModule::PostRender");

    if (!frameReady)
    {
        return;
    }

    VkSemaphore imageAvailableSemaphore = frameContext.GetImageAvailableSemaphore();
    VkSemaphore renderFinishedSemaphore = frameContext.GetRenderFinishedSemaphore(currentImageIndex);
    VkFence inFlightFence = frameContext.GetInFlightFence();
    VkCommandBuffer commandBuffer = commandContext.GetCommandBuffer();

    gpuProfiler.EndFrame(commandBuffer);

    {
        PROFILE_SCOPE(app->GetProfiler(), "EndCommandBuffer");

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to end Vulkan command buffer.");
        }
    }

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

    {
        PROFILE_SCOPE(app->GetProfiler(), "QueueSubmit");

        if (vkQueueSubmit(vulkanDevice.GetGraphicsQueue(), 1, &submitInfo, inFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit Vulkan command buffer.");
        }
    }

    gpuProfiler.MarkSubmitted();

    VkSwapchainKHR swapchain = vulkanSwapchain.GetSwapchain();

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImageIndex;

    VkResult result = VK_SUCCESS;

    {
        PROFILE_SCOPE(app->GetProfiler(), "Present");
        result = vkQueuePresentKHR(vulkanDevice.GetPresentQueue(), &presentInfo);
    }

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        swapchainNeedsRecreation = true;
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present Vulkan swapchain image.");
    }

    frameReady = false;
}

void VulkanModule::OnResize(int width, int height)
{
    // Actual recreation is deferred until PreRender().
    // Zero-sized framebuffers are ignored while the window is minimized.
    if (width > 0 && height > 0)
    {
        swapchainNeedsRecreation = true;
    }
}

bool VulkanModule::CleanUp()
{
    // GPU resources cannot be destroyed while they are still in use.
    vulkanDevice.WaitIdle();

    // Destroy resources in reverse dependency order.
    gpuProfiler.CleanUp();
    frameContext.CleanUp();
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
// Swapchain Recreation
// ============================================================================

void VulkanModule::RecreateSwapchain()
{
    int width = 0;
    int height = 0;
    app->GetWindow().GetFramebufferSize(width, height);

    // Minimized windows commonly report a 0x0 framebuffer.
    // Keep the recreation request pending until a valid size is available.
    if (width == 0 || height == 0)
    {
        return;
    }

    // Swapchain-dependent resources must no longer be in use before they are
    // destroyed.
    vulkanDevice.WaitIdle();

    // Give higher-level systems a chance to destroy resources that depend on
    // the old swapchain, such as RenderModule framebuffers.
    for (IVulkanSwapchainListener* listener : swapchainListeners)
    {
        if (listener)
        {
            listener->BeforeSwapchainRecreate();
        }
    }

    frameContext.CleanUp();
    vulkanSwapchain.CleanUp();

    if (!vulkanSwapchain.Init(vulkanDevice.GetPhysicalDevice(), vulkanDevice.GetDevice(), surface, vulkanDevice.GetQueueFamilies(), static_cast<uint32_t>(width), static_cast<uint32_t>(height)))
    {
        throw std::runtime_error("Failed to recreate Vulkan swapchain.");
    }

    // Synchronization is recreated because the number of swapchain images may
    // have changed.
    if (!frameContext.Init(vulkanDevice.GetDevice(), vulkanSwapchain.GetImageCount()))
    {
        throw std::runtime_error("Failed to recreate Vulkan frame synchronization.");
    }

    // Higher-level systems can now rebuild against the new swapchain.
    for (IVulkanSwapchainListener* listener : swapchainListeners)
    {
        if (listener)
        {
            listener->AfterSwapchainRecreate();
        }
    }

    currentImageIndex = 0;
    frameReady = false;
    swapchainNeedsRecreation = false;
}

// ============================================================================
// Swapchain Listeners
// ============================================================================

void VulkanModule::RegisterSwapchainListener(IVulkanSwapchainListener* listener)
{
    if (!listener)
    {
        return;
    }

    // Avoid registering the same listener more than once.
    if (std::find(swapchainListeners.begin(), swapchainListeners.end(), listener) == swapchainListeners.end())
    {
        swapchainListeners.push_back(listener);
    }
}

void VulkanModule::UnregisterSwapchainListener(IVulkanSwapchainListener* listener)
{
    swapchainListeners.erase(std::remove(swapchainListeners.begin(), swapchainListeners.end(), listener), swapchainListeners.end());
}

// ============================================================================
// Instance
// ============================================================================

void VulkanModule::CreateInstance()
{
    // GLFW verifies that the current platform can provide Vulkan support.
    if (!glfwVulkanSupported())
    {
        throw std::runtime_error("GLFW reports that Vulkan is not supported on this system.");
    }

    // Validation layers are requested only for Debug builds.
    if (enableValidationLayers && !CheckValidationLayerSupport())
    {
        throw std::runtime_error("Validation layers requested, but not available.");
    }

    // Describe the application and the Vulkan API version it expects.
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "STAGE_VK";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "STAGE_VK Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = API_VERSION;

    const auto extensions = GetRequiredExtensions();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

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

    // VkInstance is the application's main connection to the Vulkan API.
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }
}

std::vector<const char*> VulkanModule::GetRequiredExtensions() const
{
    // GLFW provides the platform-specific extensions required to create a
    // Vulkan surface for its windows.
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (!glfwExtensions || glfwExtensionCount == 0)
    {
        throw std::runtime_error("Failed to get required GLFW Vulkan extensions.");
    }

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // The debug messenger requires VK_EXT_debug_utils.
    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool VulkanModule::CheckValidationLayerSupport() const
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Every requested layer must exist on the current system.
    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

// ============================================================================
// Validation
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

    // Debug-utils functions are extension functions, so their addresses are
    // retrieved dynamically from the Vulkan instance.
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
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        LOG_ERROR("[Vulkan Validation] {}", callbackData->pMessage);
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LOG_WARN("[Vulkan Validation] {}", callbackData->pMessage);
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        LOG_INFO("[Vulkan Validation] {}", callbackData->pMessage);
    }
    else
    {
        LOG_TRACE("[Vulkan Validation] {}", callbackData->pMessage);
    }

    return VK_FALSE;
}

// ============================================================================
// Surface
// ============================================================================

void VulkanModule::CreateSurface(GLFWwindow* window)
{
    // GLFW creates the platform-specific VkSurfaceKHR used by the presentation
    // system to display swapchain images in the application window.
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan window surface.");
    }
}
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

    CreateRenderPass();
    CreateFramebuffers();

    if (!commandContext.Init(vulkanDevice.GetDevice(), vulkanDevice.GetQueueFamilies().graphicsFamily.value()))
    {
        return false;
    }

    if (!frameContext.Init(vulkanDevice.GetDevice(), vulkanSwapchain.GetImageCount()))
    {
        return false;
    }

    return true;
}

void VulkanModule::PreRender()
{
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

    // 1. Wait until the GPU has finished using the resources from the previous frame.
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);

    // 2. Acquire the next available image from the swapchain.
    VkResult result = vkAcquireNextImageKHR(device, vulkanSwapchain.GetSwapchain(), UINT64_MAX, frameContext.GetImageAvailableSemaphore(), VK_NULL_HANDLE, &currentImageIndex);

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

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire Vulkan swapchain image.");
    }

    // 3. The image was acquired successfully, so the fence can now be reused
    // for the work that will be submitted for this frame.
    vkResetFences(device, 1, &inFlightFence);

    // 4. Reset the command buffer before recording the new frame.
    VkCommandBuffer commandBuffer = commandContext.GetCommandBuffer();

    if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to reset Vulkan command buffer.");
    }

    // 5. Record the commands that clear the acquired swapchain image.
    RecordClearCommands(currentImageIndex);

    frameReady = true;
}

void VulkanModule::PostRender()
{
    if (!frameReady)
    {
        return;
    }

    VkSemaphore imageAvailableSemaphore = frameContext.GetImageAvailableSemaphore();
    VkSemaphore renderFinishedSemaphore = frameContext.GetRenderFinishedSemaphore(currentImageIndex);
    VkFence inFlightFence = frameContext.GetInFlightFence();
    VkCommandBuffer commandBuffer = commandContext.GetCommandBuffer();

    // 1. Submit the recorded command buffer to the graphics queue.
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

    if (vkQueueSubmit(vulkanDevice.GetGraphicsQueue(), 1, &submitInfo, inFlightFence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit Vulkan command buffer.");
    }

    // 2. Present the rendered swapchain image.
    VkSwapchainKHR swapchain = vulkanSwapchain.GetSwapchain();

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImageIndex;

    VkResult result = vkQueuePresentKHR(vulkanDevice.GetPresentQueue(), &presentInfo);

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
    if (width > 0 && height > 0)
    {
        swapchainNeedsRecreation = true;
    }
}

void VulkanModule::RecreateSwapchain()
{
    int width = 0;
    int height = 0;
    app->GetWindow().GetFramebufferSize(width, height);

    // A minimized window may temporarily have a framebuffer size of 0x0.
    // In that case there is nothing valid to recreate yet.
    if (width == 0 || height == 0)
    {
        return;
    }

    // 1. Make sure the GPU is no longer using swapchain-dependent resources.
    vulkanDevice.WaitIdle();

    // 2. Destroy resources that depend on the current swapchain.
    frameContext.CleanUp();
    DestroyRenderTargets();
    vulkanSwapchain.CleanUp();

    // 3. Create the swapchain again using the new framebuffer size.
    if (!vulkanSwapchain.Init(vulkanDevice.GetPhysicalDevice(), vulkanDevice.GetDevice(), surface, vulkanDevice.GetQueueFamilies(), static_cast<uint32_t>(width), static_cast<uint32_t>(height)))
    {
        throw std::runtime_error("Failed to recreate Vulkan swapchain.");
    }

    CreateRenderPass();
    CreateFramebuffers();

    // 4. Recreate synchronization objects that depend on the swapchain image count.
    if (!frameContext.Init(vulkanDevice.GetDevice(), vulkanSwapchain.GetImageCount()))
    {
        throw std::runtime_error("Failed to recreate Vulkan frame synchronization.");
    }

    currentImageIndex = 0;
    frameReady = false;
    swapchainNeedsRecreation = false;
}

bool VulkanModule::CleanUp()
{
    vulkanDevice.WaitIdle();

    frameContext.CleanUp();
    commandContext.CleanUp();
    DestroyRenderTargets();
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

// ============================================================================
// Swapchain Render Targets
// ============================================================================
void VulkanModule::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vulkanSwapchain.GetImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &colorAttachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(vulkanDevice.GetDevice(), &createInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan render pass.");
    }
}

void VulkanModule::CreateFramebuffers()
{
    const std::vector<VkImageView>& imageViews = vulkanSwapchain.GetImageViews();
    swapchainFramebuffers.resize(imageViews.size(), VK_NULL_HANDLE);

    for (size_t i = 0; i < imageViews.size(); ++i)
    {
        VkImageView attachments[] = { imageViews[i] };

        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = attachments;
        createInfo.width = vulkanSwapchain.GetExtent().width;
        createInfo.height = vulkanSwapchain.GetExtent().height;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(vulkanDevice.GetDevice(), &createInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan framebuffer.");
        }
    }
}

void VulkanModule::DestroyRenderTargets()
{
    VkDevice device = vulkanDevice.GetDevice();

    for (VkFramebuffer framebuffer : swapchainFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }

    swapchainFramebuffers.clear();

    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }
}
// ============================================================================
// Frame Recording
// ============================================================================
void VulkanModule::RecordClearCommands(uint32_t imageIndex)
{
    VkCommandBuffer commandBuffer = commandContext.GetCommandBuffer();

    // 1. Begin recording commands into the command buffer.
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin recording Vulkan command buffer.");
    }

    // 2. Define the clear color used when the render pass begins.
    VkClearValue clearValue{};
    clearValue.color = { { 0.0f, 1.0f, 1.0f, 1.0f } };

    // 3. Describe this render pass execution.
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vulkanSwapchain.GetExtent();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    // 4. Begin and immediately end the render pass.
    // For now the render pass only clears the swapchain image.
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(commandBuffer);

    // 5. Finish command recording.
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record Vulkan command buffer.");
    }
}
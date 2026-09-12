/*
    ===========================================================================
    VulkanModule
    ===========================================================================

    This module represents the Vulkan backend layer of the engine.

    Its responsibility is to manage the low-level graphics API objects that
    allow the engine to communicate with Vulkan and with the operating system
    windowing system.

    This module is responsible for:
    - creating the Vulkan instance
    - creating the window surface used for presentation
    - exposing access to those Vulkan handles to other engine systems
    - reacting to window resize events
    - releasing Vulkan resources during shutdown
    - physical device selection
    - logical device creation
    - graphics and presentation queues
    - swapchain creation and recreation
    - synchronization objects
    - frame lifecycle management
*/

#pragma once

#include "Module.h"

#include <vulkan/vulkan.h>

#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandContext.h"
#include "FrameContext.h"

#include <vector>

struct GLFWwindow;

class VulkanModule : public Module
{
public:

    // Module Lifetime
    bool Init() override;
    void PreRender() override;
    void PostRender() override;
    void OnResize(int width, int height) override;
    bool CleanUp() override;

    // Vulkan Handles Access
    VkInstance GetInstance() const { return instance; }
    VkSurfaceKHR GetSurface() const { return surface; }


    VkPhysicalDevice GetPhysicalDevice() const { return vulkanDevice.GetPhysicalDevice(); }
    VkDevice GetDevice() const { return vulkanDevice.GetDevice(); }
    VkQueue GetGraphicsQueue() const { return vulkanDevice.GetGraphicsQueue(); }
    VkQueue GetPresentQueue() const { return vulkanDevice.GetPresentQueue(); }

private:

    // Instance
    void CreateInstance();
    std::vector<const char*> GetRequiredExtensions() const;
    bool CheckValidationLayerSupport() const;

    // Debug
    void CreateDebugMessenger();
    void DestroyDebugMessenger();
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);

    // Surface
    void CreateSurface(GLFWwindow* window);

private:

    // Instance
    VkInstance instance = VK_NULL_HANDLE;

    // Debug
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    // Surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VulkanDevice vulkanDevice;
    VulkanSwapchain vulkanSwapchain;
    VulkanCommandContext commandContext;
    FrameContext frameContext;
};

/*
    ===========================================================================
    BackendVKModule
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

#include "../Core/Module.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

struct GLFWwindow;

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class BackendVKModule : public Module
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
    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }

private:
    // Vulkan Initialization
    void CreateInstance();
    void CreateSurface(GLFWwindow* window);

    void PickPhysicalDevice();
    bool IsDeviceSuitable(VkPhysicalDevice device) const;

    std::vector<const char*> GetRequiredExtensions() const;
    bool CheckValidationLayerSupport() const;

    void CreateDebugMessenger();
    void DestroyDebugMessenger();

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

private:
    // Core Vulkan Objects
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    QueueFamilyIndices queueFamilies;
};

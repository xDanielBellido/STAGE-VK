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

struct GLFWwindow;

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

private:
    // Vulkan Initialization
    void CreateInstance();
    void CreateSurface(GLFWwindow* window);
    std::vector<const char*> GetRequiredExtensions() const;

private:
    // Core Vulkan Objects
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
};

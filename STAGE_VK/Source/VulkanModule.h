/*
    ===========================================================================
    VulkanModule
    ===========================================================================

    Low-level Vulkan backend and frame-lifecycle coordinator for STAGE VK.

    VulkanModule owns the top-level Vulkan objects required by the application
    and orchestrates the helper classes that manage the different parts of the
    backend.

    Directly owned Vulkan objects:
    - VkInstance
    - VkDebugUtilsMessengerEXT
    - VkSurfaceKHR

    Backend systems coordinated by this module:
    - VulkanDevice
        Selects the physical device, creates the logical device and exposes
        the graphics and presentation queues.

    - VulkanSwapchain
        Creates and manages the swapchain, its images and image views.

    - VulkanCommandContext
        Owns the command pool and primary command buffer used to record GPU
        work.

    - FrameContext
        Owns the semaphores and fence used to synchronize frame execution.

    VulkanModule is also responsible for the Vulkan frame lifecycle:

        PreRender()
            Wait for previous GPU work
            Acquire a swapchain image
            Reset command resources
            Begin command-buffer recording

        [RenderModule records rendering commands]

        PostRender()
            End command-buffer recording
            Submit work to the graphics queue
            Present the acquired swapchain image

    The module also handles swapchain recreation when the window changes size
    or Vulkan reports that the current swapchain is no longer valid.

    Higher-level renderer systems can register as swapchain listeners so that
    swapchain-dependent resources can be destroyed and rebuilt safely.

    IMPORTANT:
    VulkanModule controls HOW a Vulkan frame is executed.
    It does not decide WHAT is rendered. That responsibility belongs to the
    renderer layer.
*/

#pragma once

#include "Module.h"

#include <vulkan/vulkan.h>

#include "VulkanDevice.h"
#include "VulkanAllocator.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandContext.h"
#include "FrameContext.h"
#include "VulkanGpuProfiler.h"
#include "IVulkanSwapchainListener.h"

#include <vector>

struct GLFWwindow;

class VulkanModule : public Module
{
private:

    // Core Vulkan objects
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    // Vulkan subsystems
    VulkanDevice vulkanDevice;
    VulkanAllocator vulkanAllocator;
    VulkanSwapchain vulkanSwapchain;
    VulkanCommandContext commandContext;
    FrameContext frameContext;
    VulkanGpuProfiler gpuProfiler;

    // Current frame state
    uint32_t currentImageIndex = 0;
    bool frameReady = false;
    bool swapchainNeedsRecreation = false;
    uint32_t currentFrameSlot = 0;

    // Systems that depend on swapchain resources
    std::vector<IVulkanSwapchainListener*> swapchainListeners;


public:

    static constexpr uint32_t API_VERSION = VK_API_VERSION_1_2;
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

                        // Module lifecycle
    bool                Init() override;
    void                PreRender() override;
    void                PostRender() override;
    void                OnResize(int width, int height) override;
    bool                CleanUp() override;

                        // Core Vulkan access
    VkInstance          GetInstance() const { return instance; }
    VkSurfaceKHR        GetSurface() const { return surface; }

    VkPhysicalDevice    GetPhysicalDevice() const { return vulkanDevice.GetPhysicalDevice(); }
    VkDevice            GetDevice() const { return vulkanDevice.GetDevice(); }

    VkQueue             GetGraphicsQueue() const { return vulkanDevice.GetGraphicsQueue(); }
    VkQueue             GetPresentQueue() const { return vulkanDevice.GetPresentQueue(); }

                        // Current frame
    VkCommandBuffer     GetCommandBuffer() const { return commandContext.GetCommandBuffer(currentFrameSlot); }
    uint32_t            GetCurrentImageIndex() const { return currentImageIndex; }
    bool                IsFrameReady() const { return frameReady; }

                        // Swapchain information
    VkFormat            GetSwapchainImageFormat() const { return vulkanSwapchain.GetImageFormat(); }
    VkExtent2D          GetSwapchainExtent() const { return vulkanSwapchain.GetExtent(); }
    uint32_t            GetSwapchainImageCount() const { return vulkanSwapchain.GetImageCount(); }
    const std::vector<VkImageView>& GetSwapchainImageViews() const { return vulkanSwapchain.GetImageViews(); }

                        // Device information
    uint32_t            GetApiVersion() const { return API_VERSION; }
    uint32_t            GetGraphicsQueueFamily() const { return vulkanDevice.GetQueueFamilies().graphicsFamily.value(); }
    VulkanAllocator&    GetAllocator() { return vulkanAllocator; }
    const VulkanAllocator& GetAllocator() const { return vulkanAllocator; }

                        // Swapchain listeners
    void                RegisterSwapchainListener(IVulkanSwapchainListener* listener);
    void                UnregisterSwapchainListener(IVulkanSwapchainListener* listener);

                        // Synchronization
    void                WaitIdle() const { vulkanDevice.WaitIdle(); }


private:

                        // Instance
    void                CreateInstance();
    std::vector<const char*> GetRequiredExtensions() const;
    bool                CheckValidationLayerSupport() const;

                        // Validation
    void                CreateDebugMessenger();
    void                DestroyDebugMessenger();
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);

                        // Surface
    void                CreateSurface(GLFWwindow* window);

                        // Swapchain
    void                RecreateSwapchain();
};

/*
    ==========================================================================
    ImGuiPass
    ==========================================================================

    This class manages the rendering integration between Dear ImGui and Vulkan.

    Its main responsibility is to provide the technical infrastructure required
    to render ImGui inside the current Vulkan frame:
    - create and destroy the Dear ImGui context
    - initialize the GLFW and Vulkan backends
    - manage the descriptor pool required by ImGui
    - begin a new ImGui frame
    - record ImGui draw data into the active Vulkan command buffer
    - update Vulkan-dependent resources after swapchain recreation

    ImGuiPass does not define the editor interface itself.
    Editor windows and tools are created by EditorModule.

    ImGuiPass is owned and orchestrated by RenderModule.
*/
#pragma once

#include <vulkan/vulkan.h>

struct GLFWwindow;

/*
    Initialization data required to connect Dear ImGui with the current
    window, Vulkan device, graphics queue and RenderModule render pass.
*/
struct ImGuiPassInitInfo
{
    GLFWwindow* window = nullptr;

    uint32_t                apiVersion = VK_API_VERSION_1_0;

    VkInstance              instance = VK_NULL_HANDLE;
    VkPhysicalDevice        physicalDevice = VK_NULL_HANDLE;
    VkDevice                device = VK_NULL_HANDLE;

    uint32_t                graphicsQueueFamily = 0;
    VkQueue                 graphicsQueue = VK_NULL_HANDLE;

    uint32_t                imageCount = 0;
    VkRenderPass            renderPass = VK_NULL_HANDLE;
};

class ImGuiPass
{
private:
    ImGuiPassInitInfo       initInfo{};

    VkDescriptorPool        descriptorPool = VK_NULL_HANDLE;

    bool                    contextCreated = false;
    bool                    glfwBackendInitialized = false;
    bool                    vulkanBackendInitialized = false;

public:
                            // Pass Lifetime
    bool                    Init(const ImGuiPassInitInfo& initInfo);
    void                    CleanUp();

                            // Frame Rendering
    void                    StartFrame();
    void                    Record(VkCommandBuffer commandBuffer);

                            // Swapchain Recreation
    void                    BeforeSwapchainRecreate();
    void                    AfterSwapchainRecreate(VkRenderPass renderPass, uint32_t imageCount);

private:
                            // Vulkan Backend
    bool                    InitVulkanBackend(VkRenderPass renderPass, uint32_t imageCount);

                            // Descriptor Pool
    void                    CreateDescriptorPool();
    void                    DestroyDescriptorPool();
};
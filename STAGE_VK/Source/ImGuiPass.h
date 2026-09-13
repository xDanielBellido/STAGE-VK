#pragma once

#include <vulkan/vulkan.h>

struct GLFWwindow;

struct ImGuiPassInitInfo
{
    GLFWwindow* window = nullptr;

    uint32_t apiVersion = VK_API_VERSION_1_0;

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    uint32_t graphicsQueueFamily = 0;
    VkQueue graphicsQueue = VK_NULL_HANDLE;

    uint32_t imageCount = 0;
    VkRenderPass renderPass = VK_NULL_HANDLE;
};

class ImGuiPass
{
public:
    bool Init(const ImGuiPassInitInfo& initInfo);

    void StartFrame();
    void Record(VkCommandBuffer commandBuffer);

    void BeforeSwapchainRecreate();
    void AfterSwapchainRecreate(VkRenderPass renderPass, uint32_t imageCount);

    void CleanUp();

private:
    void CreateDescriptorPool();
    void DestroyDescriptorPool();
    bool InitVulkanBackend(VkRenderPass renderPass, uint32_t imageCount);

private:
    ImGuiPassInitInfo initInfo{};

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    bool contextCreated = false;
    bool glfwBackendInitialized = false;
    bool vulkanBackendInitialized = false;
};

#include "ImGuiPass.h"
#include "Log.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <GLFW/glfw3.h>

#include <filesystem>
#include <stdexcept>

namespace
{
    constexpr uint32_t IMGUI_MIN_IMAGE_COUNT = 2;

    std::filesystem::path FindFontAwesomePath()
    {
        constexpr const char* FONT_FILE = "Font Awesome 7 Free-Solid-900.otf";

        std::filesystem::path directory = std::filesystem::current_path();

        for (int i = 0; i < 5; ++i)
        {
            const std::filesystem::path candidates[] =
            {
                directory / "External Libraries" / "FontAwesome" / FONT_FILE,
                directory / "External Libraries" / "Font Awesome" / FONT_FILE,
                directory / "External Libraries" / "FontAwesome" / "otfs" / FONT_FILE
            };

            for (const std::filesystem::path& candidate : candidates)
            {
                if (std::filesystem::exists(candidate))
                {
                    return std::filesystem::absolute(candidate);
                }
            }

            if (!directory.has_parent_path())
            {
                break;
            }

            directory = directory.parent_path();
        }

        throw std::runtime_error("Could not find Font Awesome file: " + std::string(FONT_FILE));
    }

    void CheckVkResult(VkResult result)
    {
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("[ImGui Vulkan] VkResult = {}", static_cast<int>(result));
        }
    }
}

bool ImGuiPass::Init(const ImGuiPassInitInfo& initInfo)
{
    if (!initInfo.window || initInfo.instance == VK_NULL_HANDLE || initInfo.physicalDevice == VK_NULL_HANDLE || initInfo.device == VK_NULL_HANDLE || initInfo.graphicsQueue == VK_NULL_HANDLE || initInfo.renderPass == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Invalid ImGui pass initialization data.");
    }

    if (initInfo.imageCount < IMGUI_MIN_IMAGE_COUNT)
    {
        throw std::runtime_error("ImGui requires at least two swapchain images.");
    }

    this->initInfo = initInfo;

    CreateDescriptorPool();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    contextCreated = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.Fonts->AddFontDefault();

    ImFontConfig iconConfig{};
    iconConfig.MergeMode = true;
    iconConfig.PixelSnapH = true;

    const std::filesystem::path iconFontPath = FindFontAwesomePath();

#if IMGUI_VERSION_NUM >= 19200
    ImFont* iconFont = io.Fonts->AddFontFromFileTTF(iconFontPath.string().c_str(), 0.0f, &iconConfig);
#else
    static const ImWchar iconRanges[] = { 0xF000, 0xF8FF, 0 };
    ImFont* iconFont = io.Fonts->AddFontFromFileTTF(iconFontPath.string().c_str(), 13.0f, &iconConfig, iconRanges);
#endif

    if (!iconFont)
    {
        throw std::runtime_error("Failed to load Font Awesome icon font: " + iconFontPath.string());
    }

    if (!iconFont)
    {
        throw std::runtime_error("Failed to load Font Awesome icon font: " + iconFontPath.string());
    }

    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForVulkan(initInfo.window, true))
    {
        CleanUp();
        return false;
    }

    glfwBackendInitialized = true;

    if (!InitVulkanBackend(initInfo.renderPass, initInfo.imageCount))
    {
        CleanUp();
        return false;
    }

    return true;
}

void ImGuiPass::CreateDescriptorPool()
{
    VkDescriptorPoolSize poolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE }
    };

    uint32_t maxSets = 0;

    for (const VkDescriptorPoolSize& poolSize : poolSizes)
    {
        maxSets += poolSize.descriptorCount;
    }

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    createInfo.maxSets = maxSets;
    createInfo.poolSizeCount = static_cast<uint32_t>(sizeof(poolSizes) / sizeof(poolSizes[0]));
    createInfo.pPoolSizes = poolSizes;

    if (vkCreateDescriptorPool(initInfo.device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create ImGui Vulkan descriptor pool.");
    }
}

void ImGuiPass::DestroyDescriptorPool()
{
    if (descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(initInfo.device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
}

bool ImGuiPass::InitVulkanBackend(VkRenderPass renderPass, uint32_t imageCount)
{
    ImGui_ImplVulkan_InitInfo backendInfo{};
    backendInfo.ApiVersion = initInfo.apiVersion;
    backendInfo.Instance = initInfo.instance;
    backendInfo.PhysicalDevice = initInfo.physicalDevice;
    backendInfo.Device = initInfo.device;
    backendInfo.QueueFamily = initInfo.graphicsQueueFamily;
    backendInfo.Queue = initInfo.graphicsQueue;
    backendInfo.DescriptorPool = descriptorPool;
    backendInfo.MinImageCount = IMGUI_MIN_IMAGE_COUNT;
    backendInfo.ImageCount = imageCount;
    backendInfo.PipelineInfoMain.RenderPass = renderPass;
    backendInfo.PipelineInfoMain.Subpass = 0;
    backendInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    backendInfo.CheckVkResultFn = CheckVkResult;

    if (!ImGui_ImplVulkan_Init(&backendInfo))
    {
        return false;
    }

    vulkanBackendInitialized = true;

    return true;
}

void ImGuiPass::StartFrame()
{
    if (!vulkanBackendInitialized || !glfwBackendInitialized)
    {
        return;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiPass::Record(VkCommandBuffer commandBuffer)
{
    if (!vulkanBackendInitialized)
    {
        return;
    }

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void ImGuiPass::BeforeSwapchainRecreate()
{
    /*if (vulkanBackendInitialized)
    {
        ImGui_ImplVulkan_Shutdown();
        vulkanBackendInitialized = false;
    }*/
}

void ImGuiPass::AfterSwapchainRecreate(VkRenderPass renderPass, uint32_t imageCount)
{
    if (!vulkanBackendInitialized)
    {
        return;
    }

    // If the swapchain image count changed, the backend needs to be initialized
    // again because ImGui keeps per-image render buffers internally.
    if (imageCount != initInfo.imageCount)
    {
        ImGui_ImplVulkan_Shutdown();
        vulkanBackendInitialized = false;

        initInfo.renderPass = renderPass;
        initInfo.imageCount = imageCount;

        if (!InitVulkanBackend(renderPass, imageCount))
        {
            throw std::runtime_error("Failed to recreate ImGui Vulkan backend.");
        }

        return;
    }

    // The render pass object was recreated, so rebuild ImGui's graphics pipeline
    // against the new render pass without destroying the whole Vulkan backend.
    ImGui_ImplVulkan_PipelineInfo pipelineInfo{};
    pipelineInfo.RenderPass = renderPass;
    pipelineInfo.Subpass = 0;
    pipelineInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_CreateMainPipeline(&pipelineInfo);

    initInfo.renderPass = renderPass;
}

void ImGuiPass::CleanUp()
{
    if (vulkanBackendInitialized)
    {
        ImGui_ImplVulkan_Shutdown();
        vulkanBackendInitialized = false;
    }

    if (glfwBackendInitialized)
    {
        ImGui_ImplGlfw_Shutdown();
        glfwBackendInitialized = false;
    }

    if (contextCreated)
    {
        ImGui::DestroyContext();
        contextCreated = false;
    }

    DestroyDescriptorPool();

    initInfo = {};
}
#include "../Core/Globals.h"
#include "BackendVKModule.h"

#include "../Core/Application.h"
#include "../Platform/Window.h"

#include <GLFW/glfw3.h>

#include <stdexcept>

bool BackendVKModule::Init()
{
    CreateInstance();
    CreateSurface(app->GetWindow().GetNativeHandle());

    return true;
}

void BackendVKModule::PreRender()
{
}

void BackendVKModule::PostRender()
{
}

void BackendVKModule::OnResize(int width, int height)
{
    // De momento no hacemos nada aquí.
    // Cuando exista la swapchain, aquí o en el renderer
    // se recrearán los recursos dependientes del tamaño.
    (void)width;
    (void)height;
}

bool BackendVKModule::CleanUp()
{

    if (surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }

    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }

    return true;
}

void BackendVKModule::CreateInstance()
{
    if (!glfwVulkanSupported())
    {
        throw std::runtime_error("GLFW reports that Vulkan is not supported on this system.");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "STAGE_VK";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "STAGE_VK Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    const auto extensions = GetRequiredExtensions();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }
}

void BackendVKModule::CreateSurface(GLFWwindow* window)
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan window surface.");
    }
}

std::vector<const char*> BackendVKModule::GetRequiredExtensions() const
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (!glfwExtensions || glfwExtensionCount == 0)
    {
        throw std::runtime_error("Failed to get required GLFW Vulkan extensions.");
    }

    return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}

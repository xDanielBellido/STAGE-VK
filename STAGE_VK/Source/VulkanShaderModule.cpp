#include "VulkanShaderModule.h"

#include <fstream>
#include <utility>

VulkanShaderModule::~VulkanShaderModule()
{
    CleanUp();
}

VulkanShaderModule::VulkanShaderModule(VulkanShaderModule&& other) noexcept
{
    *this = std::move(other);
}

VulkanShaderModule& VulkanShaderModule::operator=(VulkanShaderModule&& other) noexcept
{
    if (this != &other)
    {
        CleanUp();

        device = other.device;
        shaderModule = other.shaderModule;

        other.device = VK_NULL_HANDLE;
        other.shaderModule = VK_NULL_HANDLE;
    }

    return *this;
}

bool VulkanShaderModule::Init(VkDevice device, const std::filesystem::path& spirvPath)
{
    CleanUp();

    if (device == VK_NULL_HANDLE)
    {
        return false;
    }

    std::vector<uint32_t> code;

    if (!ReadSpirvFile(spirvPath, code))
    {
        return false;
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    VkShaderModule newShaderModule = VK_NULL_HANDLE;

    if (vkCreateShaderModule(device, &createInfo, nullptr, &newShaderModule) != VK_SUCCESS)
    {
        return false;
    }

    this->device = device;
    shaderModule = newShaderModule;

    return true;
}

void VulkanShaderModule::CleanUp()
{
    if (shaderModule != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }

    shaderModule = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
}

bool VulkanShaderModule::ReadSpirvFile(const std::filesystem::path& path, std::vector<uint32_t>& code)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return false;
    }

    const std::streamsize fileSize = file.tellg();

    if (fileSize <= 0 || fileSize % static_cast<std::streamsize>(sizeof(uint32_t)) != 0)
    {
        return false;
    }

    code.resize(static_cast<size_t>(fileSize) / sizeof(uint32_t));

    file.seekg(0);

    if (!file.read(reinterpret_cast<char*>(code.data()), fileSize))
    {
        code.clear();
        return false;
    }

    return true;
}
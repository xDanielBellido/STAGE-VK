/*
    ===========================================================================
    VulkanShaderModule
    ===========================================================================

    Small RAII wrapper around VkShaderModule.

    Vulkan consumes shaders as SPIR-V binary modules. This class loads compiled
    SPIR-V code from disk, creates the corresponding VkShaderModule and owns its
    lifetime.

    The VkDevice is borrowed and is not owned by this class.
*/

#pragma once

#include <vulkan/vulkan.h>

#include <filesystem>
#include <vector>

class VulkanShaderModule
{
private:

    VkDevice        device = VK_NULL_HANDLE;
    VkShaderModule  shaderModule = VK_NULL_HANDLE;

public:

    // Lifetime
    VulkanShaderModule() = default;
    ~VulkanShaderModule();

    VulkanShaderModule(const VulkanShaderModule&) = delete;
    VulkanShaderModule& operator=(const VulkanShaderModule&) = delete;

    VulkanShaderModule(VulkanShaderModule&& other) noexcept;
    VulkanShaderModule& operator=(VulkanShaderModule&& other) noexcept;

    bool            Init(VkDevice device, const std::filesystem::path& spirvPath);
    void            CleanUp();

    // Access
    VkShaderModule  GetHandle() const { return shaderModule; }
    bool            IsValid() const { return shaderModule != VK_NULL_HANDLE; }

private:

    // SPIR-V
    static bool     ReadSpirvFile(const std::filesystem::path& path, std::vector<uint32_t>& code);
};
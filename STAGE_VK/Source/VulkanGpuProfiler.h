#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

class Profiler;

class VulkanGpuProfiler
{
private:
    static constexpr uint32_t FRAME_BEGIN_QUERY = 0;
    static constexpr uint32_t FRAME_END_QUERY = 1;
    static constexpr uint32_t QUERY_COUNT = 2;

    VkDevice device = VK_NULL_HANDLE;
    VkQueryPool queryPool = VK_NULL_HANDLE;

    float timestampPeriod = 0.0f;
    uint32_t timestampValidBits = 0;

    bool supported = false;
    bool frameActive = false;
    bool pendingResult = false;

public:
    // Lifetime
    bool                        Init(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamily);
    void                        CleanUp();

    // GPU Frame Profiling
    void                        BeginFrame(VkCommandBuffer commandBuffer);
    void                        EndFrame(VkCommandBuffer commandBuffer);
    void                        MarkSubmitted();
    void                        ResolvePreviousFrame(Profiler& profiler);

    // State
    bool                        IsSupported() const { return supported; }
};

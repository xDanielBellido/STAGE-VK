#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

class Profiler;

class VulkanGpuProfiler
{
private:
    static constexpr uint32_t QUERIES_PER_FRAME = 2;

    struct FrameQueryState
    {
        bool active = false;
        bool pending = false;
        uint64_t frameNumber = 0;
    };

    VkDevice device = VK_NULL_HANDLE;
    VkQueryPool queryPool = VK_NULL_HANDLE;

    float timestampPeriod = 0.0f;
    uint32_t timestampValidBits = 0;
    uint32_t frameCount = 0;

    bool supported = false;

    std::vector<FrameQueryState> frameStates;

public:
                                    // Lifetime
    bool                            Init(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamily, uint32_t frameCount);
    void                            CleanUp();

                                    // GPU Frame Profiling
    void                            BeginFrame(VkCommandBuffer commandBuffer, uint32_t frameSlot, uint64_t frameNumber);
    void                            EndFrame(VkCommandBuffer commandBuffer, uint32_t frameSlot);
    void                            MarkSubmitted(uint32_t frameSlot);
    void                            ResolveFrame(uint32_t frameSlot, Profiler& profiler);

                                    // State
    bool                            IsSupported() const { return supported; }
    float                           GetTimestampPeriod() const { return timestampPeriod; }
};
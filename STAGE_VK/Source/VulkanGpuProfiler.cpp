#include "VulkanGpuProfiler.h"

#include "Profiler.h"

#include <stdexcept>
#include <vector>

bool VulkanGpuProfiler::Init(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamily, uint32_t frameCount)
{
    this->device = device;
    this->frameCount = frameCount;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    timestampPeriod = properties.limits.timestampPeriod;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    if (graphicsQueueFamily >= queueFamilyCount)
    {
        return false;
    }

    timestampValidBits = queueFamilies[graphicsQueueFamily].timestampValidBits;

    if (timestampValidBits == 0)
    {
        supported = false;
        return true;
    }

    VkQueryPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = frameCount * QUERIES_PER_FRAME;

    if (vkCreateQueryPool(device, &createInfo, nullptr, &queryPool) != VK_SUCCESS)
    {
        return false;
    }

    frameStates.resize(frameCount);

    supported = true;
    return true;
}

void VulkanGpuProfiler::CleanUp()
{
    if (queryPool != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(device, queryPool, nullptr);
        queryPool = VK_NULL_HANDLE;
    }

    frameStates.clear();

    device = VK_NULL_HANDLE;
    timestampPeriod = 0.0f;
    timestampValidBits = 0;
    frameCount = 0;
    supported = false;
}

void VulkanGpuProfiler::BeginFrame(VkCommandBuffer commandBuffer, uint32_t frameSlot, uint64_t frameNumber)
{
    if (!supported || frameSlot >= frameStates.size())
    {
        return;
    }

    const uint32_t beginQuery = frameSlot * QUERIES_PER_FRAME;

    vkCmdResetQueryPool(commandBuffer, queryPool, beginQuery, QUERIES_PER_FRAME);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, beginQuery);

    FrameQueryState& state = frameStates[frameSlot];
    state.active = true;
    state.frameNumber = frameNumber;
}

void VulkanGpuProfiler::EndFrame(VkCommandBuffer commandBuffer, uint32_t frameSlot)
{
    if (!supported || frameSlot >= frameStates.size())
    {
        return;
    }

    FrameQueryState& state = frameStates[frameSlot];

    if (!state.active)
    {
        return;
    }

    const uint32_t endQuery = frameSlot * QUERIES_PER_FRAME + 1;

    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, endQuery);
}

void VulkanGpuProfiler::MarkSubmitted(uint32_t frameSlot)
{
    if (!supported || frameSlot >= frameStates.size())
    {
        return;
    }

    FrameQueryState& state = frameStates[frameSlot];

    if (!state.active)
    {
        return;
    }

    state.pending = true;
    state.active = false;
}

void VulkanGpuProfiler::ResolveFrame(uint32_t frameSlot, Profiler& profiler)
{
    if (!supported || frameSlot >= frameStates.size())
    {
        return;
    }

    FrameQueryState& state = frameStates[frameSlot];

    if (!state.pending)
    {
        return;
    }

    const uint32_t beginQuery = frameSlot * QUERIES_PER_FRAME;

    uint64_t timestamps[QUERIES_PER_FRAME]{};

    const VkResult result = vkGetQueryPoolResults(device, queryPool, beginQuery, QUERIES_PER_FRAME, sizeof(timestamps), timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

    if (result == VK_NOT_READY)
    {
        return;
    }

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to retrieve Vulkan GPU timestamp queries.");
    }

    uint64_t timestampDelta = 0;

    if (timestampValidBits >= 64)
    {
        timestampDelta = timestamps[1] - timestamps[0];
    }
    else
    {
        const uint64_t timestampMask = (1ULL << timestampValidBits) - 1ULL;
        timestampDelta = (timestamps[1] - timestamps[0]) & timestampMask;
    }

    const double gpuTimeNs = static_cast<double>(timestampDelta) * static_cast<double>(timestampPeriod);
    const float gpuTimeMs = static_cast<float>(gpuTimeNs / 1000000.0);

    profiler.SetGpuFrameTime(state.frameNumber, gpuTimeMs);

    state.pending = false;
}
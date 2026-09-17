#include "VulkanGpuProfiler.h"

#include "Profiler.h"

#include <stdexcept>
#include <vector>

bool VulkanGpuProfiler::Init(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamily)
{
    this->device = device;

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
    createInfo.queryCount = QUERY_COUNT;

    if (vkCreateQueryPool(device, &createInfo, nullptr, &queryPool) != VK_SUCCESS)
    {
        return false;
    }

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

    device = VK_NULL_HANDLE;

    timestampPeriod = 0.0f;
    timestampValidBits = 0;

    supported = false;
    frameActive = false;
    pendingResult = false;
}

void VulkanGpuProfiler::BeginFrame(VkCommandBuffer commandBuffer)
{
    if (!supported)
    {
        return;
    }

    vkCmdResetQueryPool(commandBuffer, queryPool, 0, QUERY_COUNT);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, FRAME_BEGIN_QUERY);

    frameActive = true;
}

void VulkanGpuProfiler::EndFrame(VkCommandBuffer commandBuffer)
{
    if (!supported || !frameActive)
    {
        return;
    }

    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, FRAME_END_QUERY);
}

void VulkanGpuProfiler::MarkSubmitted()
{
    if (!supported || !frameActive)
    {
        return;
    }

    pendingResult = true;
    frameActive = false;
}

void VulkanGpuProfiler::ResolvePreviousFrame(Profiler& profiler)
{
    if (!supported || !pendingResult)
    {
        return;
    }

    uint64_t timestamps[QUERY_COUNT]{};

    const VkResult result = vkGetQueryPoolResults(device, queryPool, 0, QUERY_COUNT, sizeof(timestamps), timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

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
        timestampDelta = timestamps[FRAME_END_QUERY] - timestamps[FRAME_BEGIN_QUERY];
    }
    else
    {
        const uint64_t timestampMask = (1ULL << timestampValidBits) - 1ULL;
        timestampDelta = (timestamps[FRAME_END_QUERY] - timestamps[FRAME_BEGIN_QUERY]) & timestampMask;
    }

    const double gpuTimeNs = static_cast<double>(timestampDelta) * static_cast<double>(timestampPeriod);
    const float gpuTimeMs = static_cast<float>(gpuTimeNs / 1000000.0);

    profiler.SetLatestGpuFrameTime(gpuTimeMs);

    pendingResult = false;
}
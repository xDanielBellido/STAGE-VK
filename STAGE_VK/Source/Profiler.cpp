#include "Profiler.h"

void Profiler::FinalizePreviousFrameTiming(float deltaTimeSeconds, float fps)
{
    if (!pendingFrameTiming || frameHistory.empty())
    {
        return;
    }

    frameHistory.back().fps = fps;
    frameHistory.back().frameTimeMs = deltaTimeSeconds * 1000.0f;

    pendingFrameTiming = false;
}

void Profiler::BeginFrame()
{
    recording = recordingRequested;

    if (!recording)
    {
        return;
    }

    currentFrame = {};
    currentFrame.frameNumber = nextFrameNumber++;

    currentCpuDepth = 0;
    frameStartTime = Clock::now();
}

void Profiler::EndFrame()
{
    if (!recording)
    {
        return;
    }

    const auto now = Clock::now();

    currentFrame.cpuFrameTimeMs = std::chrono::duration<float, std::milli>(now - frameStartTime).count();

    if (frameHistory.size() >= MAX_FRAME_HISTORY)
    {
        frameHistory.pop_front();
    }

    frameHistory.push_back(currentFrame);
    pendingFrameTiming = true;
}

std::size_t Profiler::BeginCpuEvent(const char* name)
{
    if (!recording)
    {
        return INVALID_EVENT_INDEX;
    }

    const auto now = Clock::now();

    CpuProfileEvent event{};
    event.name = name;
    event.startTimeMs = std::chrono::duration<float, std::milli>(now - frameStartTime).count();
    event.depth = currentCpuDepth++;

    currentFrame.cpuEvents.push_back(std::move(event));

    return currentFrame.cpuEvents.size() - 1;
}

void Profiler::EndCpuEvent(std::size_t eventIndex)
{
    if (!recording || eventIndex == INVALID_EVENT_INDEX || eventIndex >= currentFrame.cpuEvents.size())
    {
        return;
    }

    const auto now = Clock::now();
    const float endTimeMs = std::chrono::duration<float, std::milli>(now - frameStartTime).count();

    currentFrame.cpuEvents[eventIndex].durationMs = endTimeMs - currentFrame.cpuEvents[eventIndex].startTimeMs;

    if (currentCpuDepth > 0)
    {
        --currentCpuDepth;
    }
}

void Profiler::SetGpuFrameTime(uint64_t frameNumber, float gpuFrameTimeMs)
{
    for (auto it = frameHistory.rbegin(); it != frameHistory.rend(); ++it)
    {
        if (it->frameNumber == frameNumber)
        {
            it->gpuFrameTimeMs = gpuFrameTimeMs;
            return;
        }
    }
}

void Profiler::SetGpuDeviceInfo(const char* name, const char* type, uint32_t apiMajor, uint32_t apiMinor, uint32_t apiPatch, bool timestampSupported, float timestampPeriodNs)
{
    gpuDevice.name = name;
    gpuDevice.type = type;

    gpuDevice.apiMajor = apiMajor;
    gpuDevice.apiMinor = apiMinor;
    gpuDevice.apiPatch = apiPatch;

    gpuDevice.timestampSupported = timestampSupported;
    gpuDevice.timestampPeriodNs = timestampPeriodNs;
}

void Profiler::SetRenderingContext(uint32_t width, uint32_t height, uint32_t swapchainImageCount, uint32_t currentImageIndex, uint32_t commandBufferCount)
{
    if (!recording)
    {
        return;
    }

    currentFrame.rendering.width = width;
    currentFrame.rendering.height = height;

    currentFrame.rendering.swapchainImageCount = swapchainImageCount;
    currentFrame.rendering.currentImageIndex = currentImageIndex;

    currentFrame.rendering.commandBufferCount = commandBufferCount;
}

void Profiler::RecordRenderPass()
{
    if (recording)
    {
        ++currentFrame.rendering.renderPassCount;
    }
}

void Profiler::RecordQueueSubmission()
{
    if (recording)
    {
        ++currentFrame.rendering.queueSubmissionCount;
    }
}

void Profiler::RecordPresent()
{
    if (recording)
    {
        ++currentFrame.rendering.presentCount;
    }
}

void Profiler::StartRecording()
{
    recordingRequested = true;
}

void Profiler::StopRecording()
{
    recordingRequested = false;
}

void Profiler::Clear()
{
    frameHistory.clear();
    pendingFrameTiming = false;
}
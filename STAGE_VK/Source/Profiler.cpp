#include "Profiler.h"

void Profiler::BeginFrame(float deltaTimeSeconds, float fps)
{
    recording = recordingRequested;

    if (!recording)
    {
        return;
    }

    currentFrame = {};

    currentFrame.frameNumber = nextFrameNumber++;
    currentFrame.fps = fps;
    currentFrame.frameTimeMs = deltaTimeSeconds * 1000.0f;

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

void Profiler::SetLatestGpuFrameTime(float gpuFrameTimeMs)
{
    if (frameHistory.empty())
    {
        return;
    }

    frameHistory.back().gpuFrameTimeMs = gpuFrameTimeMs;
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
}
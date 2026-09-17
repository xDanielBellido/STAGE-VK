#pragma once

#include "FrameProfileData.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>

class Profiler
{
private:
    using Clock = std::chrono::steady_clock;

    static constexpr std::size_t MAX_FRAME_HISTORY = 240;
    static constexpr std::size_t INVALID_EVENT_INDEX = std::numeric_limits<std::size_t>::max();

    bool recording = true;
    bool recordingRequested = true;

    uint64_t nextFrameNumber = 0;
    uint32_t currentCpuDepth = 0;

    Clock::time_point frameStartTime{};

    FrameProfileData currentFrame;
    std::deque<FrameProfileData> frameHistory;

public:
                                    // Frame Recording
    void                            BeginFrame(float deltaTimeSeconds, float fps);
    void                            EndFrame();

                                    // CPU Profiling
    std::size_t                     BeginCpuEvent(const char* name);
    void                            EndCpuEvent(std::size_t eventIndex);

                                    // GPU Profiling
    void                            SetLatestGpuFrameTime(float gpuFrameTimeMs);

                                    // Rendering Profiling
    void                            SetRenderingContext(uint32_t width, uint32_t height, uint32_t swapchainImageCount, uint32_t currentImageIndex, uint32_t commandBufferCount);
    void                            RecordRenderPass();
    void                            RecordQueueSubmission();
    void                            RecordPresent();

                                    // Recording Control
    void                            StartRecording();
    void                            StopRecording();
    void                            Clear();

                                    // State
    bool                            IsRecording() const { return recordingRequested; }

                                    // Frame Data
    const FrameProfileData&         GetCurrentFrame() const { return currentFrame; }
    const FrameProfileData&         GetLatestFrame() const { return frameHistory.empty() ? currentFrame : frameHistory.back(); }
    const std::deque<FrameProfileData>& GetFrameHistory() const { return frameHistory; }
};
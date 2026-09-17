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

                                    // Recording Control
    void                            StartRecording();
    void                            StopRecording();
    void                            Clear();

                                    // State
    bool                            IsRecording() const { return recording; }

                                    // Frame Data
    const FrameProfileData&         GetCurrentFrame() const { return currentFrame; }
    const FrameProfileData&         GetLatestFrame() const { return frameHistory.empty() ? currentFrame : frameHistory.back(); }
    const std::deque<FrameProfileData>& GetFrameHistory() const { return frameHistory; }
};
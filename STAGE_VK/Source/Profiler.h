#pragma once

#include "FrameProfileData.h"

#include <cstddef>
#include <cstdint>
#include <deque>

class Profiler
{
private:
    static constexpr std::size_t MAX_FRAME_HISTORY = 240;

    bool recording = true;

    uint64_t nextFrameNumber = 0;

    FrameProfileData currentFrame;
    std::deque<FrameProfileData> frameHistory;

public:
                                    // Frame Recording
    void                            BeginFrame(float deltaTimeSeconds, float fps);
    void                            EndFrame();

                                    // Recording Control
    void                            StartRecording();
    void                            StopRecording();
    void                            Clear();

                                    // State
    bool                            IsRecording() const { return recording; }

                                    // Frame Data
    const FrameProfileData&         GetCurrentFrame() const { return currentFrame; }
    const std::deque<FrameProfileData>& GetFrameHistory() const { return frameHistory; }
};
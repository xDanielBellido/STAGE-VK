#pragma once

#include <cstdint>

struct FrameProfileData
{
    uint64_t frameNumber = 0;

    float fps = 0.0f;
    float frameTimeMs = 0.0f;

    float cpuFrameTimeMs = 0.0f;
    float gpuFrameTimeMs = 0.0f;
};
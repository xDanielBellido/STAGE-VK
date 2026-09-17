#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct CpuProfileEvent
{
    std::string name;

    float startTimeMs = 0.0f;
    float durationMs = 0.0f;

    uint32_t depth = 0;
};

struct FrameProfileData
{
    uint64_t frameNumber = 0;

    float fps = 0.0f;
    float frameTimeMs = 0.0f;

    float cpuFrameTimeMs = 0.0f;
    float gpuFrameTimeMs = 0.0f;

    std::vector<CpuProfileEvent> cpuEvents;
};
#pragma once

#include "FrameProfileData.h"

#include <optional>

class Profiler;

class PerformancePanel
{
public:
    void Draw(Profiler& profiler, bool* open);

private:
    void DrawToolbar(Profiler& profiler);
    void DrawSummary(const FrameProfileData& frame);
    void DrawFrameHistory(Profiler& profiler);

    void DrawOverview(const Profiler& profiler, const FrameProfileData& frame);
    void DrawCpuHierarchy(const FrameProfileData& frame);
    void DrawGpuProfiler(const Profiler& profiler, const FrameProfileData& frame);
    void DrawRendering(const FrameProfileData& frame);

private:
    std::optional<FrameProfileData> selectedFrame;
};
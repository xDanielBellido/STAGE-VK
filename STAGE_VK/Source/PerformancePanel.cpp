#include "PerformancePanel.h"

#include "Profiler.h"

#include "imgui.h"

#include <vector>

void PerformancePanel::Draw(const Profiler& profiler, bool* open)
{
    if (!ImGui::Begin("Performance", open))
    {
        ImGui::End();
        return;
    }

    const FrameProfileData& currentFrame = profiler.GetCurrentFrame();
    const auto& frameHistory = profiler.GetFrameHistory();

    ImGui::Text("Frame: %llu", static_cast<unsigned long long>(currentFrame.frameNumber));
    ImGui::Text("FPS: %.1f", currentFrame.fps);
    ImGui::Text("Frame Time: %.2f ms", currentFrame.frameTimeMs);

    std::vector<float> fpsHistory;
    fpsHistory.reserve(frameHistory.size());

    for (const FrameProfileData& frame : frameHistory)
    {
        fpsHistory.push_back(frame.fps);
    }

    if (!fpsHistory.empty())
    {
        ImGui::PlotHistogram("##FPS", fpsHistory.data(), static_cast<int>(fpsHistory.size()), 0, nullptr, 0.0f, 144.0f, ImVec2(0.0f, 100.0f));
    }
    else
    {
        ImGui::TextDisabled("Waiting for frame history...");
    }

    ImGui::End();
}
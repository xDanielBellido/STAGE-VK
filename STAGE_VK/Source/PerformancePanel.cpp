#include "PerformancePanel.h"

#include "imgui.h"

PerformancePanel::PerformancePanel()
{
    fpsHistory.resize(MAX_FPS_SAMPLES, 0.0f);
}

void PerformancePanel::AddFPS(float fps)
{
    fpsHistory[nextSample] = fps;
    nextSample = (nextSample + 1) % MAX_FPS_SAMPLES;
}

void PerformancePanel::Draw(bool* open)
{
    if (!ImGui::Begin("Performance", open))
    {
        ImGui::End();
        return;
    }

    const float currentFPS = fpsHistory[(nextSample + MAX_FPS_SAMPLES - 1) % MAX_FPS_SAMPLES];

    ImGui::Text("FPS: %.1f", currentFPS);
    ImGui::Text("Frame Time: %.2f ms", currentFPS > 0.0f ? 1000.0f / currentFPS : 0.0f);

    ImGui::PlotHistogram("##FPS", fpsHistory.data(), static_cast<int>(fpsHistory.size()), nextSample, nullptr, 0.0f, 144.0f, ImVec2(0.0f, 100.0f));

    ImGui::End();
}
#pragma once

#include <vector>

class PerformancePanel
{
public:
    PerformancePanel();

    void AddFPS(float fps);
    void Draw(bool* open);

private:
    static constexpr int MAX_FPS_SAMPLES = 120;

    std::vector<float> fpsHistory;
    int nextSample = 0;
};
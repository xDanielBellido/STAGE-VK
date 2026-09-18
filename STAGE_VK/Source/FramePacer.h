#pragma once

#include <chrono>

class FramePacer
{
private:
    using Clock = std::chrono::steady_clock;

    Clock::time_point frameStart{};

public:
    void BeginFrame();
    void WaitForRefreshRate(int refreshRate);
};

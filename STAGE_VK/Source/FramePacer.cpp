#include "FramePacer.h"

#include <thread>

void FramePacer::BeginFrame()
{
    frameStart = Clock::now();
}

void FramePacer::WaitForRefreshRate(int refreshRate)
{
    if (refreshRate <= 0)
    {
        return;
    }

    const std::chrono::duration<double> targetDuration(1.0 / static_cast<double>(refreshRate));
    const auto targetTime = frameStart + std::chrono::duration_cast<Clock::duration>(targetDuration);

    auto now = Clock::now();

    if (now >= targetTime)
    {
        return;
    }

    constexpr auto spinThreshold = std::chrono::milliseconds(2);

    if (targetTime - now > spinThreshold)
    {
        std::this_thread::sleep_until(targetTime - spinThreshold);
    }

    while (Clock::now() < targetTime)
    {
    }
}
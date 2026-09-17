#include "Profiler.h"

void Profiler::BeginFrame(float deltaTimeSeconds, float fps)
{
    if (!recording)
    {
        return;
    }

    currentFrame = {};

    currentFrame.frameNumber = nextFrameNumber++;
    currentFrame.fps = fps;
    currentFrame.frameTimeMs = deltaTimeSeconds * 1000.0f;
}

void Profiler::EndFrame()
{
    if (!recording)
    {
        return;
    }

    if (frameHistory.size() >= MAX_FRAME_HISTORY)
    {
        frameHistory.pop_front();
    }

    frameHistory.push_back(currentFrame);
}

void Profiler::StartRecording()
{
    recording = true;
}

void Profiler::StopRecording()
{
    recording = false;
}

void Profiler::Clear()
{
    frameHistory.clear();
}
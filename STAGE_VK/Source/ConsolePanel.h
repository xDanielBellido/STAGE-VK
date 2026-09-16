/*
    ==========================================================================
    ConsolePanel
    ==========================================================================

    Editor panel used to display the log history captured by MemoryLogSink.

    ConsolePanel does not produce or own log messages.
    It simply reads the current logging history and presents it through ImGui.

    More advanced console features such as filtering, severity colors and
    auto-scrolling can be added later without changing the logging system.
*/

#pragma once

#include "imgui.h"

class ConsolePanel
{
public:
    void Draw(bool* open);

private:
    ImGuiTextFilter filter;
    bool autoScroll = true;
};
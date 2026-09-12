
/*
    ===========================================================================
    AppConfig
    ===========================================================================

    This structure stores the basic startup configuration of the application.

    Its purpose is to keep the most important initialization parameters
    in a single place, so the engine does not rely on hardcoded values
    spread across different files.

    At the moment, this configuration only contains window-related data,
    but in the future it could be expanded with options such as:
    - fullscreen mode
    - VSync
    - frames in flight
    - project name
    - selected rendering backend

    AppConfig does not create or manage anything by itself.
    It only describes how the application should start.
*/

#pragma once

#include <string>
#include <cstdint>

struct AppConfig
{
    // App Window
    std::string title = "STAGE_VK";
    uint32_t width = 1280;
    uint32_t height = 720;
    bool resizable = true;
};
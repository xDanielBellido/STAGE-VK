#pragma once

#include <spdlog/common.h>

#include <string>

struct LogEntry
{
    spdlog::level::level_enum level = spdlog::level::info;
    std::string message;
};
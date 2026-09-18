#pragma once

#include <spdlog/common.h>

#include <string>

struct LogEntry
{
    spdlog::level::level_enum level = spdlog::level::info;
    spdlog::log_clock::time_point timestamp{};

    std::string message;
    std::string file;
    std::string function;

    int line = 0;
};
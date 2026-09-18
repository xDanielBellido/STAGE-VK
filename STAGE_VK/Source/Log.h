/*
    ==========================================================================
    Log
    ==========================================================================

    Central logging system used by STAGE VK.

    The engine logs through this class instead of depending directly on a
    specific spdlog configuration.

    Log is responsible for:
    - initializing the engine logger
    - configuring logging sinks and formatting
    - exposing the logger to engine systems
    - shutting the logging system down safely

    Additional sinks, such as the ImGui Console, can later be attached without
    changing logging calls throughout the engine.
*/

#pragma once

#include <memory>

#include <spdlog/spdlog.h>

class MemoryLogSink;

class Log
{
private:
    static std::shared_ptr<spdlog::logger> logger;
    static std::shared_ptr<MemoryLogSink> memorySink;

public:
    // Logging Lifetime
    static void             Init();
    static void             Shutdown();

    // Logging State
    static bool             IsInitialized();

    // Logger Access
    static const std::shared_ptr<spdlog::logger>& GetLogger() { return logger; }
    static const std::shared_ptr<MemoryLogSink>& GetMemorySink() { return memorySink; }
};

#define LOG_TRACE(...)      do { if (Log::IsInitialized()) SPDLOG_LOGGER_CALL(Log::GetLogger(), spdlog::level::trace, __VA_ARGS__); } while (false)
#define LOG_DEBUG(...)      do { if (Log::IsInitialized()) SPDLOG_LOGGER_CALL(Log::GetLogger(), spdlog::level::debug, __VA_ARGS__); } while (false)
#define LOG_INFO(...)       do { if (Log::IsInitialized()) SPDLOG_LOGGER_CALL(Log::GetLogger(), spdlog::level::info, __VA_ARGS__); } while (false)
#define LOG_WARN(...)       do { if (Log::IsInitialized()) SPDLOG_LOGGER_CALL(Log::GetLogger(), spdlog::level::warn, __VA_ARGS__); } while (false)
#define LOG_ERROR(...)      do { if (Log::IsInitialized()) SPDLOG_LOGGER_CALL(Log::GetLogger(), spdlog::level::err, __VA_ARGS__); } while (false)
#define LOG_CRITICAL(...)   do { if (Log::IsInitialized()) SPDLOG_LOGGER_CALL(Log::GetLogger(), spdlog::level::critical, __VA_ARGS__); } while (false)
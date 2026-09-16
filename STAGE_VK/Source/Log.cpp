#include "Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Log::logger = nullptr;

void Log::Init()
{
    if (logger)
    {
        return;
    }

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::trace);

    logger = std::make_shared<spdlog::logger>("STAGE_VK", consoleSink);

    logger->set_level(spdlog::level::trace);
    logger->set_pattern("[%H:%M:%S] [%^%l%$] %v");
    logger->flush_on(spdlog::level::err);

    spdlog::set_default_logger(logger);
}

void Log::Shutdown()
{
    if (!logger)
    {
        return;
    }

    logger->flush();

    spdlog::shutdown();
    logger.reset();
}

bool Log::IsInitialized()
{
    return logger != nullptr;
}
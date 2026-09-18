#include "Log.h"
#include "MemoryLogSink.h"

#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Log::logger = nullptr;
std::shared_ptr<MemoryLogSink> Log::memorySink = nullptr;

void Log::Init()
{
    if (logger)
    {
        return;
    }

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::trace);

    memorySink = std::make_shared<MemoryLogSink>();
    memorySink->set_level(spdlog::level::trace);

    logger = std::make_shared<spdlog::logger>("STAGE_VK", spdlog::sinks_init_list{ consoleSink, memorySink });

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
    memorySink.reset();
}

bool Log::IsInitialized()
{
    return logger != nullptr;
}
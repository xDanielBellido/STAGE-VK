#include "MemoryLogSink.h"

#include <algorithm>

MemoryLogSink::MemoryLogSink(std::size_t maxEntries)
    : maxEntries(std::max<std::size_t>(1, maxEntries))
{
}

std::vector<LogEntry> MemoryLogSink::GetEntries()
{
    std::lock_guard<std::mutex> lock(this->mutex_);

    return std::vector<LogEntry>(entries.begin(), entries.end());
}

void MemoryLogSink::Clear()
{
    std::lock_guard<std::mutex> lock(this->mutex_);

    entries.clear();
}

void MemoryLogSink::sink_it_(const spdlog::details::log_msg& message)
{
    if (entries.size() >= maxEntries)
    {
        entries.pop_front();
    }

    LogEntry entry{};
    entry.level = message.level;
    entry.message = std::string(message.payload.data(), message.payload.size());

    entries.push_back(std::move(entry));
}

void MemoryLogSink::flush_()
{
}
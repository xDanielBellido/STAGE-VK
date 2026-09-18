/*
    ==========================================================================
    MemoryLogSink
    ==========================================================================

    Custom spdlog sink used to keep a copy of engine log messages in memory.

    The standard console sink writes messages to the terminal.
    MemoryLogSink stores the same messages as LogEntry objects so editor tools
    such as the ImGui Console can read and display them later.

    The stored history is bounded to prevent logging from growing indefinitely
    during long engine sessions.
*/

#pragma once

#include "LogEntry.h"

#include <spdlog/sinks/base_sink.h>

#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>

class MemoryLogSink final : public spdlog::sinks::base_sink<std::mutex>
{
public:
    explicit MemoryLogSink(std::size_t maxEntries = 2000);

    std::vector<LogEntry> GetEntries();
    void Clear();

protected:
    void sink_it_(const spdlog::details::log_msg& message) override;
    void flush_() override;

private:
    std::deque<LogEntry> entries;
    std::size_t maxEntries = 2000;
};
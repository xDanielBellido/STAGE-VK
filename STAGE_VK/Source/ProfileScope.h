#pragma once

#include "Profiler.h"

#include <cstddef>

class ProfileScope
{
private:
    Profiler* profiler = nullptr;
    std::size_t eventIndex = 0;

public:
    ProfileScope(Profiler& profiler, const char* name) : profiler(&profiler)
    {
        eventIndex = profiler.BeginCpuEvent(name);
    }

    ~ProfileScope()
    {
        profiler->EndCpuEvent(eventIndex);
    }

    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;
};

#define PROFILE_CONCAT_INTERNAL(a, b) a##b
#define PROFILE_CONCAT(a, b) PROFILE_CONCAT_INTERNAL(a, b)
#define PROFILE_SCOPE(profiler, name) ProfileScope PROFILE_CONCAT(profileScope_, __LINE__)(profiler, name)
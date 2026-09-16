#include "ConsolePanel.h"

#include "Log.h"
#include "MemoryLogSink.h"

#include "imgui.h"

#include <string>

namespace
{
    const char* GetLevelLabel(spdlog::level::level_enum level)
    {
        switch (level)
        {
        case spdlog::level::trace:      return "Trace";
        case spdlog::level::debug:      return "Debug";
        case spdlog::level::info:       return "Info";
        case spdlog::level::warn:       return "Warning";
        case spdlog::level::err:        return "Error";
        case spdlog::level::critical:   return "Critical";
        default:                        return "Unknown";
        }
    }

    ImVec4 GetLevelColor(spdlog::level::level_enum level)
    {
        switch (level)
        {
        case spdlog::level::trace:      return ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
        case spdlog::level::debug:      return ImVec4(0.40f, 0.75f, 1.00f, 1.0f);
        case spdlog::level::info:       return ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
        case spdlog::level::warn:       return ImVec4(1.00f, 0.80f, 0.20f, 1.0f);
        case spdlog::level::err:        return ImVec4(1.00f, 0.35f, 0.35f, 1.0f);
        case spdlog::level::critical:   return ImVec4(1.00f, 0.20f, 0.65f, 1.0f);
        default:                        return ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
        }
    }
}

void ConsolePanel::Draw(bool* open)
{
    if (!ImGui::Begin("Console", open))
    {
        ImGui::End();
        return;
    }

    const auto& memorySink = Log::GetMemorySink();

    if (ImGui::Button("Clear") && memorySink)
    {
        memorySink->Clear();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll);

    ImGui::SameLine();
    filter.Draw("Filter", 250.0f);

    ImGui::Separator();

    if (memorySink)
    {
        const auto entries = memorySink->GetEntries();

        ImGui::BeginChild("ConsoleLogRegion", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

        for (const LogEntry& entry : entries)
        {
            std::string line = "[" + std::string(GetLevelLabel(entry.level)) + "] " + entry.message;

            if (!filter.PassFilter(line.c_str()))
            {
                continue;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, GetLevelColor(entry.level));
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();
        }

        if (autoScroll)
        {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
    }

    ImGui::End();
}
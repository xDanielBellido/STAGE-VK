#include "ConsolePanel.h"

#include "EditorIcons.h"
#include "Log.h"
#include "MemoryLogSink.h"

#include "imgui.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    enum class ConsoleSeverity
    {
        Log,
        Warning,
        Error
    };

    struct DisplayEntry
    {
        const LogEntry* entry = nullptr;
        int count = 1;
    };

    // =========================================================================
    // Severity
    // =========================================================================

    ConsoleSeverity GetConsoleSeverity(spdlog::level::level_enum level)
    {
        if (level == spdlog::level::warn)
        {
            return ConsoleSeverity::Warning;
        }

        if (level == spdlog::level::err || level == spdlog::level::critical)
        {
            return ConsoleSeverity::Error;
        }

        return ConsoleSeverity::Log;
    }

    const char* GetLevelName(spdlog::level::level_enum level)
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

    ImVec4 GetSeverityColor(ConsoleSeverity severity)
    {
        switch (severity)
        {
        case ConsoleSeverity::Log:      return ImVec4(0.65f, 0.80f, 1.00f, 1.00f);
        case ConsoleSeverity::Warning:  return ImVec4(1.00f, 0.75f, 0.15f, 1.00f);
        case ConsoleSeverity::Error:    return ImVec4(1.00f, 0.30f, 0.30f, 1.00f);
        }

        return ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    }

    bool PassSeverityFilter(const LogEntry& entry, bool showLogs, bool showWarnings, bool showErrors)
    {
        switch (GetConsoleSeverity(entry.level))
        {
        case ConsoleSeverity::Log:      return showLogs;
        case ConsoleSeverity::Warning:  return showWarnings;
        case ConsoleSeverity::Error:    return showErrors;
        }

        return true;
    }

    // =========================================================================
    // Formatting
    // =========================================================================

    std::string FormatTimestamp(const spdlog::log_clock::time_point& timestamp)
    {
        const std::time_t time = spdlog::log_clock::to_time_t(timestamp);

        std::tm localTime{};
        localtime_s(&localTime, &time);

        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count() % 1000;

        char buffer[32]{};
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03lld", localTime.tm_hour, localTime.tm_min, localTime.tm_sec, static_cast<long long>(milliseconds));

        return buffer;
    }

    std::string GetFileName(const std::string& path)
    {
        const std::size_t position = path.find_last_of("\\/");

        if (position == std::string::npos)
        {
            return path;
        }

        return path.substr(position + 1);
    }

    // =========================================================================
    // Collapse
    // =========================================================================

    std::string MakeCollapseKey(const LogEntry& entry)
    {
        return std::to_string(static_cast<int>(entry.level)) + "|" + entry.message;
    }

    std::vector<DisplayEntry> BuildDisplayEntries(const std::vector<LogEntry>& entries, bool collapse)
    {
        std::vector<DisplayEntry> result;

        if (!collapse)
        {
            result.reserve(entries.size());

            for (const LogEntry& entry : entries)
            {
                result.push_back({ &entry, 1 });
            }

            return result;
        }

        std::unordered_map<std::string, std::size_t> lookup;

        for (const LogEntry& entry : entries)
        {
            const std::string key = MakeCollapseKey(entry);
            const auto found = lookup.find(key);

            if (found != lookup.end())
            {
                DisplayEntry& displayEntry = result[found->second];

                ++displayEntry.count;

                // Keep metadata from the latest occurrence.
                displayEntry.entry = &entry;

                continue;
            }

            lookup[key] = result.size();
            result.push_back({ &entry, 1 });
        }

        return result;
    }

    // =========================================================================
    // Icon Drawing
    // =========================================================================
    bool IconButton(const char* id, const char* icon, const char* label, bool selected = false, const ImVec4* iconColor = nullptr, const char* tooltip = nullptr)
    {
        const ImGuiStyle& style = ImGui::GetStyle();

        const float height = ImGui::GetFrameHeight();
        const float iconArea = height - style.FramePadding.y * 2.0f;

        const bool hasLabel = label && label[0] != '\0';

        const ImVec2 iconSize = ImGui::CalcTextSize(icon);
        const ImVec2 textSize = hasLabel ? ImGui::CalcTextSize(label) : ImVec2(0.0f, 0.0f);

        float width = style.FramePadding.x * 2.0f + iconArea;

        if (hasLabel)
        {
            width += style.ItemInnerSpacing.x + textSize.x;
        }

        const ImVec2 position = ImGui::GetCursorScreenPos();

        ImGui::InvisibleButton(id, ImVec2(width, height));

        const bool hovered = ImGui::IsItemHovered();
        const bool active = ImGui::IsItemActive();
        const bool clicked = ImGui::IsItemClicked();

        ImU32 backgroundColor = ImGui::GetColorU32(ImGuiCol_Button);

        if (selected)
        {
            backgroundColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);
        }

        if (hovered)
        {
            backgroundColor = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        }

        if (active)
        {
            backgroundColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRectFilled(position, ImVec2(position.x + width, position.y + height), backgroundColor, style.FrameRounding);

        const ImVec2 iconPosition(
            position.x + style.FramePadding.x + (iconArea - iconSize.x) * 0.5f,
            position.y + (height - iconSize.y) * 0.5f
        );

        const ImU32 actualIconColor = iconColor ? ImGui::GetColorU32(*iconColor) : ImGui::GetColorU32(ImGuiCol_Text);

        drawList->AddText(iconPosition, actualIconColor, icon);

        if (hasLabel)
        {
            const ImVec2 textPosition(
                position.x + style.FramePadding.x + iconArea + style.ItemInnerSpacing.x,
                position.y + (height - textSize.y) * 0.5f
            );

            drawList->AddText(textPosition, ImGui::GetColorU32(ImGuiCol_Text), label);
        }

        if (tooltip && hovered)
        {
            ImGui::SetTooltip("%s", tooltip);
        }

        return clicked;
    }

    void DrawSearchBox(ImGuiTextFilter& filter)
    {
        const float frameHeight = ImGui::GetFrameHeight();
        const ImVec2 iconPosition = ImGui::GetCursorScreenPos();

        ImGui::Dummy(ImVec2(frameHeight, frameHeight));

        const ImVec2 iconSize = ImGui::CalcTextSize(ICON_FA_MAGNIFYING_GLASS);

        ImGui::GetWindowDrawList()->AddText(
            ImVec2(
                iconPosition.x + (frameHeight - iconSize.x) * 0.5f,
                iconPosition.y + (frameHeight - iconSize.y) * 0.5f
            ),
            ImGui::GetColorU32(ImGuiCol_TextDisabled),
            ICON_FA_MAGNIFYING_GLASS
        );

        ImGui::SameLine(0.0f, 3.0f);

        filter.Draw("##ConsoleSearch", 250.0f);
    }

    bool IsSameEntry(const LogEntry& a, const LogEntry& b)
    {
        return a.level == b.level &&
            a.message == b.message &&
            a.file == b.file &&
            a.line == b.line &&
            a.timestamp == b.timestamp;
    }
}

// =============================================================================
// ConsolePanel
// =============================================================================

void ConsolePanel::Draw(bool* open)
{
    if (!ImGui::Begin("Console", open))
    {
        ImGui::End();
        return;
    }

    const auto& memorySink = Log::GetMemorySink();

    if (!memorySink)
    {
        ImGui::TextDisabled("Logging system unavailable.");
        ImGui::End();
        return;
    }

    auto entries = memorySink->GetEntries();

    // =========================================================================
    // Counters
    // =========================================================================

    int logCount = 0;
    int warningCount = 0;
    int errorCount = 0;

    for (const LogEntry& entry : entries)
    {
        switch (GetConsoleSeverity(entry.level))
        {
        case ConsoleSeverity::Log:      ++logCount; break;
        case ConsoleSeverity::Warning:  ++warningCount; break;
        case ConsoleSeverity::Error:    ++errorCount; break;
        }
    }

    // =========================================================================
    // Toolbar
    // =========================================================================

    if (IconButton("##ConsoleClear", ICON_FA_TRASH_CAN, "Clear", false, nullptr, "Clear console"))
    {
        memorySink->Clear();
        entries.clear();

        selectedEntry.reset();
        selectedRepeatCount = 1;
    }

    ImGui::SameLine();

    if (IconButton("##ConsoleCollapse", ICON_FA_LAYER_GROUP, "Collapse", collapse, nullptr, "Collapse identical messages"))
    {
        collapse = !collapse;
    }

    ImGui::SameLine();

    if (IconButton("##ConsoleAutoScroll", ICON_FA_ARROW_DOWN, "Auto Scroll", autoScroll, nullptr, "Automatically scroll to new messages"))
    {
        autoScroll = !autoScroll;
    }

    ImGui::SameLine();

    DrawSearchBox(filter);

    ImGui::SameLine();

    const ImVec4 logColor = GetSeverityColor(ConsoleSeverity::Log);
    const ImVec4 warningColor = GetSeverityColor(ConsoleSeverity::Warning);
    const ImVec4 errorColor = GetSeverityColor(ConsoleSeverity::Error);

    const std::string logCountText = std::to_string(logCount);
    const std::string warningCountText = std::to_string(warningCount);
    const std::string errorCountText = std::to_string(errorCount);

    if (IconButton("##ConsoleLogs", ICON_FA_CIRCLE_INFO, logCountText.c_str(), showLogs, &logColor, "Show / hide logs"))
    {
        showLogs = !showLogs;
    }

    ImGui::SameLine();

    if (IconButton("##ConsoleWarnings", ICON_FA_TRIANGLE_EXCLAMATION, warningCountText.c_str(), showWarnings, &warningColor, "Show / hide warnings"))
    {
        showWarnings = !showWarnings;
    }

    ImGui::SameLine();

    if (IconButton("##ConsoleErrors", ICON_FA_CIRCLE_XMARK, errorCountText.c_str(), showErrors, &errorColor, "Show / hide errors"))
    {
        showErrors = !showErrors;
    }

    ImGui::Separator();

    // =========================================================================
    // Build visible log list
    // =========================================================================

    const std::vector<DisplayEntry> displayEntries = BuildDisplayEntries(entries, collapse);

    const ImGuiStyle& style = ImGui::GetStyle();

    const float detailsHeight = selectedEntry ? 165.0f : 0.0f;
    const float availableHeight = ImGui::GetContentRegionAvail().y;

    const float listHeight = selectedEntry
        ? std::max(120.0f, availableHeight - detailsHeight - style.ItemSpacing.y)
        : availableHeight;

    const bool hasNewEntries = entries.size() > lastEntryCount;

    ImGui::BeginChild("ConsoleLogRegion", ImVec2(0.0f, listHeight), true);

    const float rowHeight = ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f;

    for (std::size_t i = 0; i < displayEntries.size(); ++i)
    {
        const DisplayEntry& displayEntry = displayEntries[i];
        const LogEntry& entry = *displayEntry.entry;

        if (!PassSeverityFilter(entry, showLogs, showWarnings, showErrors))
        {
            continue;
        }

        const std::string searchableText = entry.message + " " + entry.file + " " + entry.function;

        if (!filter.PassFilter(searchableText.c_str()))
        {
            continue;
        }

        const bool selected = selectedEntry && IsSameEntry(*selectedEntry, entry);

        ImGui::PushID(static_cast<int>(i));

        const float availableWidth = ImGui::GetContentRegionAvail().x;

        if (ImGui::Selectable("##ConsoleRow", selected, ImGuiSelectableFlags_None, ImVec2(availableWidth, rowHeight)))
        {
            selectedEntry = entry;
            selectedRepeatCount = displayEntry.count;
        }

        const ImVec2 rowMin = ImGui::GetItemRectMin();
        const ImVec2 rowMax = ImGui::GetItemRectMax();

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        const ConsoleSeverity severity = GetConsoleSeverity(entry.level);
        const ImVec4 severityColor = GetSeverityColor(severity);

        const char* severityIcon = ICON_FA_CIRCLE_INFO;

        if (severity == ConsoleSeverity::Warning)
        {
            severityIcon = ICON_FA_TRIANGLE_EXCLAMATION;
        }
        else if (severity == ConsoleSeverity::Error)
        {
            severityIcon = ICON_FA_CIRCLE_XMARK;
        }

        const float centerY = (rowMin.y + rowMax.y) * 0.5f;
        const ImVec2 severityIconSize = ImGui::CalcTextSize(severityIcon);

        drawList->AddText(
            ImVec2(rowMin.x + 8.0f, centerY - severityIconSize.y * 0.5f),
            ImGui::GetColorU32(severityColor),
            severityIcon
        );

        const std::string timestamp = FormatTimestamp(entry.timestamp);
        const ImVec2 timestampSize = ImGui::CalcTextSize(timestamp.c_str());

        const float timestampX = rowMax.x - timestampSize.x - 8.0f;

        float messageEndX = timestampX - 12.0f;

        // Repeat-count badge.
        if (displayEntry.count > 1)
        {
            const std::string countText = std::to_string(displayEntry.count);
            const ImVec2 countTextSize = ImGui::CalcTextSize(countText.c_str());

            const float badgeWidth = std::max(26.0f, countTextSize.x + 12.0f);
            const float badgeHeight = rowHeight - 6.0f;

            const float badgeX = timestampX - badgeWidth - 12.0f;
            const float badgeY = rowMin.y + 3.0f;

            drawList->AddRectFilled(
                ImVec2(badgeX, badgeY),
                ImVec2(badgeX + badgeWidth, badgeY + badgeHeight),
                ImGui::GetColorU32(ImGuiCol_FrameBg),
                badgeHeight * 0.5f
            );

            drawList->AddText(
                ImVec2(badgeX + (badgeWidth - countTextSize.x) * 0.5f, badgeY + (badgeHeight - countTextSize.y) * 0.5f),
                ImGui::GetColorU32(ImGuiCol_Text),
                countText.c_str()
            );

            messageEndX = badgeX - 10.0f;
        }

        const ImVec2 messagePosition(rowMin.x + 30.0f, rowMin.y + style.FramePadding.y);

        drawList->PushClipRect(messagePosition, ImVec2(messageEndX, rowMax.y), true);
        drawList->AddText(messagePosition, ImGui::GetColorU32(ImGuiCol_Text), entry.message.c_str());
        drawList->PopClipRect();

        drawList->AddText(
            ImVec2(timestampX, rowMin.y + style.FramePadding.y),
            ImGui::GetColorU32(ImGuiCol_TextDisabled),
            timestamp.c_str()
        );

        ImGui::PopID();
    }

    if (autoScroll && hasNewEntries)
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    lastEntryCount = entries.size();

    // =========================================================================
    // Selected Entry Details
    // =========================================================================

    if (selectedEntry)
    {
        ImGui::Separator();

        ImGui::BeginChild("ConsoleDetails", ImVec2(0.0f, 0.0f), true);

        const ConsoleSeverity severity = GetConsoleSeverity(selectedEntry->level);
        const ImVec4 severityColor = GetSeverityColor(severity);

        const char* severityIcon = ICON_FA_CIRCLE_INFO;

        if (severity == ConsoleSeverity::Warning)
        {
            severityIcon = ICON_FA_TRIANGLE_EXCLAMATION;
        }
        else if (severity == ConsoleSeverity::Error)
        {
            severityIcon = ICON_FA_CIRCLE_XMARK;
        }

        ImGui::TextColored(severityColor, "%s  %s", severityIcon, GetLevelName(selectedEntry->level));

        ImGui::SameLine();

        if (selectedRepeatCount > 1)
        {
            ImGui::TextDisabled("(%d occurrences)", selectedRepeatCount);
        }

        ImGui::SameLine();

        if (IconButton("##ConsoleCopy", ICON_FA_COPY, "Copy", false, nullptr, "Copy message to clipboard"))
        {
            ImGui::SetClipboardText(selectedEntry->message.c_str());
        }

        ImGui::Separator();

        ImGui::PushTextWrapPos();
        ImGui::TextUnformatted(selectedEntry->message.c_str());
        ImGui::PopTextWrapPos();

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::TextDisabled("Time");
        ImGui::SameLine(90.0f);
        ImGui::TextUnformatted(FormatTimestamp(selectedEntry->timestamp).c_str());

        ImGui::TextDisabled("Source");
        ImGui::SameLine(90.0f);

        if (!selectedEntry->file.empty())
        {
            const std::string source = GetFileName(selectedEntry->file) + ":" + std::to_string(selectedEntry->line);
            ImGui::TextUnformatted(source.c_str());

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", selectedEntry->file.c_str());
            }
        }
        else
        {
            ImGui::TextDisabled("Unavailable");
        }

        ImGui::TextDisabled("Function");
        ImGui::SameLine(90.0f);

        if (!selectedEntry->function.empty())
        {
            ImGui::TextUnformatted(selectedEntry->function.c_str());
        }
        else
        {
            ImGui::TextDisabled("Unavailable");
        }

        ImGui::TextDisabled("Level");
        ImGui::SameLine(90.0f);
        ImGui::TextUnformatted(GetLevelName(selectedEntry->level));

        ImGui::EndChild();
    }

    ImGui::End();
}
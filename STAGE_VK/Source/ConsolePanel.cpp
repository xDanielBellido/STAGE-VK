#include "ConsolePanel.h"

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

    enum class ConsoleIcon
    {
        Trash,
        Collapse,
        AutoScroll,
        Search,
        Info,
        Warning,
        Error,
        Copy
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

    void DrawIcon(ImDrawList* drawList, ConsoleIcon icon, const ImVec2& center, float radius, ImU32 color)
    {
        const float thickness = 1.6f;

        switch (icon)
        {
        case ConsoleIcon::Info:
        {
            drawList->AddCircle(center, radius, color, 20, thickness);
            drawList->AddCircleFilled(ImVec2(center.x, center.y - radius * 0.35f), radius * 0.11f, color);
            drawList->AddLine(ImVec2(center.x, center.y - radius * 0.05f), ImVec2(center.x, center.y + radius * 0.45f), color, thickness);
            break;
        }

        case ConsoleIcon::Warning:
        {
            const ImVec2 top(center.x, center.y - radius);
            const ImVec2 left(center.x - radius, center.y + radius * 0.80f);
            const ImVec2 right(center.x + radius, center.y + radius * 0.80f);

            drawList->AddTriangle(top, left, right, color, thickness);
            drawList->AddLine(ImVec2(center.x, center.y - radius * 0.35f), ImVec2(center.x, center.y + radius * 0.25f), color, thickness);
            drawList->AddCircleFilled(ImVec2(center.x, center.y + radius * 0.50f), radius * 0.09f, color);
            break;
        }

        case ConsoleIcon::Error:
        {
            drawList->AddCircle(center, radius, color, 20, thickness);
            drawList->AddLine(ImVec2(center.x - radius * 0.42f, center.y - radius * 0.42f), ImVec2(center.x + radius * 0.42f, center.y + radius * 0.42f), color, thickness);
            drawList->AddLine(ImVec2(center.x + radius * 0.42f, center.y - radius * 0.42f), ImVec2(center.x - radius * 0.42f, center.y + radius * 0.42f), color, thickness);
            break;
        }

        case ConsoleIcon::Trash:
        {
            const float halfWidth = radius * 0.60f;

            drawList->AddRect(ImVec2(center.x - halfWidth, center.y - radius * 0.35f), ImVec2(center.x + halfWidth, center.y + radius * 0.75f), color, 1.0f, 0, thickness);
            drawList->AddLine(ImVec2(center.x - radius * 0.75f, center.y - radius * 0.55f), ImVec2(center.x + radius * 0.75f, center.y - radius * 0.55f), color, thickness);
            drawList->AddLine(ImVec2(center.x - radius * 0.25f, center.y - radius * 0.80f), ImVec2(center.x + radius * 0.25f, center.y - radius * 0.80f), color, thickness);
            break;
        }

        case ConsoleIcon::Collapse:
        {
            drawList->AddRect(ImVec2(center.x - radius * 0.80f, center.y - radius * 0.45f), ImVec2(center.x + radius * 0.40f, center.y + radius * 0.75f), color, 1.0f, 0, thickness);
            drawList->AddRect(ImVec2(center.x - radius * 0.35f, center.y - radius * 0.80f), ImVec2(center.x + radius * 0.80f, center.y + radius * 0.35f), color, 1.0f, 0, thickness);
            break;
        }

        case ConsoleIcon::AutoScroll:
        {
            drawList->AddLine(ImVec2(center.x, center.y - radius * 0.80f), ImVec2(center.x, center.y + radius * 0.45f), color, thickness);
            drawList->AddTriangleFilled(ImVec2(center.x - radius * 0.45f, center.y + radius * 0.15f), ImVec2(center.x + radius * 0.45f, center.y + radius * 0.15f), ImVec2(center.x, center.y + radius * 0.80f), color);
            break;
        }

        case ConsoleIcon::Search:
        {
            drawList->AddCircle(ImVec2(center.x - radius * 0.15f, center.y - radius * 0.15f), radius * 0.58f, color, 16, thickness);
            drawList->AddLine(ImVec2(center.x + radius * 0.28f, center.y + radius * 0.28f), ImVec2(center.x + radius * 0.85f, center.y + radius * 0.85f), color, thickness);
            break;
        }

        case ConsoleIcon::Copy:
        {
            drawList->AddRect(ImVec2(center.x - radius * 0.70f, center.y - radius * 0.45f), ImVec2(center.x + radius * 0.35f, center.y + radius * 0.70f), color, 1.0f, 0, thickness);
            drawList->AddRect(ImVec2(center.x - radius * 0.30f, center.y - radius * 0.75f), ImVec2(center.x + radius * 0.70f, center.y + radius * 0.35f), color, 1.0f, 0, thickness);
            break;
        }
        }
    }

    bool IconButton(const char* id, ConsoleIcon icon, const char* label, bool selected = false, const ImVec4* iconColor = nullptr, const char* tooltip = nullptr)
    {
        const ImGuiStyle& style = ImGui::GetStyle();

        const float height = ImGui::GetFrameHeight();
        const float iconArea = height - style.FramePadding.y * 2.0f;

        const bool hasLabel = label && label[0] != '\0';
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

        const ImVec2 iconCenter(
            position.x + style.FramePadding.x + iconArea * 0.5f,
            position.y + height * 0.5f
        );

        const ImU32 actualIconColor = iconColor ? ImGui::GetColorU32(*iconColor) : ImGui::GetColorU32(ImGuiCol_Text);

        DrawIcon(drawList, icon, iconCenter, iconArea * 0.32f, actualIconColor);

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

        const ImVec2 center(iconPosition.x + frameHeight * 0.5f, iconPosition.y + frameHeight * 0.5f);

        DrawIcon(ImGui::GetWindowDrawList(), ConsoleIcon::Search, center, frameHeight * 0.23f, ImGui::GetColorU32(ImGuiCol_TextDisabled));

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

    if (IconButton("##ConsoleClear", ConsoleIcon::Trash, "Clear", false, nullptr, "Clear console"))
    {
        memorySink->Clear();
        entries.clear();

        selectedEntry.reset();
        selectedRepeatCount = 1;
    }

    ImGui::SameLine();

    if (IconButton("##ConsoleCollapse", ConsoleIcon::Collapse, "Collapse", collapse, nullptr, "Collapse identical messages"))
    {
        collapse = !collapse;
    }

    ImGui::SameLine();

    if (IconButton("##ConsoleAutoScroll", ConsoleIcon::AutoScroll, "Auto Scroll", autoScroll, nullptr, "Automatically scroll to new messages"))
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

    if (IconButton("##ConsoleLogs", ConsoleIcon::Info, logCountText.c_str(), showLogs, &logColor, "Show / hide logs"))
    {
        showLogs = !showLogs;
    }

    ImGui::SameLine();

    if (IconButton("##ConsoleWarnings", ConsoleIcon::Warning, warningCountText.c_str(), showWarnings, &warningColor, "Show / hide warnings"))
    {
        showWarnings = !showWarnings;
    }

    ImGui::SameLine();

    if (IconButton("##ConsoleErrors", ConsoleIcon::Error, errorCountText.c_str(), showErrors, &errorColor, "Show / hide errors"))
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

        ConsoleIcon severityIcon = ConsoleIcon::Info;

        if (severity == ConsoleSeverity::Warning)
        {
            severityIcon = ConsoleIcon::Warning;
        }
        else if (severity == ConsoleSeverity::Error)
        {
            severityIcon = ConsoleIcon::Error;
        }

        const float centerY = (rowMin.y + rowMax.y) * 0.5f;

        DrawIcon(drawList, severityIcon, ImVec2(rowMin.x + 13.0f, centerY), 6.5f, ImGui::GetColorU32(severityColor));

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

        const ImVec2 messagePosition(rowMin.x + 28.0f, rowMin.y + style.FramePadding.y);

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

        ConsoleIcon severityIcon = ConsoleIcon::Info;

        if (severity == ConsoleSeverity::Warning)
        {
            severityIcon = ConsoleIcon::Warning;
        }
        else if (severity == ConsoleSeverity::Error)
        {
            severityIcon = ConsoleIcon::Error;
        }

        const ImVec2 iconPosition = ImGui::GetCursorScreenPos();
        const float headerHeight = ImGui::GetFrameHeight();

        ImGui::Dummy(ImVec2(headerHeight, headerHeight));

        DrawIcon(
            ImGui::GetWindowDrawList(),
            severityIcon,
            ImVec2(iconPosition.x + headerHeight * 0.5f, iconPosition.y + headerHeight * 0.5f),
            headerHeight * 0.25f,
            ImGui::GetColorU32(severityColor)
        );

        ImGui::SameLine();

        ImGui::TextColored(severityColor, "%s", GetLevelName(selectedEntry->level));

        ImGui::SameLine();

        if (selectedRepeatCount > 1)
        {
            ImGui::TextDisabled("(%d occurrences)", selectedRepeatCount);
        }

        ImGui::SameLine();

        if (IconButton("##ConsoleCopy", ConsoleIcon::Copy, "Copy", false, nullptr, "Copy message to clipboard"))
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
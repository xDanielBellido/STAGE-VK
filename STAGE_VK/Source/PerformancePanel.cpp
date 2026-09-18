#include "PerformancePanel.h"

#include "EditorIcons.h"
#include "Profiler.h"

#include "imgui.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <limits>
#include <vector>

namespace
{
    struct MetricStats
    {
        float average = 0.0f;
        float minimum = 0.0f;
        float maximum = 0.0f;
    };

    template<typename Getter>
    MetricStats CalculateStats(const std::deque<FrameProfileData>& history, Getter getter)
    {
        MetricStats stats{};

        if (history.empty())
        {
            return stats;
        }

        float total = 0.0f;
        float minimum = std::numeric_limits<float>::max();
        float maximum = std::numeric_limits<float>::lowest();

        for (const FrameProfileData& frame : history)
        {
            const float value = getter(frame);

            total += value;
            minimum = std::min(minimum, value);
            maximum = std::max(maximum, value);
        }

        stats.average = total / static_cast<float>(history.size());
        stats.minimum = minimum;
        stats.maximum = maximum;

        return stats;
    }

    float GetGraphX(std::size_t index, std::size_t count, float minX, float maxX)
    {
        if (count <= 1)
        {
            return minX;
        }

        const float t = static_cast<float>(index) / static_cast<float>(count - 1);

        return minX + (maxX - minX) * t;
    }

    float GetGraphY(float value, float maximumValue, float minY, float maxY)
    {
        if (maximumValue <= 0.0f)
        {
            return maxY;
        }

        const float t = std::clamp(value / maximumValue, 0.0f, 1.0f);

        return maxY - (maxY - minY) * t;
    }

    void DrawCpuEventNode(const std::vector<CpuProfileEvent>& events, std::size_t& index, float rootDuration)
    {
        const std::size_t currentIndex = index;
        const CpuProfileEvent& event = events[currentIndex];

        std::size_t subtreeEnd = currentIndex + 1;

        while (subtreeEnd < events.size() && events[subtreeEnd].depth > event.depth)
        {
            ++subtreeEnd;
        }

        const bool hasChildren = currentIndex + 1 < subtreeEnd;

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        ImGui::PushID(static_cast<int>(currentIndex));

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;

        if (!hasChildren)
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
            flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        if (event.depth <= 1)
        {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        const bool open = ImGui::TreeNodeEx("##CpuEvent", flags, "%s", event.name.c_str());

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.3f ms", event.durationMs);

        ImGui::TableSetColumnIndex(2);

        const float percentage = rootDuration > 0.0f ? (event.durationMs / rootDuration) * 100.0f : 0.0f;

        ImGui::Text("%.1f%%", percentage);

        if (hasChildren && open)
        {
            index = currentIndex + 1;

            while (index < subtreeEnd)
            {
                DrawCpuEventNode(events, index, rootDuration);
            }

            ImGui::TreePop();
        }
        else
        {
            index = subtreeEnd;
        }

        ImGui::PopID();
    }

    void DrawStatsRow(const char* name, float current, const MetricStats& stats)
    {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(name);

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.3f", current);

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%.3f", stats.average);

        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%.3f", stats.minimum);

        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%.3f", stats.maximum);
    }
}

void PerformancePanel::Draw(Profiler& profiler, bool* open)
{
    if (!ImGui::Begin("Performance", open))
    {
        ImGui::End();
        return;
    }

    DrawToolbar(profiler);

    ImGui::Separator();

    const FrameProfileData& frame = selectedFrame ? *selectedFrame : profiler.GetLatestFrame();

    DrawSummary(frame);

    ImGui::Spacing();
    DrawFrameHistory(profiler);

    ImGui::Spacing();
    ImGui::Separator();

    const FrameProfileData& inspectedFrame = selectedFrame ? *selectedFrame : profiler.GetLatestFrame();

    if (ImGui::BeginTabBar("PerformanceTabs"))
    {
        if (ImGui::BeginTabItem("Overview"))
        {
            DrawOverview(profiler, inspectedFrame);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("CPU"))
        {
            DrawCpuHierarchy(inspectedFrame);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("GPU"))
        {
            DrawGpuProfiler(profiler, inspectedFrame);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Rendering"))
        {
            DrawRendering(inspectedFrame);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void PerformancePanel::DrawToolbar(Profiler& profiler)
{
    ImGui::BeginDisabled(profiler.IsRecording());

    if (ImGui::Button(ICON_FA_PLAY " Record"))
    {
        profiler.StartRecording();
        selectedFrame.reset();
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(!profiler.IsRecording());

    if (ImGui::Button(ICON_FA_PAUSE " Pause"))
    {
        profiler.StopRecording();
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_TRASH_CAN " Clear"))
    {
        profiler.Clear();
        selectedFrame.reset();
    }

    if (selectedFrame)
    {
        const auto& history = profiler.GetFrameHistory();

        std::size_t selectedIndex = history.size();

        for (std::size_t i = 0; i < history.size(); ++i)
        {
            if (history[i].frameNumber == selectedFrame->frameNumber)
            {
                selectedIndex = i;
                break;
            }
        }

        const bool canGoPrevious = selectedIndex < history.size() && selectedIndex > 0;
        const bool canGoNext = selectedIndex < history.size() && selectedIndex + 1 < history.size();

        ImGui::SameLine();

        if (ImGui::Button("Live"))
        {
            selectedFrame.reset();
        }

        ImGui::SameLine();

        ImGui::BeginDisabled(!canGoPrevious);

        if (ImGui::ArrowButton("##PreviousFrame", ImGuiDir_Left))
        {
            SelectAdjacentFrame(profiler, -1);
        }

        ImGui::EndDisabled();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("Previous frame");
        }

        ImGui::SameLine();

        ImGui::BeginDisabled(!canGoNext);

        if (ImGui::ArrowButton("##NextFrame", ImGuiDir_Right))
        {
            SelectAdjacentFrame(profiler, 1);
        }

        ImGui::EndDisabled();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("Next frame");
        }
    }

    ImGui::SameLine();

    if (profiler.IsRecording())
    {
        ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.45f, 1.0f), "Recording");
    }
    else
    {
        ImGui::TextDisabled("Paused");
    }

    if (selectedFrame)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("| Inspecting frame #%llu", static_cast<unsigned long long>(selectedFrame->frameNumber));
    }
}

void PerformancePanel::DrawSummary(const FrameProfileData& frame)
{
    if (!ImGui::BeginTable("PerformanceSummary", 4, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_BordersInnerV))
    {
        return;
    }

    ImGui::TableNextColumn();
    ImGui::TextDisabled("FPS");
    ImGui::Text("%.1f", frame.fps);

    ImGui::TableNextColumn();
    ImGui::TextDisabled("FRAME");
    ImGui::Text("%.2f ms", frame.frameTimeMs);

    ImGui::TableNextColumn();
    ImGui::TextDisabled("CPU");
    ImGui::Text("%.2f ms", frame.cpuFrameTimeMs);

    ImGui::TableNextColumn();
    ImGui::TextDisabled("GPU");
    ImGui::Text("%.2f ms", frame.gpuFrameTimeMs);

    ImGui::EndTable();
}

void PerformancePanel::DrawFrameHistory(Profiler& profiler)
{
    const auto& history = profiler.GetFrameHistory();

    ImGui::TextUnformatted("Frame History");

    if (history.empty())
    {
        ImGui::TextDisabled("Waiting for profiler data...");
        return;
    }

    const float graphWidth = std::max(200.0f, ImGui::GetContentRegionAvail().x);
    const float graphHeight = 180.0f;

    ImGui::InvisibleButton("##PerformanceFrameHistory", ImVec2(graphWidth, graphHeight));

    const ImVec2 graphMin = ImGui::GetItemRectMin();
    const ImVec2 graphMax = ImGui::GetItemRectMax();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(graphMin, graphMax, ImGui::GetColorU32(ImGuiCol_FrameBg));
    drawList->AddRect(graphMin, graphMax, ImGui::GetColorU32(ImGuiCol_Border));

    float maximumMs = 20.0f;

    for (const FrameProfileData& frame : history)
    {
        maximumMs = std::max(maximumMs, frame.frameTimeMs * 1.15f);
        maximumMs = std::max(maximumMs, frame.cpuFrameTimeMs * 1.15f);
        maximumMs = std::max(maximumMs, frame.gpuFrameTimeMs * 1.15f);
    }

    const ImU32 frameColor = ImGui::GetColorU32(ImVec4(0.90f, 0.90f, 0.90f, 1.0f));
    const ImU32 cpuColor = ImGui::GetColorU32(ImVec4(0.25f, 0.65f, 1.00f, 1.0f));
    const ImU32 gpuColor = ImGui::GetColorU32(ImVec4(0.70f, 0.40f, 1.00f, 1.0f));

    const ImU32 sixtyFpsColor = ImGui::GetColorU32(ImVec4(1.00f, 0.70f, 0.20f, 0.55f));
    const ImU32 oneTwentyFpsColor = ImGui::GetColorU32(ImVec4(0.30f, 0.85f, 0.45f, 0.45f));

    const float sixtyFpsY = GetGraphY(16.67f, maximumMs, graphMin.y, graphMax.y);
    const float oneTwentyFpsY = GetGraphY(8.33f, maximumMs, graphMin.y, graphMax.y);

    if (16.67f <= maximumMs)
    {
        drawList->AddLine(ImVec2(graphMin.x, sixtyFpsY), ImVec2(graphMax.x, sixtyFpsY), sixtyFpsColor);
        drawList->AddText(ImVec2(graphMin.x + 6.0f, sixtyFpsY - ImGui::GetTextLineHeight()), sixtyFpsColor, "60 FPS");
    }

    if (8.33f <= maximumMs)
    {
        drawList->AddLine(ImVec2(graphMin.x, oneTwentyFpsY), ImVec2(graphMax.x, oneTwentyFpsY), oneTwentyFpsColor);
        drawList->AddText(ImVec2(graphMin.x + 6.0f, oneTwentyFpsY - ImGui::GetTextLineHeight()), oneTwentyFpsColor, "120 FPS");
    }

    auto drawSeries = [&](auto getter, ImU32 color)
        {
            if (history.size() < 2)
            {
                return;
            }

            for (std::size_t i = 1; i < history.size(); ++i)
            {
                const float previousX = GetGraphX(i - 1, history.size(), graphMin.x, graphMax.x);
                const float currentX = GetGraphX(i, history.size(), graphMin.x, graphMax.x);

                const float previousY = GetGraphY(getter(history[i - 1]), maximumMs, graphMin.y, graphMax.y);
                const float currentY = GetGraphY(getter(history[i]), maximumMs, graphMin.y, graphMax.y);

                drawList->AddLine(ImVec2(previousX, previousY), ImVec2(currentX, currentY), color, 1.5f);
            }
        };

    drawSeries([](const FrameProfileData& frame) { return frame.frameTimeMs; }, frameColor);
    drawSeries([](const FrameProfileData& frame) { return frame.cpuFrameTimeMs; }, cpuColor);
    drawSeries([](const FrameProfileData& frame) { return frame.gpuFrameTimeMs; }, gpuColor);

    if (selectedFrame)
    {
        for (std::size_t i = 0; i < history.size(); ++i)
        {
            if (history[i].frameNumber != selectedFrame->frameNumber)
            {
                continue;
            }

            const float selectedX = GetGraphX(i, history.size(), graphMin.x, graphMax.x);

            drawList->AddLine(ImVec2(selectedX, graphMin.y), ImVec2(selectedX, graphMax.y), ImGui::GetColorU32(ImGuiCol_Text), 2.0f);
            break;
        }
    }

    if (ImGui::IsItemHovered())
    {
        const float mouseX = ImGui::GetIO().MousePos.x;
        const float normalizedX = std::clamp((mouseX - graphMin.x) / graphWidth, 0.0f, 1.0f);

        const std::size_t index = history.size() > 1
            ? static_cast<std::size_t>(normalizedX * static_cast<float>(history.size() - 1) + 0.5f)
            : 0;

        const FrameProfileData& hoveredFrame = history[index];
        const float hoverX = GetGraphX(index, history.size(), graphMin.x, graphMax.x);

        drawList->AddLine(ImVec2(hoverX, graphMin.y), ImVec2(hoverX, graphMax.y), ImGui::GetColorU32(ImGuiCol_TextDisabled));

        ImGui::BeginTooltip();

        ImGui::Text("Frame #%llu", static_cast<unsigned long long>(hoveredFrame.frameNumber));
        ImGui::Separator();

        ImGui::Text("Frame: %.3f ms", hoveredFrame.frameTimeMs);
        ImGui::Text("CPU:   %.3f ms", hoveredFrame.cpuFrameTimeMs);
        ImGui::Text("GPU:   %.3f ms", hoveredFrame.gpuFrameTimeMs);
        ImGui::Text("FPS:   %.1f", hoveredFrame.fps);

        ImGui::EndTooltip();

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            selectedFrame = hoveredFrame;
        }
    }

    ImGui::Spacing();

    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(frameColor), "Frame");
    ImGui::SameLine();

    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(cpuColor), "CPU");
    ImGui::SameLine();

    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(gpuColor), "GPU");

    ImGui::SameLine();
    ImGui::TextDisabled("Click a frame to inspect it");
}

void PerformancePanel::DrawOverview(const Profiler& profiler, const FrameProfileData& frame)
{
    const auto& history = profiler.GetFrameHistory();

    const MetricStats frameStats = CalculateStats(history, [](const FrameProfileData& sample) { return sample.frameTimeMs; });
    const MetricStats cpuStats = CalculateStats(history, [](const FrameProfileData& sample) { return sample.cpuFrameTimeMs; });
    const MetricStats gpuStats = CalculateStats(history, [](const FrameProfileData& sample) { return sample.gpuFrameTimeMs; });

    ImGui::Text("Frame #%llu", static_cast<unsigned long long>(frame.frameNumber));

    ImGui::Spacing();

    if (ImGui::BeginTable("PerformanceOverviewStats", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Metric");
        ImGui::TableSetupColumn("Current");
        ImGui::TableSetupColumn("Average");
        ImGui::TableSetupColumn("Minimum");
        ImGui::TableSetupColumn("Maximum");

        ImGui::TableHeadersRow();

        DrawStatsRow("Frame Time (ms)", frame.frameTimeMs, frameStats);
        DrawStatsRow("CPU Time (ms)", frame.cpuFrameTimeMs, cpuStats);
        DrawStatsRow("GPU Time (ms)", frame.gpuFrameTimeMs, gpuStats);

        ImGui::EndTable();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextDisabled("FRAME BUDGET");

        const float sixtyFpsBudget = 1000.0f / 60.0f;
        const float oneTwentyFpsBudget = 1000.0f / 120.0f;

        const bool within60FpsBudget = frame.frameTimeMs <= sixtyFpsBudget;
        const bool within120FpsBudget = frame.frameTimeMs <= oneTwentyFpsBudget;

        const ImVec4 successColor(0.35f, 0.85f, 0.45f, 1.0f);
        const ImVec4 warningColor(1.00f, 0.55f, 0.20f, 1.0f);

        if (ImGui::BeginTable("FrameBudget", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Target");
            ImGui::TableSetupColumn("Budget");
            ImGui::TableSetupColumn("Status");

            ImGui::TableHeadersRow();

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted("60 FPS");

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f ms", sixtyFpsBudget);

            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(within60FpsBudget ? successColor : warningColor, "%s", within60FpsBudget ? "Within Budget" : "Over Budget");

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted("120 FPS");

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f ms", oneTwentyFpsBudget);

            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(within120FpsBudget ? successColor : warningColor, "%s", within120FpsBudget ? "Within Budget" : "Over Budget");

            ImGui::EndTable();
        }
    }
}

void PerformancePanel::DrawCpuHierarchy(const FrameProfileData& frame)
{
    ImGui::Text("Frame #%llu", static_cast<unsigned long long>(frame.frameNumber));
    ImGui::Spacing();

    if (frame.cpuEvents.empty())
    {
        ImGui::TextDisabled("No CPU profiling events available.");
        return;
    }

    const float rootDuration = frame.cpuEvents.front().durationMs > 0.0f
        ? frame.cpuEvents.front().durationMs
        : frame.cpuFrameTimeMs;

    const ImGuiTableFlags flags =
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY;

    if (!ImGui::BeginTable("CpuHierarchy", 3, flags, ImVec2(0.0f, 0.0f)))
    {
        return;
    }

    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 110.0f);
    ImGui::TableSetupColumn("% Frame", ImGuiTableColumnFlags_WidthFixed, 90.0f);

    ImGui::TableHeadersRow();

    std::size_t index = 0;

    while (index < frame.cpuEvents.size())
    {
        DrawCpuEventNode(frame.cpuEvents, index, rootDuration);
    }

    ImGui::EndTable();
}

void PerformancePanel::DrawGpuProfiler(const Profiler& profiler, const FrameProfileData& frame)
{
    ImGui::Text("Frame #%llu", static_cast<unsigned long long>(frame.frameNumber));
    const GpuDeviceProfileData& gpu = profiler.GetGpuDeviceInfo();

    ImGui::Spacing();

    ImGui::TextDisabled("DEVICE");

    if (ImGui::BeginTable("GpuDeviceInfo", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("GPU");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(gpu.name.c_str());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Type");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(gpu.type.c_str());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Vulkan API");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u.%u.%u", gpu.apiMajor, gpu.apiMinor, gpu.apiPatch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Timestamp Queries");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(gpu.timestampSupported ? "Supported" : "Unsupported");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Timestamp Period");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.3f ns", gpu.timestampPeriodNs);

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Spacing();

    ImGui::TextDisabled("GPU FRAME TIME");
    ImGui::Text("%.3f ms", frame.gpuFrameTimeMs);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const MetricStats gpuStats = CalculateStats(profiler.GetFrameHistory(), [](const FrameProfileData& sample) { return sample.gpuFrameTimeMs; });

    if (ImGui::BeginTable("GpuStats", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Average");

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.3f ms", gpuStats.average);

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Minimum");

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.3f ms", gpuStats.minimum);

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Maximum");

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.3f ms", gpuStats.maximum);

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Measured using Vulkan timestamp queries.");
}

void PerformancePanel::DrawRendering(const FrameProfileData& frame)
{
    const RenderingProfileData& rendering = frame.rendering;

    ImGui::Text("Frame #%llu", static_cast<unsigned long long>(frame.frameNumber));

    ImGui::Spacing();

    ImGui::TextDisabled("RENDERING CONTEXT");

    if (ImGui::BeginTable("RenderingContext", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Backend");

        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted("Vulkan");

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Resolution");

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u x %u", rendering.width, rendering.height);

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Swapchain Images");

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u", rendering.swapchainImageCount);

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Current Image");

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u", rendering.currentImageIndex);

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextDisabled("FRAME COMMANDS");

    if (ImGui::BeginTable("RenderingCommands", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Command Buffers");

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u", rendering.commandBufferCount);

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Render Passes");

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u", rendering.renderPassCount);

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Queue Submissions");

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u", rendering.queueSubmissionCount);

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Present Calls");

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u", rendering.presentCount);

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextDisabled("SCENE STATISTICS");
    ImGui::TextDisabled("Draw calls, vertices and triangles are not tracked yet.");
}

void PerformancePanel::SelectAdjacentFrame(const Profiler& profiler, int direction)
{
    if (!selectedFrame || direction == 0)
    {
        return;
    }

    const auto& history = profiler.GetFrameHistory();

    for (std::size_t i = 0; i < history.size(); ++i)
    {
        if (history[i].frameNumber != selectedFrame->frameNumber)
        {
            continue;
        }

        const int nextIndex = static_cast<int>(i) + direction;

        if (nextIndex >= 0 && nextIndex < static_cast<int>(history.size()))
        {
            selectedFrame = history[static_cast<std::size_t>(nextIndex)];
        }

        return;
    }
}

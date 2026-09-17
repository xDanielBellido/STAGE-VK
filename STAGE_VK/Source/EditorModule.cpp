#include "Globals.h"
#include "Application.h"
#include "EditorModule.h"
#include "RenderModule.h"

#include "imgui.h"

EditorModule::EditorModule(RenderModule& renderModule) : renderModule(renderModule)
{
}

void EditorModule::Render()
{
    if (!renderModule.IsFrameActive())
    {
        return;
    }

    DrawMainMenu();

    ImGuiIO& io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        //ImGui::DockSpaceOverViewport();
    }

    // Draw panels and windows here:

    if (showConsole)
    {
        consolePanel.Draw(&showConsole);
    }

    if (showPerformance)
    {
        performancePanel.Draw(app->GetProfiler(), &showPerformance);
    }
}

void EditorModule::DrawMainMenu()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }
 
    if (ImGui::BeginMenu("Window"))
    {
        // Add menu options here:

        ImGui::MenuItem("Console", nullptr, &showConsole);
        ImGui::MenuItem("Performance", nullptr, &showPerformance);

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}
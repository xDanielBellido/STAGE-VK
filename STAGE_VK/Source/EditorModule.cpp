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

    if (showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow);
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
        ImGui::MenuItem("Dear ImGui Demo", nullptr, &showDemoWindow);

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}
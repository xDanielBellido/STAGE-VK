#pragma once

#include "Module.h"
#include "ConsolePanel.h"
#include "PerformancePanel.h"

class RenderModule;

class EditorModule final : public Module
{
public:
    explicit EditorModule(RenderModule& renderModule);

    void Render() override;

private:
    void DrawMainMenu();

private:
    RenderModule& renderModule;

    ConsolePanel consolePanel;
    PerformancePanel performancePanel;

    bool showConsole = true;
    bool showDemoWindow = true;
    bool showPerformance = true;
};

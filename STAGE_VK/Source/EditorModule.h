#pragma once

#include "Module.h"

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

    bool showDemoWindow = true;
};


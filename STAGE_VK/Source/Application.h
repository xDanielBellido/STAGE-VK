/*
    ==========================================================================
    Application
    ==========================================================================

    This class is the high-level core of the engine.

    Its main responsibility is to coordinate the general lifetime of the
    application:
    - store the startup configuration
    - create and destroy the main window
    - register and manage engine modules
    - execute the main per-frame update flow
    - provide basic frame timing information

    Application acts as the main orchestrator of the engine.

    Application also exposes access to important shared systems such as:
    - the main window
    - the Engine modules

    This allows other parts of the engine to query central systems when needed.
*/
#pragma once

#include "AppConfig.h"
#include "Profiler.h"

#include <memory>
#include <vector>
#include <chrono>

class Module;
class Window;
class VulkanModule;
class RenderModule;
class EditorModule;

class Application
{
private:
    AppConfig config;
    Profiler profiler;

    std::unique_ptr<Window> window;
    std::vector<Module*> modules;

    VulkanModule* vulkanModule = nullptr;
    RenderModule* renderModule = nullptr;
    EditorModule* editorModule = nullptr;

    bool paused = false;

    float deltaTime = 0.0f;
    float fps = 0.0f;

    using Clock = std::chrono::steady_clock;
    Clock::time_point lastFrameTime;

public:
    explicit Application(const AppConfig& config);
    ~Application();

                            // Application Lifetime
    bool                    Init();
    void                    Update();
    bool                    CleanUp();

                            // Core Systems Access
    Window&                 GetWindow() { return *window; }
    const Window&           GetWindow() const { return *window; }
                            
                            // Module Getters
    VulkanModule&           GetVulkanModule() { return *vulkanModule; }
    const VulkanModule&     GetVulkanModule() const { return *vulkanModule; }
    RenderModule&           GetRenderModule() { return *renderModule; }
    const RenderModule&     GetRenderModule() const { return *renderModule; }
    EditorModule&           GetEditorModule() { return *editorModule; }
    const EditorModule&     GetEditorModule() const { return *editorModule; }

                            // Profiler
    Profiler&               GetProfiler() { return profiler; }
    const Profiler&         GetProfiler() const { return profiler; }

                            // Frame Info
    float                   GetDeltaTime() const { return deltaTime; }
    float                   GetFPS() const { return fps; }
    bool                    IsPaused() const { return paused; }
    bool                    ShouldClose() const;


};
#include "Globals.h"
#include "Application.h"
#include "ProfileScope.h"

#include "Module.h"
#include "Window.h"
#include "VulkanModule.h"
#include "RenderModule.h"
#include "EditorModule.h"

#include <stdexcept>

Application::Application(const AppConfig& config) : config(config)
{
    app = this;

    modules.push_back(vulkanModule = new VulkanModule());
    modules.push_back(renderModule = new RenderModule(*vulkanModule));
    modules.push_back(editorModule = new EditorModule(*renderModule));
}

Application::~Application()
{
    CleanUp();

    for (auto it = modules.rbegin(); it != modules.rend(); ++it)
    {
        delete* it;
    }

    modules.clear();
    app = nullptr;
}

bool Application::Init()
{
    window = std::make_unique<Window>(
        config.title,
        static_cast<int>(config.width),
        static_cast<int>(config.height),
        config.resizable
    );

    bool ret = true;

    for (auto it = modules.begin(); it != modules.end() && ret; ++it)
    {
        ret = (*it)->Init();
    }

    lastFrameTime = Clock::now();
    return ret;
}

void Application::Update()
{
    framePacer.BeginFrame();

    const auto now = Clock::now();
    deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
    lastFrameTime = now;

    if (deltaTime > 0.0f)
    {
        fps = 1.0f / deltaTime;
    }

    profiler.FinalizePreviousFrameTiming(deltaTime, fps);
    profiler.BeginFrame();

    {
        PROFILE_SCOPE(profiler, "Frame");

        {
            PROFILE_SCOPE(profiler, "PollEvents");
            window->PollEvents();
        }

        {
            PROFILE_SCOPE(profiler, "Window");

            int framebufferWidth = 0;
            int framebufferHeight = 0;
            window->GetFramebufferSize(framebufferWidth, framebufferHeight);

            paused = (framebufferWidth == 0 || framebufferHeight == 0);

            if (window->WasResized() && !paused)
            {
                for (auto& module : modules)
                {
                    module->OnResize(framebufferWidth, framebufferHeight);
                }

                window->ResetResizedFlag();
            }
        }

        if (!paused)
        {
            {
                PROFILE_SCOPE(profiler, "Update");

                for (auto& module : modules)
                {
                    module->Update();
                }
            }

            {
                PROFILE_SCOPE(profiler, "PreRender");

                for (auto& module : modules)
                {
                    module->PreRender();
                }
            }

            {
                PROFILE_SCOPE(profiler, "Render");

                for (auto& module : modules)
                {
                    module->Render();
                }
            }

            {
                PROFILE_SCOPE(profiler, "PostRender");

                for (auto it = modules.rbegin(); it != modules.rend(); ++it)
                {
                    (*it)->PostRender();
                }
            }
        }
    }

    profiler.EndFrame();

    if (!paused)
    {
        framePacer.WaitForRefreshRate(window->GetCurrentMonitorRefreshRate());
    }
}

bool Application::CleanUp()
{
    bool ret = true;

    for (auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
    {
        ret = (*it)->CleanUp();
    }

    editorModule = nullptr;
    renderModule = nullptr;
    vulkanModule = nullptr;
    window.reset();

    return ret;
}

bool Application::ShouldClose() const
{
    return !window || window->ShouldClose();
}
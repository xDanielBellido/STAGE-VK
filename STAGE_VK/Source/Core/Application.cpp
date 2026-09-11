#include "Globals.h"
#include "Application.h"

#include "Module.h"
#include "../Platform/Window.h"
#include "../Renderer/BackendVKModule.h"

#include <stdexcept>

Application::Application(const AppConfig& config) : config(config)
{
    app = this;

    modules.push_back(backendVK = new BackendVKModule());
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
    window->PollEvents();

    const auto now = Clock::now();
    deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
    lastFrameTime = now;

    if (deltaTime > 0.0f)
    {
        fps = 1.0f / deltaTime;
    }

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

    if (paused)
    {
        return;
    }

    for (auto& module : modules)
    {
        module->Update();
    }

    for (auto& module : modules)
    {
        module->PreRender();
    }

    for (auto& module : modules)
    {
        module->Render();
    }

    for (auto& module : modules)
    {
        module->PostRender();
    }
}

bool Application::CleanUp()
{
    bool ret = true;

    for (auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
    {
        ret = (*it)->CleanUp();
    }

    backendVK = nullptr;
    window.reset();

    return ret;
}

bool Application::ShouldClose() const
{
    return !window || window->ShouldClose();
}
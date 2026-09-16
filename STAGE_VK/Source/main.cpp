#include <cstdlib>
#include <iostream>

#include "AppConfig.h"
#include "Application.h"
#include "Log.h"

int main()
{
    try
    {
        Log::Init();

        LOG_INFO("STAGE VK logging initialized.");

        AppConfig config{};
        config.title = "STAGE_VK";
        config.width = 1280;
        config.height = 720;
        config.resizable = true;

        Application app(config);

        if (!app.Init())
        {
            throw std::runtime_error("Application initialization failed.");
        }

        while (!app.ShouldClose())
        {
            app.Update();
        }
    }
    catch (const std::exception& e)
    {
        if (Log::IsInitialized())
        {
            LOG_CRITICAL("Fatal error: {}", e.what());
        }
        else
        {
            std::cerr << "[Fatal Error] " << e.what() << '\n';
        }

        Log::Shutdown();
        return EXIT_FAILURE;
    }

    Log::Shutdown();

    return EXIT_SUCCESS;
}
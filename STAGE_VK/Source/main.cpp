#include <iostream>
#include "AppConfig.h"
#include "Application.h"

int main()
{
    try
    {
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
        std::cerr << "[Fatal Error] " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
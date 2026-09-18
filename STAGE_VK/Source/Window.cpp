#include "Window.h"

#include <stdexcept>
#include <algorithm>

Window::Window(const std::string& title, int width, int height, bool resizable)
{
    if (glfwInit() != GLFW_TRUE)
    {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window.");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
}

Window::~Window()
{
    if (window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
}

void Window::PollEvents()
{
    glfwPollEvents();
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(window);
}

void Window::GetFramebufferSize(int& width, int& height) const
{
    glfwGetFramebufferSize(window, &width, &height);
}

int Window::GetCurrentMonitorRefreshRate() const
{
    GLFWmonitor* currentMonitor = glfwGetWindowMonitor(window);

    if (!currentMonitor)
    {
        int windowX = 0;
        int windowY = 0;
        int windowWidth = 0;
        int windowHeight = 0;

        glfwGetWindowPos(window, &windowX, &windowY);
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        int monitorCount = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

        int bestOverlap = 0;

        for (int i = 0; i < monitorCount; ++i)
        {
            int monitorX = 0;
            int monitorY = 0;
            int monitorWidth = 0;
            int monitorHeight = 0;

            glfwGetMonitorWorkarea(monitors[i], &monitorX, &monitorY, &monitorWidth, &monitorHeight);

            const int overlapWidth = std::max(0, std::min(windowX + windowWidth, monitorX + monitorWidth) - std::max(windowX, monitorX));
            const int overlapHeight = std::max(0, std::min(windowY + windowHeight, monitorY + monitorHeight) - std::max(windowY, monitorY));
            const int overlapArea = overlapWidth * overlapHeight;

            if (overlapArea > bestOverlap)
            {
                bestOverlap = overlapArea;
                currentMonitor = monitors[i];
            }
        }
    }

    if (!currentMonitor)
    {
        currentMonitor = glfwGetPrimaryMonitor();
    }

    const GLFWvidmode* videoMode = glfwGetVideoMode(currentMonitor);

    return videoMode ? videoMode->refreshRate : 0;
}

void Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto* self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self)
    {
        self->framebufferResized = true;
    }
}
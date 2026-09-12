#include "Window.h"

#include <stdexcept>

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

void Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto* self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self)
    {
        self->framebufferResized = true;
    }
}
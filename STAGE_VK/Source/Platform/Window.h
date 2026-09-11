/*
    ===========================================================================
    Window
    ===========================================================================

    This class represents the main operating system window used by the engine.

    Its responsibility is to encapsulate the GLFW window object and provide a
    simple interface for the rest of the engine to interact with the platform
    window without dealing directly with GLFW-specific details everywhere.

    This class is responsible for:
    - creating and destroying the main window
    - polling operating system events
    - reporting whether the user requested to close the window
    - exposing the native GLFW handle when low-level systems need it
    - providing the current framebuffer size
    - detecting when the window has been resized

    Its job is to represent and manage the platform window that rendering
    systems will use.
*/

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

class Window
{
public:
    Window(const std::string& title, int width, int height, bool resizable);
    ~Window();

    // Window Update
    void PollEvents();
    bool ShouldClose() const;

    // Native Handle Access
    GLFWwindow* GetNativeHandle() const { return window; }

    // Framebuffer Info
    void GetFramebufferSize(int& width, int& height) const;

    // Resize State
    bool WasResized() const { return framebufferResized; }
    void ResetResizedFlag() { framebufferResized = false; }

private:
    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
    GLFWwindow* window = nullptr;
    bool framebufferResized = false;
};
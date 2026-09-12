#pragma once

class Application;

class Module
{
public:
    virtual ~Module() = default;

    virtual bool Init() { return true; }
    virtual void Update() {}
    virtual void PreRender() {}
    virtual void Render() {}
    virtual void PostRender() {}
    virtual void OnResize(int width, int height) {}
    virtual bool CleanUp() { return true; }
};

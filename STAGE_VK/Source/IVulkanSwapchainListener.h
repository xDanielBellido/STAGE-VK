#pragma once

class IVulkanSwapchainListener
{
public:
    virtual ~IVulkanSwapchainListener() = default;

    virtual void BeforeSwapchainRecreate() = 0;
    virtual void AfterSwapchainRecreate() = 0;
};
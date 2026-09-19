/*
    ===========================================================================
    VulkanSwapchain
    ===========================================================================

    This class manages the Vulkan swapchain used to present rendered images
    to the application window.

    The swapchain owns a collection of images that are presented to the screen.
    During creation, the class queries the capabilities of the window surface
    and chooses an appropriate image format, presentation mode and extent.

    This class is responsible for:
    - querying swapchain support and surface capabilities
    - selecting the surface format
    - selecting the presentation mode
    - selecting the swapchain extent
    - selecting a supported composite alpha mode
    - validating the required swapchain image usage
    - creating and destroying VkSwapchainKHR

    VkPhysicalDevice, VkDevice and VkSurfaceKHR are provided by the Vulkan
    module/device and are not owned by this class.
*/

#pragma once

#include "VulkanDevice.h"

#include <vulkan/vulkan.h>

#include <vector>

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanSwapchain
{
public:
    bool Init(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, const QueueFamilyIndices& queueFamilies, uint32_t width, uint32_t height);
    void CleanUp();

    VkSwapchainKHR GetSwapchain() const { return swapchain; }
    VkFormat GetImageFormat() const { return imageFormat; }
    VkImage GetImage(uint32_t index) const { return images[index]; }
    uint32_t GetImageCount() const { return static_cast<uint32_t>(images.size()); }
    VkExtent2D GetExtent() const { return extent; }

    const std::vector<VkImage>& GetImages() const { return images; }
    const std::vector<VkImageView>& GetImageViews() const { return imageViews; }

private:
    SwapchainSupportDetails QuerySupport(VkPhysicalDevice physicalDevice) const;

    VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
    VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
    VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) const;
    VkCompositeAlphaFlagBitsKHR ChooseCompositeAlpha(const VkSurfaceCapabilitiesKHR& capabilities) const;

    void GetSwapchainImages();
    void CreateImageViews();

private:
    // Non-owned handles
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;

    VkFormat imageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
};
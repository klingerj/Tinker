#pragma once

#include "../../Include/Core/CoreDefines.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <windows.h>

typedef struct vulkan_context_res
{
    VkInstance instance                      = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice          = VK_NULL_HANDLE;
    uint32 graphicsQueueIndex                = 0xffffffff;
    uint32 presentationQueueIndex            = 0xffffffff;
    VkDevice device                          = VK_NULL_HANDLE;
    VkQueue graphicsQueue                    = VK_NULL_HANDLE;
    VkQueue presentationQueue                = VK_NULL_HANDLE;
    VkSurfaceKHR surface                     = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain                 = VK_NULL_HANDLE;
    VkExtent2D swapChainExtent               = { 0, 0 };
    VkFormat swapChainFormat                 = VK_FORMAT_UNDEFINED;
    VkImage* swapChainImages                 = nullptr;
    VkImageView* swapChainImageViews         = nullptr;
    uint32 numSwapChainImages                = 0;
} VulkanContextResources;

int InitVulkan(VulkanContextResources* vulkanContextResources, HINSTANCE hInstance, HWND windowHandle, uint32 width, uint32 height);
void DestroyVulkan(VulkanContextResources* vulkanContextResources);

#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <windows.h>

typedef struct vulkan_context_res
{
    VkSurfaceKHR surface;
    VkInstance instance;
} VulkanContextResources;

int InitVulkan(VulkanContextResources* vulkanContextResources, HINSTANCE hInstance, HWND windowHandle);
void DestroyVulkan(VulkanContextResources* vulkanContextResources);

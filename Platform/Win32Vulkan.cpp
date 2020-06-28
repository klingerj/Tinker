#include "../Include/Platform/Win32Vulkan.h"
#include "../Include/Core/CoreDefines.h"

int InitVulkan(VulkanContextResources* vulkanContextResources, HINSTANCE hInstance, HWND windowHandle)
{
    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "Tinker with Vulkan";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "Tinker Engine";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    uint32 numExtensions = 2;
    const char* extensionNames[2] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
    // TODO: get the extensions
    instanceCreateInfo.enabledExtensionCount = numExtensions;
    instanceCreateInfo.ppEnabledExtensionNames = extensionNames;
    instanceCreateInfo.enabledLayerCount = 0;

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &vulkanContextResources->instance);
    if (result != VK_SUCCESS)
    {
        // TODO: Log? Fail?
        return 1;
    }

    // TODO: create the other devices and such

    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {};
    win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32SurfaceCreateInfo.hinstance = hInstance;
    win32SurfaceCreateInfo.hwnd = windowHandle;

    result = vkCreateWin32SurfaceKHR(vulkanContextResources->instance, &win32SurfaceCreateInfo, NULL, &vulkanContextResources->surface);
    if (result != VK_SUCCESS)
    {
        // TODO: Log? Fail?
        return 1;
    }

    return 0;
}

void DestroyVulkan(VulkanContextResources* vulkanContextResources)
{
    vkDestroySurfaceKHR(vulkanContextResources->instance, vulkanContextResources->surface, nullptr);
    vkDestroyInstance(vulkanContextResources->instance, nullptr);
}

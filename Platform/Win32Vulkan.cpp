#include "../Include/Platform/Win32Vulkan.h"
#include "../Include/Core/CoreDefines.h"

#include <cstring>

#ifdef _DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFunc(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    OutputDebugString("VALIDATION LAYER:\n");
    OutputDebugString(pCallbackData->pMessage);
    return VK_FALSE;
}
#endif

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

    const uint32 numRequiredExtensions = 
    #ifdef _DEBUG
    3
    #else
    2
    #endif
    ;
    const char* requiredExtensionNames[numRequiredExtensions] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME

        #ifdef _DEBUG
        , VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        #endif
    };
    
    instanceCreateInfo.enabledExtensionCount = numRequiredExtensions;
    instanceCreateInfo.ppEnabledExtensionNames = requiredExtensionNames;

    instanceCreateInfo.enabledLayerCount = 0;
    instanceCreateInfo.ppEnabledLayerNames = nullptr;
    
    // Get the available extensions
    uint32 numAvailableExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions, nullptr);
    
    VkExtensionProperties* availableExtensions = new VkExtensionProperties[numAvailableExtensions];
    vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions, availableExtensions);
    // TODO: log the available extensions?
    delete availableExtensions;

    // Validation layers
    #ifdef _DEBUG
    const uint32 numRequiredLayers = 1;
    const char* requestedLayersStr[numRequiredLayers] = { "VK_LAYER_KHRONOS_validation" };

    uint32 numAvailableLayers = 0;
    vkEnumerateInstanceLayerProperties(&numAvailableLayers, nullptr);

    if (numAvailableLayers == 0)
    {
        // TODO: Log? Fail?
    }

    VkLayerProperties* availableLayers = new VkLayerProperties[numAvailableLayers];
    vkEnumerateInstanceLayerProperties(&numAvailableLayers, availableLayers);
    // TODO: log the available layers?

    bool layersSupported[numRequiredLayers] = { false };
    for (uint32 uiReqLayer = 0; uiReqLayer < numRequiredLayers; ++uiReqLayer)
    {
        for (uint32 uiAvailLayer = 0; uiAvailLayer < numAvailableLayers; ++uiAvailLayer)
        {
            if (!strcmp(availableLayers[uiAvailLayer].layerName, requestedLayersStr[uiReqLayer]))
            {
                layersSupported[uiReqLayer] = true;
                break;
            }
        }
    }

    delete availableLayers;

    for (uint32 uiReqLayer = 0; uiReqLayer < numRequiredLayers; ++uiReqLayer)
    {
        if (!layersSupported[uiReqLayer])
        {
            // TODO: Log? Fail?
        }
    }

    instanceCreateInfo.enabledLayerCount = numRequiredLayers;
    instanceCreateInfo.ppEnabledLayerNames = requestedLayersStr;
    #endif

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &vulkanContextResources->instance);
    if (result != VK_SUCCESS)
    {
        // TODO: Log? Fail?
        return 1;
    }

    #ifdef _DEBUG
    // Debug utils callback
    VkDebugUtilsMessengerCreateInfoEXT dbgUtilsMsgCreateInfo = {};
    dbgUtilsMsgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbgUtilsMsgCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbgUtilsMsgCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbgUtilsMsgCreateInfo.pfnUserCallback = DebugCallbackFunc;
    dbgUtilsMsgCreateInfo.pUserData = nullptr; // Optional

    PFN_vkCreateDebugUtilsMessengerEXT dbgCreateFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanContextResources->instance, "vkCreateDebugUtilsMessengerEXT");
    if (dbgCreateFunc)
    {
        dbgCreateFunc(vulkanContextResources->instance, &dbgUtilsMsgCreateInfo, nullptr, &vulkanContextResources->debugMessenger);
    }
    else
    {
        // TODO: Log? Fail?
    }
    #endif

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
    #ifdef _DEBUG
    // Debug utils messenger
    PFN_vkDestroyDebugUtilsMessengerEXT dbgDestroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vulkanContextResources->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (dbgDestroyFunc)
    {
        dbgDestroyFunc(vulkanContextResources->instance, vulkanContextResources->debugMessenger, nullptr);
    }
    else
    {
        // TODO: Log? Fail?
    }
    #endif

    vkDestroySurfaceKHR(vulkanContextResources->instance, vulkanContextResources->surface, nullptr);
    vkDestroyInstance(vulkanContextResources->instance, nullptr);
}

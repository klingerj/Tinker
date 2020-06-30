#include "../Include/Platform/Win32Vulkan.h"

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
    dbgUtilsMsgCreateInfo.pUserData = nullptr;

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

    // Surface
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

    // Physical device
    uint32 numPhysicalDevices = 0;
    vkEnumeratePhysicalDevices(vulkanContextResources->instance, &numPhysicalDevices, nullptr);

    if (numPhysicalDevices == 0)
    {
        // TODO: Log? Fail?
    }

    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[numPhysicalDevices];
    vkEnumeratePhysicalDevices(vulkanContextResources->instance, &numPhysicalDevices, physicalDevices);

    for (uint32 uiPhysicalDevice = 0; uiPhysicalDevice < numPhysicalDevices; ++uiPhysicalDevice)
    {
        VkPhysicalDevice currPhysicalDevice = physicalDevices[uiPhysicalDevice];

        VkPhysicalDeviceProperties physicalDeviceProperties;
        VkPhysicalDeviceFeatures physicalDeviceFeatures;
        vkGetPhysicalDeviceProperties(currPhysicalDevice, &physicalDeviceProperties);
        vkGetPhysicalDeviceFeatures(currPhysicalDevice, &physicalDeviceFeatures);

        if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            // Queue family
            uint32 numQueueFamilies = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(currPhysicalDevice, &numQueueFamilies, nullptr);

            VkQueueFamilyProperties* queueFamilyProperties = new VkQueueFamilyProperties[numQueueFamilies];
            vkGetPhysicalDeviceQueueFamilyProperties(currPhysicalDevice, &numQueueFamilies, queueFamilyProperties);

            bool graphicsSupport = false;
            bool presentationSupport = false;

            for (uint32 uiQueueFamily = 0; uiQueueFamily < numQueueFamilies; ++uiQueueFamily)
            {
                if (queueFamilyProperties[uiQueueFamily].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    graphicsSupport = true;
                    vulkanContextResources->graphicsQueueIndex = uiQueueFamily;
                }

                VkBool32 presentSupport;
                vkGetPhysicalDeviceSurfaceSupportKHR(currPhysicalDevice, uiQueueFamily, vulkanContextResources->surface, &presentSupport);
                if (presentSupport)
                {
                    presentationSupport = true;
                    vulkanContextResources->presentationQueueIndex = uiQueueFamily;
                }
            }

            if (graphicsSupport && presentationSupport)
            {
                // Select the current physical device
                vulkanContextResources->physicalDevice = currPhysicalDevice;
                break;
            }

            delete queueFamilyProperties;
        }
    }
    delete physicalDevices;

    if (vulkanContextResources->physicalDevice == VK_NULL_HANDLE)
    {
        // We did not select a physical device
        // TODO: Log? Fail?
    }

    // Logical device
    const uint32 numQueues = 2;
    VkDeviceQueueCreateInfo deviceQueueCreateInfos[numQueues] = {};

    // Create graphics queue
    deviceQueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfos[0].queueFamilyIndex = vulkanContextResources->graphicsQueueIndex;
    deviceQueueCreateInfos[0].queueCount = 1;
    float graphicsQueuePriority = 1.0f;
    deviceQueueCreateInfos[0].pQueuePriorities = &graphicsQueuePriority;

    // Create presentation queue
    deviceQueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfos[1].queueFamilyIndex = vulkanContextResources->presentationQueueIndex;
    deviceQueueCreateInfos[1].queueCount = 1;
    float presentationQueuePriority = 1.0f;
    deviceQueueCreateInfos[1].pQueuePriorities = &presentationQueuePriority;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos;
    deviceCreateInfo.queueCreateInfoCount = numQueues;
    VkPhysicalDeviceFeatures requestedPhysicalDeviceFeatures = {};
    // TODO: request physical device features
    deviceCreateInfo.pEnabledFeatures = &requestedPhysicalDeviceFeatures;
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;
    deviceCreateInfo.enabledExtensionCount = 0;
    deviceCreateInfo.ppEnabledExtensionNames = nullptr;

    result = vkCreateDevice(vulkanContextResources->physicalDevice, &deviceCreateInfo, nullptr, &vulkanContextResources->device);
    if (result != VK_SUCCESS)
    {
        // TODO: Log? Fail?
        return 1;
    }

    // Graphics queue
    vkGetDeviceQueue(vulkanContextResources->device, vulkanContextResources->graphicsQueueIndex, 0, &vulkanContextResources->graphicsQueue);
    vkGetDeviceQueue(vulkanContextResources->device, vulkanContextResources->presentationQueueIndex, 0, &vulkanContextResources->presentationQueue);

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

    vkDestroyDevice(vulkanContextResources->device, nullptr);
    vkDestroySurfaceKHR(vulkanContextResources->instance, vulkanContextResources->surface, nullptr);
    vkDestroyInstance(vulkanContextResources->instance, nullptr);
}

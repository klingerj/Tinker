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

int InitVulkan(VulkanContextResources* vulkanContextResources, HINSTANCE hInstance, HWND windowHandle, uint32 width, uint32 height)
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

    const uint32 numRequiredPhysicalDeviceExtensions = 1;
    const char* requiredPhysicalDeviceExtensions[numRequiredPhysicalDeviceExtensions] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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
            bool extensionSupport[numRequiredPhysicalDeviceExtensions] = { false };

            // Check queue family properties
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

            uint32 numAvailablePhysicalDeviceExtensions = 0;
            vkEnumerateDeviceExtensionProperties(currPhysicalDevice, nullptr, &numAvailablePhysicalDeviceExtensions, nullptr);

            if (numAvailablePhysicalDeviceExtensions == 0)
            {
                // TODO: Log? Fail?
            }

            VkExtensionProperties* availablePhysicalDeviceExtensions = new VkExtensionProperties[numAvailablePhysicalDeviceExtensions];
            vkEnumerateDeviceExtensionProperties(currPhysicalDevice, nullptr, &numAvailablePhysicalDeviceExtensions, availablePhysicalDeviceExtensions);

            for (uint32 uiReqExt = 0; uiReqExt < numRequiredPhysicalDeviceExtensions; ++uiReqExt)
            {
                for (uint32 uiAvailExt = 0; uiAvailExt < numAvailablePhysicalDeviceExtensions; ++uiAvailExt)
                {
                    if (!strcmp(availablePhysicalDeviceExtensions[uiAvailExt].extensionName, requiredPhysicalDeviceExtensions[uiReqExt]))
                    {
                        extensionSupport[uiReqExt] = true;
                        break;
                    }
                }
            }
            delete availablePhysicalDeviceExtensions;

            for (uint32 uiReqExt = 0; uiReqExt < numRequiredPhysicalDeviceExtensions; ++uiReqExt)
            {
                if (!extensionSupport[uiReqExt])
                {
                    // TODO: Log? Fail?
                }
            }

            // Extension support assumed at this point
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
    deviceCreateInfo.enabledExtensionCount = numRequiredPhysicalDeviceExtensions;
    deviceCreateInfo.ppEnabledExtensionNames = requiredPhysicalDeviceExtensions;

    result = vkCreateDevice(vulkanContextResources->physicalDevice, &deviceCreateInfo, nullptr, &vulkanContextResources->device);
    if (result != VK_SUCCESS)
    {
        // TODO: Log? Fail?
        return 1;
    }

    // Queues
    vkGetDeviceQueue(vulkanContextResources->device, vulkanContextResources->graphicsQueueIndex, 0, &vulkanContextResources->graphicsQueue);
    vkGetDeviceQueue(vulkanContextResources->device, vulkanContextResources->presentationQueueIndex, 0, &vulkanContextResources->presentationQueue);

    // Swap chain
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanContextResources->physicalDevice, vulkanContextResources->surface, &capabilities);

    VkExtent2D optimalExtent = {};
    if (capabilities.currentExtent.width != 0xffffffff)
    {
        optimalExtent = capabilities.currentExtent;
    }
    else
    {
        optimalExtent.width = CLAMP(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        optimalExtent.height = CLAMP(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    uint32 numSwapChainImages = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) // 0 can indicate no maximum
    {
        numSwapChainImages = MIN(numSwapChainImages, capabilities.maxImageCount);
    }

    uint32 numAvailableSurfaceFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContextResources->physicalDevice, vulkanContextResources->surface, &numAvailableSurfaceFormats, nullptr);

    if (numAvailableSurfaceFormats == 0)
    {
        // TODO: Log? Fail?
    }

    VkSurfaceFormatKHR* availableSurfaceFormats = new VkSurfaceFormatKHR[numAvailableSurfaceFormats];
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContextResources->physicalDevice, vulkanContextResources->surface, &numAvailableSurfaceFormats, availableSurfaceFormats);

    VkSurfaceFormatKHR chosenFormat = availableSurfaceFormats[0];
    for (uint32 uiAvailFormat = 1; uiAvailFormat < numAvailableSurfaceFormats; ++uiAvailFormat)
    {
        if (availableSurfaceFormats[uiAvailFormat].format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableSurfaceFormats[uiAvailFormat].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosenFormat = availableSurfaceFormats[uiAvailFormat];
        }
    }
    delete availableSurfaceFormats;

    uint32 numAvailablePresentModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanContextResources->physicalDevice, vulkanContextResources->surface, &numAvailablePresentModes, nullptr);

    if (numAvailablePresentModes == 0)
    {
        // TODO: Log? Fail?
    }

    VkPresentModeKHR* availablePresentModes = new VkPresentModeKHR[numAvailablePresentModes];
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanContextResources->physicalDevice, vulkanContextResources->surface, &numAvailablePresentModes, availablePresentModes);

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32 uiAvailPresMode = 0; uiAvailPresMode < numAvailablePresentModes; ++uiAvailPresMode)
    {
        if (availablePresentModes[uiAvailPresMode] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            chosenPresentMode = availablePresentModes[uiAvailPresMode];
        }
    }
    delete availablePresentModes;

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = vulkanContextResources->surface;
    swapChainCreateInfo.minImageCount = numSwapChainImages;
    swapChainCreateInfo.imageFormat = chosenFormat.format;
    swapChainCreateInfo.imageColorSpace = chosenFormat.colorSpace;
    swapChainCreateInfo.imageExtent = optimalExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (vulkanContextResources->graphicsQueueIndex != vulkanContextResources->presentationQueueIndex) {
        const uint32 numQueueFamilyIndices = 2;
        uint32 queueFamilyIndices[numQueueFamilyIndices] = { vulkanContextResources->graphicsQueueIndex, vulkanContextResources->presentationQueueIndex };
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = numQueueFamilyIndices;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }
    swapChainCreateInfo.preTransform = capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = chosenPresentMode;

    vulkanContextResources->swapChainExtent = optimalExtent;
    vulkanContextResources->swapChainFormat = chosenFormat.format;
    vulkanContextResources->numSwapChainImages = numSwapChainImages;

    result = vkCreateSwapchainKHR(vulkanContextResources->device, &swapChainCreateInfo, nullptr, &vulkanContextResources->swapChain);
    if (result != VK_SUCCESS)
    {
        // TODO: Log? Fail?
    }

    vkGetSwapchainImagesKHR(vulkanContextResources->device, vulkanContextResources->swapChain, &numSwapChainImages, nullptr);
    vulkanContextResources->swapChainImages = new VkImage[numSwapChainImages];
    vkGetSwapchainImagesKHR(vulkanContextResources->device, vulkanContextResources->swapChain, &numSwapChainImages, vulkanContextResources->swapChainImages);

    vulkanContextResources->swapChainImageViews = new VkImageView[numSwapChainImages];
    for (uint32 uiImageView = 0; uiImageView < numSwapChainImages; ++uiImageView)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = vulkanContextResources->swapChainImages[uiImageView];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = vulkanContextResources->swapChainFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(vulkanContextResources->device, &imageViewCreateInfo, nullptr, &vulkanContextResources->swapChainImageViews[uiImageView]);
        if (result != VK_SUCCESS)
        {
            // TODO: Log? Fail?
        }
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

    delete vulkanContextResources->swapChainImages;
    for (uint32 uiImageView = 0; uiImageView < vulkanContextResources->numSwapChainImages; ++uiImageView)
    {
        vkDestroyImageView(vulkanContextResources->device, vulkanContextResources->swapChainImageViews[uiImageView], nullptr);
    }
    delete vulkanContextResources->swapChainImageViews;

    vkDestroySwapchainKHR(vulkanContextResources->device, vulkanContextResources->swapChain, nullptr);
    vkDestroyDevice(vulkanContextResources->device, nullptr);
    vkDestroySurfaceKHR(vulkanContextResources->instance, vulkanContextResources->surface, nullptr);
    vkDestroyInstance(vulkanContextResources->instance, nullptr);
}

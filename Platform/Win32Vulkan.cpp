#include "../Include/Platform/Win32Vulkan.h"
#include "../Include/Platform/Win32Utilities.h"
#include "../Include/Platform/Win32Logs.h"

#include <cstring>

namespace Tinker
{
    namespace Platform
    {
        namespace Graphics
        {
            #ifdef _DEBUG
            static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFunc(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData)
            {
                OutputDebugString("VALIDATION LAYER:\n");
                OutputDebugString(pCallbackData->pMessage);
                OutputDebugString("\n");
                return VK_FALSE;
            }
            #endif

            int InitVulkan(VulkanContextResources* vulkanContextResources,
                HINSTANCE hInstance, HWND windowHandle,
                uint32 width, uint32 height)
            {
                vulkanContextResources->vulkanMemResourcePool.Init(VULKAN_RESOURCE_POOL_MAX, 16);
                vulkanContextResources->vulkanMappedMemPtrPool.Init(VULKAN_RESOURCE_POOL_MAX, 16);
                vulkanContextResources->vulkanFramebufferPool.Init(VULKAN_RESOURCE_POOL_MAX, 16);
                vulkanContextResources->vulkanImageViewPool.Init(VULKAN_RESOURCE_POOL_MAX, 16);

                vulkanContextResources->windowWidth = width;
                vulkanContextResources->windowHeight = height;

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
                LogMsg("******** Available Instance Extensions: ********", eLogSeverityInfo);
                for (uint32 uiAvailExt = 0; uiAvailExt < numAvailableExtensions; ++uiAvailExt)
                {
                    LogMsg(availableExtensions[uiAvailExt].extensionName, eLogSeverityInfo);
                }
                delete availableExtensions;

                // Validation layers
                #ifdef _DEBUG
                const uint32 numRequiredLayers = 1;
                const char* requestedLayersStr[numRequiredLayers] = { "VK_LAYER_KHRONOS_validation" };

                uint32 numAvailableLayers = 0;
                vkEnumerateInstanceLayerProperties(&numAvailableLayers, nullptr);

                if (numAvailableLayers == 0)
                {
                    LogMsg("Zero available instance layers!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkLayerProperties* availableLayers = new VkLayerProperties[numAvailableLayers];
                vkEnumerateInstanceLayerProperties(&numAvailableLayers, availableLayers);

                LogMsg("******** Available Instance Layers: ********", eLogSeverityInfo);
                for (uint32 uiAvailLayer = 0; uiAvailLayer < numAvailableLayers; ++uiAvailLayer)
                {
                    LogMsg(availableLayers[uiAvailLayer].layerName, eLogSeverityInfo);
                }

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
                        LogMsg("Requested instance layer not supported!", eLogSeverityCritical);
                        LogMsg(requestedLayersStr[uiReqLayer], eLogSeverityCritical);
                        TINKER_ASSERT(0);
                    }
                }

                instanceCreateInfo.enabledLayerCount = numRequiredLayers;
                instanceCreateInfo.ppEnabledLayerNames = requestedLayersStr;
                #endif

                VkResult result = vkCreateInstance(&instanceCreateInfo,
                    nullptr,
                    &vulkanContextResources->instance);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan instance!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
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

                PFN_vkCreateDebugUtilsMessengerEXT dbgCreateFunc =
                    (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanContextResources->instance,
                                                                              "vkCreateDebugUtilsMessengerEXT");
                if (dbgCreateFunc)
                {
                    dbgCreateFunc(vulkanContextResources->instance,
                        &dbgUtilsMsgCreateInfo,
                        nullptr,
                        &vulkanContextResources->debugMessenger);
                }
                else
                {
                    LogMsg("Failed to get create debug utils messenger proc addr!", eLogSeverityCritical);
                }
                #endif

                // Surface
                VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {};
                win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
                win32SurfaceCreateInfo.hinstance = hInstance;
                win32SurfaceCreateInfo.hwnd = windowHandle;

                result = vkCreateWin32SurfaceKHR(vulkanContextResources->instance,
                    &win32SurfaceCreateInfo,
                    NULL,
                    &vulkanContextResources->surface);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Win32SurfaceKHR!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                // Physical device
                uint32 numPhysicalDevices = 0;
                vkEnumeratePhysicalDevices(vulkanContextResources->instance, &numPhysicalDevices, nullptr);

                if (numPhysicalDevices == 0)
                {
                    LogMsg("Zero available physical devices!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[numPhysicalDevices];
                vkEnumeratePhysicalDevices(vulkanContextResources->instance, &numPhysicalDevices, physicalDevices);

                const uint32 numRequiredPhysicalDeviceExtensions = 1;
                const char* requiredPhysicalDeviceExtensions[numRequiredPhysicalDeviceExtensions] =
                {
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME
                };

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
                            vkGetPhysicalDeviceSurfaceSupportKHR(currPhysicalDevice,
                                uiQueueFamily,
                                vulkanContextResources->surface,
                                &presentSupport);
                            if (presentSupport)
                            {
                                presentationSupport = true;
                                vulkanContextResources->presentationQueueIndex = uiQueueFamily;
                            }
                        }

                        uint32 numAvailablePhysicalDeviceExtensions = 0;
                        vkEnumerateDeviceExtensionProperties(currPhysicalDevice,
                            nullptr,
                            &numAvailablePhysicalDeviceExtensions,
                            nullptr);

                        if (numAvailablePhysicalDeviceExtensions == 0)
                        {
                            LogMsg("Zero available device extensions!", eLogSeverityCritical);
                            TINKER_ASSERT(0);
                        }

                        VkExtensionProperties* availablePhysicalDeviceExtensions = new VkExtensionProperties[numAvailablePhysicalDeviceExtensions];
                        vkEnumerateDeviceExtensionProperties(currPhysicalDevice,
                            nullptr,
                            &numAvailablePhysicalDeviceExtensions,
                            availablePhysicalDeviceExtensions);

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
                                LogMsg("Requested device extension not supported!", eLogSeverityCritical);
                                LogMsg(requiredPhysicalDeviceExtensions[uiReqExt], eLogSeverityCritical);
                                TINKER_ASSERT(0);
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
                    LogMsg("No physical device chosen!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
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

                result = vkCreateDevice(vulkanContextResources->physicalDevice,
                    &deviceCreateInfo,
                    nullptr,
                    &vulkanContextResources->device);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan device!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                // Queues
                vkGetDeviceQueue(vulkanContextResources->device,
                    vulkanContextResources->graphicsQueueIndex,
                    0,
                    &vulkanContextResources->graphicsQueue);
                vkGetDeviceQueue(vulkanContextResources->device,
                    vulkanContextResources->presentationQueueIndex,
                    0, 
                    &vulkanContextResources->presentationQueue);

                // Swap chain
                CreateSwapChain(vulkanContextResources);

                // Command pool
                VkCommandPoolCreateInfo commandPoolCreateInfo = {};
                commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                commandPoolCreateInfo.queueFamilyIndex = vulkanContextResources->graphicsQueueIndex;
                commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

                result = vkCreateCommandPool(vulkanContextResources->device,
                    &commandPoolCreateInfo,
                    nullptr,
                    &vulkanContextResources->commandPool);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan command pool!", eLogSeverityCritical);
                }

                vulkanContextResources->commandBuffers = new VkCommandBuffer[vulkanContextResources->numSwapChainImages];

                VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
                commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                commandBufferAllocInfo.commandPool = vulkanContextResources->commandPool;
                commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                commandBufferAllocInfo.commandBufferCount = vulkanContextResources->numSwapChainImages;

                result = vkAllocateCommandBuffers(vulkanContextResources->device,
                    &commandBufferAllocInfo,
                    vulkanContextResources->commandBuffers);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to allocate Vulkan command buffers!", eLogSeverityCritical);
                }

                // Semaphores
                VkSemaphoreCreateInfo semaphoreCreateInfo = {};
                semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

                result = vkCreateSemaphore(vulkanContextResources->device,
                    &semaphoreCreateInfo,
                    nullptr,
                    &vulkanContextResources->swapChainImageAvailableSemaphore);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan semaphore!", eLogSeverityCritical);
                }
                result = vkCreateSemaphore(vulkanContextResources->device,
                    &semaphoreCreateInfo,
                    nullptr,
                    &vulkanContextResources->renderCompleteSemaphore);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan semaphore!", eLogSeverityCritical);
                }

                VkFenceCreateInfo fenceCreateInfo = {};
                fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                result = vkCreateFence(vulkanContextResources->device, &fenceCreateInfo, nullptr, &vulkanContextResources->fence);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan fence!", eLogSeverityCritical);
                }

                vulkanContextResources->isInitted = true;

                return 0;
            }

            void CreateSwapChain(VulkanContextResources* vulkanContextResources)
            {
                VkSurfaceCapabilitiesKHR capabilities;
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanContextResources->physicalDevice,
                    vulkanContextResources->surface,
                    &capabilities);

                VkExtent2D optimalExtent = {};
                if (capabilities.currentExtent.width != 0xffffffff)
                {
                    optimalExtent = capabilities.currentExtent;
                }
                else
                {
                    optimalExtent.width =
                        CLAMP(vulkanContextResources->windowWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                    optimalExtent.height =
                        CLAMP(vulkanContextResources->windowHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
                }

                uint32 numSwapChainImages = capabilities.minImageCount + 1;
                if (capabilities.maxImageCount > 0) // 0 can indicate no maximum
                {
                    numSwapChainImages = MIN(numSwapChainImages, capabilities.maxImageCount);
                }

                uint32 numAvailableSurfaceFormats;
                vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContextResources->physicalDevice,
                    vulkanContextResources->surface,
                    &numAvailableSurfaceFormats,
                    nullptr);

                if (numAvailableSurfaceFormats == 0)
                {
                    LogMsg("Zero available Vulkan swap chain surface formats!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkSurfaceFormatKHR* availableSurfaceFormats = new VkSurfaceFormatKHR[numAvailableSurfaceFormats];
                vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContextResources->physicalDevice,
                    vulkanContextResources->surface,
                    &numAvailableSurfaceFormats,
                    availableSurfaceFormats);

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
                vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanContextResources->physicalDevice,
                    vulkanContextResources->surface,
                    &numAvailablePresentModes,
                    nullptr);

                if (numAvailablePresentModes == 0)
                {
                    LogMsg("Zero available Vulkan swap chain present modes!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkPresentModeKHR* availablePresentModes = new VkPresentModeKHR[numAvailablePresentModes];
                vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanContextResources->physicalDevice,
                    vulkanContextResources->surface,
                    &numAvailablePresentModes,
                    availablePresentModes);

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
                    uint32 queueFamilyIndices[numQueueFamilyIndices] =
                    {
                        vulkanContextResources->graphicsQueueIndex,
                        vulkanContextResources->presentationQueueIndex
                    };

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

                VkResult result = vkCreateSwapchainKHR(vulkanContextResources->device,
                    &swapChainCreateInfo,
                    nullptr,
                    &vulkanContextResources->swapChain);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan swap chain!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                vkGetSwapchainImagesKHR(vulkanContextResources->device,
                    vulkanContextResources->swapChain,
                    &numSwapChainImages,
                    nullptr);
                vulkanContextResources->swapChainImages = new VkImage[numSwapChainImages];
                vkGetSwapchainImagesKHR(vulkanContextResources->device,
                    vulkanContextResources->swapChain,
                    &numSwapChainImages,
                    vulkanContextResources->swapChainImages);

                vulkanContextResources->swapChainImageViewHandles = new uint32[numSwapChainImages];
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

                    uint32 newImageViewHandle = vulkanContextResources->vulkanImageViewPool.Alloc();
                    vulkanContextResources->swapChainImageViewHandles[uiImageView] = newImageViewHandle;
                    VkImageView* newImageView = vulkanContextResources->vulkanImageViewPool.PtrFromHandle(newImageViewHandle);

                    result = vkCreateImageView(vulkanContextResources->device,
                        &imageViewCreateInfo,
                        nullptr,
                        newImageView);
                    if (result != VK_SUCCESS)
                    {
                        LogMsg("Failed to create Vulkan image view!", eLogSeverityCritical);
                        TINKER_ASSERT(0);
                    }
                }

                InitRenderPassResources(vulkanContextResources);
                InitGraphicsPipelineResources(vulkanContextResources);

                // Swap chain framebuffers
                vulkanContextResources->swapChainFramebufferHandles = new uint32[vulkanContextResources->numSwapChainImages];
                for (uint32 uiFramebuffer = 0; uiFramebuffer < vulkanContextResources->numSwapChainImages; ++uiFramebuffer)
                {
                    vulkanContextResources->swapChainFramebufferHandles[uiFramebuffer] =
                        CreateFramebuffer(vulkanContextResources,
                            &vulkanContextResources->swapChainImageViewHandles[uiFramebuffer], 1,
                            vulkanContextResources->swapChainExtent.width, vulkanContextResources->swapChainExtent.height);
                }

                vulkanContextResources->isSwapChainValid = true;
            }

            void DestroySwapChain(VulkanContextResources* vulkanContextResources)
            {
                vulkanContextResources->isSwapChainValid = false;
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?

                for (uint32 uiFramebuffer = 0; uiFramebuffer < vulkanContextResources->numSwapChainImages; ++uiFramebuffer)
                {
                    uint32 framebufferHandle = vulkanContextResources->swapChainFramebufferHandles[uiFramebuffer];
                    DestroyFramebuffer(vulkanContextResources, framebufferHandle);
                }
                delete vulkanContextResources->swapChainFramebufferHandles;

                vkDestroyPipeline(vulkanContextResources->device, vulkanContextResources->pipeline, nullptr);
                vkDestroyPipelineLayout(vulkanContextResources->device, vulkanContextResources->pipelineLayout, nullptr);
                vkDestroyRenderPass(vulkanContextResources->device, vulkanContextResources->renderPass, nullptr);

                for (uint32 uiImageView = 0; uiImageView < vulkanContextResources->numSwapChainImages; ++uiImageView)
                {
                    DestroyImageViewResource(vulkanContextResources, vulkanContextResources->swapChainImageViewHandles[uiImageView]);
                }
                delete vulkanContextResources->swapChainImageViewHandles;
                delete vulkanContextResources->swapChainImages;
                
                vkDestroySwapchainKHR(vulkanContextResources->device, vulkanContextResources->swapChain, nullptr);
            }

            VkShaderModule CreateShaderModule(const char* shaderCode, uint32 numShaderCodeBytes, VulkanContextResources* vulkanContextResources)
            {
                VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
                shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                shaderModuleCreateInfo.codeSize = numShaderCodeBytes;
                shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode);

                VkShaderModule shaderModule;
                VkResult result = vkCreateShaderModule(vulkanContextResources->device, &shaderModuleCreateInfo, nullptr, &shaderModule);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan shader module!", eLogSeverityCritical);
                    return VK_NULL_HANDLE;
                }
                return shaderModule;
            }

            void InitGraphicsPipelineResources(VulkanContextResources* vulkanContextResources)
            {
                const char* vertexShaderFileName   = "..\\Shaders\\basic_vert.spv";
                const char* fragmentShaderFileName = "..\\Shaders\\basic_frag.spv";

                uint32 vertexShaderFileSize   = GetFileSize(vertexShaderFileName);
                uint32 fragmentShaderFileSize = GetFileSize(fragmentShaderFileName);

                Memory::LinearAllocator linearAllocator;
                linearAllocator.Init(vertexShaderFileSize + fragmentShaderFileSize, 4);

                uint8* vertexShaderBuffer   = linearAllocator.Alloc(vertexShaderFileSize, 1);
                uint8* fragmentShaderBuffer = linearAllocator.Alloc(fragmentShaderFileSize, 1);
                const char* vertexShaderCode   = (const char*)ReadEntireFile(vertexShaderFileName, vertexShaderFileSize, vertexShaderBuffer);
                const char* fragmentShaderCode = (const char*)ReadEntireFile(fragmentShaderFileName, fragmentShaderFileSize, fragmentShaderBuffer);

                VkShaderModule vertexShaderModule   = CreateShaderModule(vertexShaderCode, vertexShaderFileSize, vulkanContextResources);
                VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode, fragmentShaderFileSize, vulkanContextResources);
                
                // Programmable shader stages
                VkPipelineShaderStageCreateInfo shaderStages[2] = {};
                shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
                shaderStages[0].module = vertexShaderModule;
                shaderStages[0].pName = "main";

                shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                shaderStages[1].module = fragmentShaderModule;
                shaderStages[1].pName = "main";

                // Fixed function
                const uint32 numBindings = 1;
                VkVertexInputBindingDescription vertexInputBindDescs[numBindings] = {};
                vertexInputBindDescs[0].binding = 0;
                vertexInputBindDescs[0].stride = sizeof(VulkanVertexPosition);
                vertexInputBindDescs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                const uint32 numAttributes = 1;
                VkVertexInputAttributeDescription vertexInputAttrDescs[numAttributes] = {};
                vertexInputAttrDescs[0].binding = 0;
                vertexInputAttrDescs[0].location = 0;
                vertexInputAttrDescs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
                vertexInputAttrDescs[0].offset = 0;

                VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                vertexInputInfo.vertexBindingDescriptionCount = numBindings;
                vertexInputInfo.pVertexBindingDescriptions = vertexInputBindDescs;
                vertexInputInfo.vertexAttributeDescriptionCount = numAttributes;
                vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttrDescs;

                VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
                inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                inputAssembly.primitiveRestartEnable = VK_FALSE;

                VkViewport viewport = {};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = (float)vulkanContextResources->swapChainExtent.width;
                viewport.height = (float)vulkanContextResources->swapChainExtent.height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                VkRect2D scissor = {};
                scissor.offset = { 0, 0 };
                scissor.extent = vulkanContextResources->swapChainExtent;

                VkPipelineViewportStateCreateInfo viewportState = {};
                viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewportState.viewportCount = 1;
                viewportState.pViewports = &viewport;
                viewportState.scissorCount = 1;
                viewportState.pScissors = &scissor;

                VkPipelineRasterizationStateCreateInfo rasterizer = {};
                rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizer.depthClampEnable = VK_FALSE;
                rasterizer.rasterizerDiscardEnable = VK_FALSE;
                rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
                rasterizer.lineWidth = 1.0f;
                rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
                rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                rasterizer.depthBiasEnable = VK_FALSE;
                rasterizer.depthBiasConstantFactor = 0.0f;
                rasterizer.depthBiasClamp = 0.0f;
                rasterizer.depthBiasSlopeFactor = 0.0f;

                VkPipelineMultisampleStateCreateInfo multisampling = {};
                multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampling.sampleShadingEnable = VK_FALSE;
                multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
                multisampling.minSampleShading = 1.0f;
                multisampling.pSampleMask = nullptr;
                multisampling.alphaToCoverageEnable = VK_FALSE;
                multisampling.alphaToOneEnable = VK_FALSE;

                VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
                colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                      VK_COLOR_COMPONENT_G_BIT |
                                                      VK_COLOR_COMPONENT_B_BIT |
                                                      VK_COLOR_COMPONENT_A_BIT;
                colorBlendAttachment.blendEnable = VK_TRUE;
                colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
                colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

                VkPipelineColorBlendStateCreateInfo colorBlending = {};
                colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlending.logicOpEnable = VK_FALSE;
                colorBlending.logicOp = VK_LOGIC_OP_COPY;
                colorBlending.attachmentCount = 1;
                colorBlending.pAttachments = &colorBlendAttachment;
                colorBlending.blendConstants[0] = 0.0f;
                colorBlending.blendConstants[1] = 0.0f;
                colorBlending.blendConstants[2] = 0.0f;
                colorBlending.blendConstants[3] = 0.0f;

                VkDynamicState dynamicStates[] = {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_LINE_WIDTH
                };

                VkPipelineDynamicStateCreateInfo dynamicState = {};
                dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicState.dynamicStateCount = 2;
                dynamicState.pDynamicStates = dynamicStates;

                VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = 0;
                pipelineLayoutInfo.pSetLayouts = nullptr;
                pipelineLayoutInfo.pushConstantRangeCount = 0;
                pipelineLayoutInfo.pPushConstantRanges = nullptr;

                VkResult result = vkCreatePipelineLayout(vulkanContextResources->device,
                    &pipelineLayoutInfo,
                    nullptr,
                    &vulkanContextResources->pipelineLayout);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan pipeline layout!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
                pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                pipelineCreateInfo.stageCount = 2;
                pipelineCreateInfo.pStages = shaderStages;
                pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
                pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
                pipelineCreateInfo.pViewportState = &viewportState;
                pipelineCreateInfo.pRasterizationState = &rasterizer;
                pipelineCreateInfo.pMultisampleState = &multisampling;
                pipelineCreateInfo.pDepthStencilState = nullptr;
                pipelineCreateInfo.pColorBlendState = &colorBlending;
                pipelineCreateInfo.pDynamicState = nullptr;
                pipelineCreateInfo.layout = vulkanContextResources->pipelineLayout;
                pipelineCreateInfo.renderPass = vulkanContextResources->renderPass;
                pipelineCreateInfo.subpass = 0;
                pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
                pipelineCreateInfo.basePipelineIndex = -1;

                result = vkCreateGraphicsPipelines(vulkanContextResources->device,
                    VK_NULL_HANDLE,
                    1,
                    &pipelineCreateInfo,
                    nullptr,
                    &vulkanContextResources->pipeline);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan graphics pipeline!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                vkDestroyShaderModule(vulkanContextResources->device, vertexShaderModule, nullptr);
                vkDestroyShaderModule(vulkanContextResources->device, fragmentShaderModule, nullptr);
            }

            void InitRenderPassResources(VulkanContextResources* vulkanContextResources)
            {
                VkAttachmentDescription colorAttachment = {};
                colorAttachment.format = vulkanContextResources->swapChainFormat;
                colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                VkAttachmentReference colorAttachmentRef = {};
                colorAttachmentRef.attachment = 0;
                colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass = {};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &colorAttachmentRef;

                VkSubpassDependency subpassDependency = {};
                subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                subpassDependency.srcAccessMask = 0;
                subpassDependency.dstSubpass = 0;
                subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                VkRenderPassCreateInfo renderPassCreateInfo = {};
                renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                renderPassCreateInfo.attachmentCount = 1;
                renderPassCreateInfo.pAttachments = &colorAttachment;
                renderPassCreateInfo.subpassCount = 1;
                renderPassCreateInfo.pSubpasses = &subpass;
                renderPassCreateInfo.dependencyCount = 1;
                renderPassCreateInfo.pDependencies = &subpassDependency;

                VkResult result = vkCreateRenderPass(vulkanContextResources->device, &renderPassCreateInfo, nullptr, &vulkanContextResources->renderPass);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan render pass!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
            }

            void SubmitFrame(VulkanContextResources* vulkanContextResources)
            {
                vkWaitForFences(vulkanContextResources->device, 1, &vulkanContextResources->fence, VK_TRUE, UINT64_MAX);
                vkResetFences(vulkanContextResources->device, 1, &vulkanContextResources->fence);

                // Submit
                VkSubmitInfo submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphores[1] = { vulkanContextResources->swapChainImageAvailableSemaphore };
                VkPipelineStageFlags waitStages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage];

                VkSemaphore signalSemaphores[1] = { vulkanContextResources->renderCompleteSemaphore };
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                VkResult result = vkQueueSubmit(vulkanContextResources->graphicsQueue, 1, &submitInfo, vulkanContextResources->fence);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to submit command buffer to queue!", eLogSeverityCritical);
                }

                // Present
                VkPresentInfoKHR presentInfo = {};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = signalSemaphores;
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = &vulkanContextResources->swapChain;
                presentInfo.pImageIndices = &vulkanContextResources->currentSwapChainImage;
                presentInfo.pResults = nullptr;

                result = vkQueuePresentKHR(vulkanContextResources->presentationQueue, &presentInfo);

                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to present swap chain!", eLogSeverityInfo);
                    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
                    {
                        LogMsg("Recreating swap chain!", eLogSeverityInfo);
                        DestroySwapChain(vulkanContextResources);
                        CreateSwapChain(vulkanContextResources);
                        return; // Don't present on this frame
                    }
                    else
                    {
                        LogMsg("Not recreating swap chain!", eLogSeverityCritical);
                        TINKER_ASSERT(0);
                    }
                }

                vkQueueWaitIdle(vulkanContextResources->presentationQueue);
            }

            void AllocGPUMemory(VulkanContextResources* vulkanContextResources, VkDeviceMemory* deviceMemory,
                VkMemoryRequirements memRequirements, VkMemoryPropertyFlags memPropertyFlags)
            {
                // Check for memory type support
                VkPhysicalDeviceMemoryProperties memProperties;
                vkGetPhysicalDeviceMemoryProperties(vulkanContextResources->physicalDevice, &memProperties);

                uint32 memTypeIndex = 0xffffffff;
                for (uint32 uiMemType = 0; uiMemType < memProperties.memoryTypeCount; ++uiMemType)
                {
                    if (((1 << uiMemType) & memRequirements.memoryTypeBits) &&
                        (memProperties.memoryTypes[uiMemType].propertyFlags & memPropertyFlags) == memPropertyFlags)
                    {
                        memTypeIndex = uiMemType;
                    }
                }
                if (memTypeIndex == 0xffffffff)
                {
                    LogMsg("Failed to find memory property flags!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkMemoryAllocateInfo memAllocInfo = {};
                memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                memAllocInfo.allocationSize = memRequirements.size;
                memAllocInfo.memoryTypeIndex = memTypeIndex;
                VkResult result = vkAllocateMemory(vulkanContextResources->device, &memAllocInfo, nullptr, deviceMemory);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to allocate gpu memory!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
            }

            void CreateBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes,
                VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
                VkBuffer& buffer, VkDeviceMemory& deviceMemory)
            {
                VkBufferCreateInfo bufferCreateInfo = {};
                bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferCreateInfo.size = sizeInBytes;
                bufferCreateInfo.usage = usageFlags;
                bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                VkResult result = vkCreateBuffer(vulkanContextResources->device, &bufferCreateInfo, nullptr, &buffer);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to allocate Vulkan buffer!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkMemoryRequirements memRequirements;
                vkGetBufferMemoryRequirements(vulkanContextResources->device, buffer, &memRequirements);

                AllocGPUMemory(vulkanContextResources, &deviceMemory, memRequirements, propertyFlags);

                vkBindBufferMemory(vulkanContextResources->device, buffer, deviceMemory, 0);
            }

            uint32 CreateVertexBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes, BufferType bufferType)
            {
                uint32 newResourceHandle =
                    vulkanContextResources->vulkanMemResourcePool.Alloc();
                TINKER_ASSERT(newResourceHandle != 0xffffffff);

                VulkanMemResource* newResource =
                    vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(newResourceHandle);

                VkBufferUsageFlags usageFlags = {};
                switch (bufferType)
                {
                    case eVertexBuffer:
                    {
                        usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                        break;
                    }

                    case eIndexBuffer:
                    {
                        usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                        break;
                    }

                    default:
                    {
                        break;
                    }
                }
                usageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

                CreateBuffer(vulkanContextResources, sizeInBytes,
                    usageFlags,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    newResource->buffer,
                    newResource->deviceMemory);
                
                return newResourceHandle;
            }

            VulkanStagingBufferData CreateStagingBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes)
            {
                uint32 newResourceHandle =
                    vulkanContextResources->vulkanMemResourcePool.Alloc();
                TINKER_ASSERT(newResourceHandle != 0xffffffff);

                VulkanMemResource* newResource =
                    vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(newResourceHandle);

                CreateBuffer(vulkanContextResources, sizeInBytes,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    newResource->buffer,
                    newResource->deviceMemory);

                uint32 newMappedMemHandle =
                    vulkanContextResources->vulkanMappedMemPtrPool.Alloc();
                void** newMappedMem =
                    vulkanContextResources->vulkanMappedMemPtrPool.PtrFromHandle(newMappedMemHandle);

                vkMapMemory(vulkanContextResources->device, newResource->deviceMemory, 0, sizeInBytes, 0, newMappedMem);

                VulkanStagingBufferData data;
                data.handle = newResourceHandle;
                data.mappedMemory = *newMappedMem;

                return data;
            }

            void BeginVulkanCommandRecording(VulkanContextResources* vulkanContextResources)
            {
                // Acquire
                uint32 currentSwapChainImageIndex = 0xffffffff;
                VkResult result = vkAcquireNextImageKHR(vulkanContextResources->device, 
                    vulkanContextResources->swapChain,
                    (uint64)-1,
                    vulkanContextResources->swapChainImageAvailableSemaphore,
                    VK_NULL_HANDLE,
                    &currentSwapChainImageIndex);

                vulkanContextResources->currentSwapChainImage = currentSwapChainImageIndex;
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to acquire next swap chain image!", eLogSeverityInfo);
                    if (result == VK_ERROR_OUT_OF_DATE_KHR)
                    {
                        LogMsg("Recreating swap chain!", eLogSeverityInfo);
                        DestroySwapChain(vulkanContextResources);
                        CreateSwapChain(vulkanContextResources);
                        return; // Don't present on this frame
                    }
                    else
                    {
                        LogMsg("Not recreating swap chain!", eLogSeverityCritical);
                        TINKER_ASSERT(0);
                    }
                }

                VkCommandBufferBeginInfo commandBufferBeginInfo = {};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                commandBufferBeginInfo.flags = 0;
                commandBufferBeginInfo.pInheritanceInfo = nullptr;

                result = vkBeginCommandBuffer(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage], &commandBufferBeginInfo);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to begin Vulkan command buffer!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
            }

            void EndVulkanCommandRecording(VulkanContextResources* vulkanContextResources)
            {
                if (vulkanContextResources->currentSwapChainImage == 0xffffffff)
                {
                    TINKER_ASSERT(0);
                }

                VkResult result = vkEndCommandBuffer(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage]);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to end Vulkan command buffer!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
            }

            void VulkanRecordCommandDrawCall(VulkanContextResources* vulkanContextResources,
                uint32 vertexBufferHandle, uint32 indexBufferHandle,
                uint32 numIndices, uint32 numVertices)
            {
                if (vulkanContextResources->currentSwapChainImage == 0xffffffff)
                {
                    TINKER_ASSERT(0);
                }

                VkBuffer vertexBuffers[] = { vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(vertexBufferHandle)->buffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage],
                    0, 1, vertexBuffers, offsets);

                VkBuffer& indexBuffer = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(indexBufferHandle)->buffer;
                vkCmdBindIndexBuffer(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage],
                    indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdDrawIndexed(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage],
                    numIndices, 1, 0, 0, 0);
            }

            void VulkanRecordCommandMemoryTransfer(VulkanContextResources* vulkanContextResources,
                uint32 sizeInBytes, uint32 srcBufferHandle, uint32 dstBufferHandle)
            {
                if (vulkanContextResources->currentSwapChainImage == 0xffffffff)
                {
                    TINKER_ASSERT(0);
                }

                VkBuffer& dstBuffer = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(dstBufferHandle)->buffer;
                VkBuffer& srcBuffer = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(srcBufferHandle)->buffer;
                VkBufferCopy bufferCopy = {};

                bufferCopy.srcOffset = 0;
                bufferCopy.dstOffset = 0;
                bufferCopy.size = sizeInBytes;
                vkCmdCopyBuffer(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage],
                    srcBuffer,
                    dstBuffer,
                    1, &bufferCopy);
            }
            
            void VulkanRecordCommandRenderPassBegin(VulkanContextResources* vulkanContextResources,
                uint32 framebufferHandle, uint32 renderWidth, uint32 renderHeight)
            {
                if (vulkanContextResources->currentSwapChainImage == 0xffffffff)
                {
                    TINKER_ASSERT(0);
                }

                VkRenderPassBeginInfo renderPassBeginInfo = {};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderPass = vulkanContextResources->renderPass;

                if (framebufferHandle == 0xffffffff)
                {
                    uint32 swapChainFramebufferHandle = vulkanContextResources->swapChainFramebufferHandles[vulkanContextResources->currentSwapChainImage];;
                    renderPassBeginInfo.framebuffer = *vulkanContextResources->vulkanFramebufferPool.PtrFromHandle(swapChainFramebufferHandle);
                }
                else
                {
                    renderPassBeginInfo.framebuffer = *vulkanContextResources->vulkanFramebufferPool.PtrFromHandle(framebufferHandle);
                }

                renderPassBeginInfo.renderArea.offset = { 0, 0 };
                if (framebufferHandle == 0xffffffff)
                {
                    renderPassBeginInfo.renderArea.extent = vulkanContextResources->swapChainExtent;
                }
                else
                {
                    renderPassBeginInfo.renderArea.extent = VkExtent2D({ renderWidth, renderHeight });
                }

                VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
                renderPassBeginInfo.clearValueCount = 1;
                renderPassBeginInfo.pClearValues = &clearColor;

                vkCmdBeginRenderPass(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage],
                    &renderPassBeginInfo,
                    VK_SUBPASS_CONTENTS_INLINE);

                vkCmdBindPipeline(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage],
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    vulkanContextResources->pipeline);
            }

            void VulkanRecordCommandRenderPassEnd(VulkanContextResources* vulkanContextResources)
            {
                if (vulkanContextResources->currentSwapChainImage == 0xffffffff)
                {
                    TINKER_ASSERT(0);
                }

                vkCmdEndRenderPass(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage]);
            }

            uint32 CreateFramebuffer(VulkanContextResources* vulkanContextResources,
                uint32* imageViewResourceHandles, uint32 numImageViewResourceHandles,
                uint32 width, uint32 height)
            {
                uint32 newFramebufferHandle = vulkanContextResources->vulkanFramebufferPool.Alloc();
                TINKER_ASSERT(newFramebufferHandle != 0xffffffff);

                VkFramebuffer* newFramebuffer =
                    vulkanContextResources->vulkanFramebufferPool.PtrFromHandle(newFramebufferHandle);
                
                VkImageView* attachments = new VkImageView[numImageViewResourceHandles];
                for (uint32 uiImageView = 0; uiImageView < numImageViewResourceHandles; ++uiImageView)
                {
                    attachments[uiImageView] =
                        *vulkanContextResources->vulkanImageViewPool.PtrFromHandle(imageViewResourceHandles[uiImageView]);
                }

                VkFramebufferCreateInfo framebufferCreateInfo = {};
                framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferCreateInfo.renderPass = vulkanContextResources->renderPass;
                framebufferCreateInfo.attachmentCount = 1;
                framebufferCreateInfo.pAttachments = attachments;
                framebufferCreateInfo.width = width;
                framebufferCreateInfo.height = height;
                framebufferCreateInfo.layers = 1;

                VkResult result = vkCreateFramebuffer(vulkanContextResources->device,
                    &framebufferCreateInfo,
                    nullptr,
                    newFramebuffer);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan framebuffer!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
                delete attachments;

                return newFramebufferHandle;
            }
            
            uint32 CreateImageViewResource(VulkanContextResources* vulkanContextResources, uint32 imageResourceHandle)
            {
                uint32 newImageViewHandle = vulkanContextResources->vulkanImageViewPool.Alloc();
                VkImageView* newImageView = vulkanContextResources->vulkanImageViewPool.PtrFromHandle(newImageViewHandle);

                VkImageViewCreateInfo imageViewCreateInfo = {};
                imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewCreateInfo.image = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(imageResourceHandle)->image;
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

                VkResult result = vkCreateImageView(vulkanContextResources->device,
                    &imageViewCreateInfo,
                    nullptr,
                    newImageView);
                if (result != VK_SUCCESS)
                {
                    LogMsg("Failed to create Vulkan image view!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
                return newImageViewHandle;
            }

            uint32 CreateImageResource(VulkanContextResources* vulkanContextResources, uint32 width, uint32 height)
            {
                uint32 newResourceHandle = vulkanContextResources->vulkanMemResourcePool.Alloc();
                VulkanMemResource* newResource =
                    vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(newResourceHandle);

                VkImageCreateInfo imageCreateInfo = {};
                imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
                imageCreateInfo.extent.width = width;
                imageCreateInfo.extent.height = height;
                imageCreateInfo.extent.depth = 1;
                imageCreateInfo.mipLevels = 1;
                imageCreateInfo.arrayLayers = 1;
                imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageCreateInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
                imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

                VkResult result = vkCreateImage(vulkanContextResources->device,
                    &imageCreateInfo,
                    nullptr,
                    &newResource->image);
                
                VkMemoryRequirements memRequirements = {};
                vkGetImageMemoryRequirements(vulkanContextResources->device, newResource->image, &memRequirements);
                AllocGPUMemory(vulkanContextResources, &newResource->deviceMemory, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                vkBindImageMemory(vulkanContextResources->device, newResource->image, newResource->deviceMemory, 0);

                return newResourceHandle;
            }

            void DestroyImageResource(VulkanContextResources* vulkanContextResources, uint32 handle)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?
                VulkanMemResource* resource = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(handle);
                vkDestroyImage(vulkanContextResources->device, resource->image, nullptr);
                vkFreeMemory(vulkanContextResources->device, resource->deviceMemory, nullptr);
                vulkanContextResources->vulkanMemResourcePool.Dealloc(handle);
            }

            void DestroyImageViewResource(VulkanContextResources* vulkanContextResources, uint32 handle)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?
                vkDestroyImageView(vulkanContextResources->device, *vulkanContextResources->vulkanImageViewPool.PtrFromHandle(handle), nullptr);
                vulkanContextResources->vulkanImageViewPool.Dealloc(handle);
            }

            void DestroyFramebuffer(VulkanContextResources* vulkanContextResources, uint32 handle)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?
                VkFramebuffer* framebuffer = vulkanContextResources->vulkanFramebufferPool.PtrFromHandle(handle);
                vkDestroyFramebuffer(vulkanContextResources->device, *framebuffer, nullptr);
                vulkanContextResources->vulkanFramebufferPool.Dealloc(handle);
            }

            void DestroyVertexBuffer(VulkanContextResources* vulkanContextResources, uint32 handle)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?
                VulkanMemResource* resource = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(handle);
                vkDestroyBuffer(vulkanContextResources->device, resource->buffer, nullptr);
                vkFreeMemory(vulkanContextResources->device, resource->deviceMemory, nullptr);
                vulkanContextResources->vulkanMemResourcePool.Dealloc(handle);
            }

            void DestroyStagingBuffer(VulkanContextResources* vulkanContextResources, uint32 handle)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?
                VulkanMemResource* resource = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(handle);
                vkDestroyBuffer(vulkanContextResources->device, resource->buffer, nullptr);
                vkUnmapMemory(vulkanContextResources->device, resource->deviceMemory);
                vkFreeMemory(vulkanContextResources->device, resource->deviceMemory, nullptr);
                vulkanContextResources->vulkanMemResourcePool.Dealloc(handle);
            }

            void DestroyVulkan(VulkanContextResources* vulkanContextResources)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?

                DestroySwapChain(vulkanContextResources);

                vkDestroyCommandPool(vulkanContextResources->device, vulkanContextResources->commandPool, nullptr);
                delete vulkanContextResources->commandBuffers;

                vkDestroySemaphore(vulkanContextResources->device, vulkanContextResources->renderCompleteSemaphore, nullptr);
                vkDestroySemaphore(vulkanContextResources->device, vulkanContextResources->swapChainImageAvailableSemaphore, nullptr);
                vkDestroyFence(vulkanContextResources->device, vulkanContextResources->fence, nullptr);

                #ifdef _DEBUG
                // Debug utils messenger
                PFN_vkDestroyDebugUtilsMessengerEXT dbgDestroyFunc =
                    (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vulkanContextResources->instance,
                                                                                "vkDestroyDebugUtilsMessengerEXT");
                if (dbgDestroyFunc)
                {
                    dbgDestroyFunc(vulkanContextResources->instance, vulkanContextResources->debugMessenger, nullptr);
                }
                else
                {
                    LogMsg("Failed to get destroy debug utils messenger proc addr!", eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
                #endif

                vkDestroyDevice(vulkanContextResources->device, nullptr);
                vkDestroySurfaceKHR(vulkanContextResources->instance, vulkanContextResources->surface, nullptr);
                vkDestroyInstance(vulkanContextResources->instance, nullptr);
            }
        }
    }
}

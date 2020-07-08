#include "../Include/Platform/Win32Vulkan.h"
#include "../Include/Platform/Win32Utilities.h"

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

                VkResult result = vkCreateInstance(&instanceCreateInfo,
                    nullptr,
                    &vulkanContextResources->instance);
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
                    // TODO: Log? Fail?
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
                            // TODO: Log? Fail?
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

                result = vkCreateDevice(vulkanContextResources->physicalDevice,
                    &deviceCreateInfo,
                    nullptr,
                    &vulkanContextResources->device);
                if (result != VK_SUCCESS)
                {
                    // TODO: Log? Fail?
                    return 1;
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
                        CLAMP(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                    optimalExtent.height =
                        CLAMP(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
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
                    // TODO: Log? Fail?
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
                    // TODO: Log? Fail?
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

                result = vkCreateSwapchainKHR(vulkanContextResources->device,
                    &swapChainCreateInfo,
                    nullptr,
                    &vulkanContextResources->swapChain);
                if (result != VK_SUCCESS)
                {
                    // TODO: Log? Fail?
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

                    result = vkCreateImageView(vulkanContextResources->device,
                        &imageViewCreateInfo,
                        nullptr,
                        &vulkanContextResources->swapChainImageViews[uiImageView]);
                    if (result != VK_SUCCESS)
                    {
                        // TODO: Log? Fail?
                    }
                }

                InitRenderPassResources(vulkanContextResources);
                InitGraphicsPipelineResources(vulkanContextResources);

                // Swap chain framebuffers
                vulkanContextResources->swapChainFramebuffers = new VkFramebuffer[vulkanContextResources->numSwapChainImages];
                for (uint32 uiFramebuffer = 0; uiFramebuffer < vulkanContextResources->numSwapChainImages; ++uiFramebuffer)
                {
                    VkImageView attachments[] = {
                        vulkanContextResources->swapChainImageViews[uiFramebuffer]
                    };

                    VkFramebufferCreateInfo framebufferCreateInfo = {};
                    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    framebufferCreateInfo.renderPass = vulkanContextResources->renderPass;
                    framebufferCreateInfo.attachmentCount = 1;
                    framebufferCreateInfo.pAttachments = attachments;
                    framebufferCreateInfo.width = vulkanContextResources->swapChainExtent.width;
                    framebufferCreateInfo.height = vulkanContextResources->swapChainExtent.height;
                    framebufferCreateInfo.layers = 1;

                    result = vkCreateFramebuffer(vulkanContextResources->device,
                        &framebufferCreateInfo,
                        nullptr,
                        &vulkanContextResources->swapChainFramebuffers[uiFramebuffer]);
                    if (result != VK_SUCCESS)
                    {
                        // TODO: Fail? Log?
                    }
                }

                vulkanContextResources->vulkanBufferPool.Init(VULKAN_MAX_BUFFERS, 16);
                vulkanContextResources->vulkanDeviceMemoryPool.Init(VULKAN_MAX_BUFFERS, 16);
                vulkanContextResources->vulkanMappedMemPtrPool.Init(VULKAN_MAX_BUFFERS, 16);

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
                    // TODO: Fail? Log?
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
                    // TODO: Fail? Log?
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
                    // TODO: Fail? Log?
                }
                result = vkCreateSemaphore(vulkanContextResources->device,
                    &semaphoreCreateInfo,
                    nullptr,
                    &vulkanContextResources->renderCompleteSemaphore);
                if (result != VK_SUCCESS)
                {
                    // TODO: Fail? Log?
                }

                VkFenceCreateInfo fenceCreateInfo = {};
                fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                result = vkCreateFence(vulkanContextResources->device, &fenceCreateInfo, nullptr, &vulkanContextResources->fence);
                if (result != VK_SUCCESS)
                {
                    // TODO: Fail? Log?
                }

                return 0;
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
                    // TODO: Log? Fail?
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
                    // TODO: Fail? Log?
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
                    // TODO: Fail? Log?
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
                    // TODO: Log? Fail?
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
                    // TODO: Log? Fail?
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

                vkQueuePresentKHR(vulkanContextResources->presentationQueue, &presentInfo);
                vkQueueWaitIdle(vulkanContextResources->presentationQueue);
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
                    // TODO: Fail
                }

                VkMemoryRequirements memRequirements;
                vkGetBufferMemoryRequirements(vulkanContextResources->device, buffer, &memRequirements);

                // Check for memory type support
                VkPhysicalDeviceMemoryProperties memProperties;
                vkGetPhysicalDeviceMemoryProperties(vulkanContextResources->physicalDevice, &memProperties);

                uint32 memTypeIndex = 0xffffffff;
                for (uint32 uiMemType = 0; uiMemType < memProperties.memoryTypeCount; ++uiMemType)
                {
                    if (((1 << uiMemType) & memRequirements.memoryTypeBits) &&
                        (memProperties.memoryTypes[uiMemType].propertyFlags & propertyFlags) == propertyFlags)
                    {
                        memTypeIndex = uiMemType;
                    }
                }
                if (memTypeIndex == 0xffffffff)
                {
                    // TODO: Fail?
                }

                VkMemoryAllocateInfo memAllocInfo = {};
                memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                memAllocInfo.allocationSize = memRequirements.size;
                memAllocInfo.memoryTypeIndex = memTypeIndex;
                result = vkAllocateMemory(vulkanContextResources->device, &memAllocInfo, nullptr, &deviceMemory);
                if (result != VK_SUCCESS)
                {
                    // TODO: Fail
                }

                vkBindBufferMemory(vulkanContextResources->device, buffer, deviceMemory, 0);
            }

            uint32 CreateVertexBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes, BufferType bufferType)
            {
                uint32 newBufferHandle =
                    vulkanContextResources->vulkanBufferPool.Alloc();
                VkBuffer* newBuffer =
                    vulkanContextResources->vulkanBufferPool.PtrFromHandle(newBufferHandle);

                TINKER_ASSERT(newBufferHandle != 0xffffffff);

                uint32 newDeviceMemoryHandle =
                    vulkanContextResources->vulkanDeviceMemoryPool.Alloc();
                VkDeviceMemory* newDeviceMemory =
                    vulkanContextResources->vulkanDeviceMemoryPool.PtrFromHandle(newDeviceMemoryHandle);

                TINKER_ASSERT(newDeviceMemoryHandle != 0xffffffff);

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
                    *newBuffer,
                    *newDeviceMemory);
                
                return newBufferHandle;
            }

            VulkanStagingBufferData CreateStagingBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes)
            {
                uint32 newBufferHandle =
                    vulkanContextResources->vulkanBufferPool.Alloc();
                VkBuffer* newBuffer =
                    vulkanContextResources->vulkanBufferPool.PtrFromHandle(newBufferHandle);

                TINKER_ASSERT(newBufferHandle != 0xffffffff);

                uint32 newDeviceMemoryHandle =
                    vulkanContextResources->vulkanDeviceMemoryPool.Alloc();
                VkDeviceMemory* newDeviceMemory =
                    vulkanContextResources->vulkanDeviceMemoryPool.PtrFromHandle(newDeviceMemoryHandle);

                TINKER_ASSERT(newDeviceMemoryHandle != 0xffffffff);

                CreateBuffer(vulkanContextResources, sizeInBytes,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    *newBuffer,
                    *newDeviceMemory);

                uint32 newMappedMemHandle =
                    vulkanContextResources->vulkanMappedMemPtrPool.Alloc();
                void** newMappedMem =
                    vulkanContextResources->vulkanMappedMemPtrPool.PtrFromHandle(newMappedMemHandle);

                vkMapMemory(vulkanContextResources->device, *newDeviceMemory, 0, sizeInBytes, 0, newMappedMem);

                VulkanStagingBufferData data;
                data.handle = newBufferHandle;
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
                if (result != VK_SUCCESS)
                {
                    // TODO: Log?
                }
                vulkanContextResources->currentSwapChainImage = currentSwapChainImageIndex;

                VkCommandBufferBeginInfo commandBufferBeginInfo = {};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                commandBufferBeginInfo.flags = 0;
                commandBufferBeginInfo.pInheritanceInfo = nullptr;

                result = vkBeginCommandBuffer(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage], &commandBufferBeginInfo);
                if (result != VK_SUCCESS)
                {
                    // TODO: Fail? Log?
                }
            }

            void EndVulkanCommandRecording(VulkanContextResources* vulkanContextResources)
            {
                VkResult result = vkEndCommandBuffer(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage]);
                if (result != VK_SUCCESS)
                {
                    // TODO: Fail? Log?
                }
            }

            void VulkanRecordCommandDrawCall(VulkanContextResources* vulkanContextResources,
                uint32 vertexBufferHandle, uint32 indexBufferHandle,
                uint32 numIndices, uint32 numVertices)
            {
                VkBuffer vertexBuffers[] = { *vulkanContextResources->vulkanBufferPool.PtrFromHandle(vertexBufferHandle) };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage],
                    0, 1, vertexBuffers, offsets);

                VkBuffer& indexBuffer = *vulkanContextResources->vulkanBufferPool.PtrFromHandle(indexBufferHandle);
                vkCmdBindIndexBuffer(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage],
                    indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdDrawIndexed(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage],
                    numIndices, 1, 0, 0, 0);
            }

            void VulkanRecordCommandMemoryTransfer(VulkanContextResources* vulkanContextResources,
                uint32 sizeInBytes, uint32 srcBufferHandle, uint32 dstBufferHandle)
            {
                VkBuffer& dstBuffer = *vulkanContextResources->vulkanBufferPool.PtrFromHandle(dstBufferHandle);
                VkBuffer& srcBuffer = *vulkanContextResources->vulkanBufferPool.PtrFromHandle(srcBufferHandle);
                
                VkBufferCopy bufferCopy = {};
                bufferCopy.srcOffset = 0;
                bufferCopy.dstOffset = 0;
                bufferCopy.size = sizeInBytes;
                vkCmdCopyBuffer(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage],
                    srcBuffer,
                    dstBuffer,
                    1, &bufferCopy);
            }
            
            void VulkanRecordCommandRenderPassBegin(VulkanContextResources* vulkanContextResources)
            {
                VkRenderPassBeginInfo renderPassBeginInfo = {};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderPass = vulkanContextResources->renderPass;
                renderPassBeginInfo.framebuffer = vulkanContextResources->swapChainFramebuffers[vulkanContextResources->currentSwapChainImage];
                renderPassBeginInfo.renderArea.offset = { 0, 0 };
                renderPassBeginInfo.renderArea.extent = vulkanContextResources->swapChainExtent;

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
                vkCmdEndRenderPass(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage]);
            }

            void DestroyVertexBuffer(VulkanContextResources* vulkanContextResources, uint32 handle)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?
                vkDestroyBuffer(vulkanContextResources->device, *vulkanContextResources->vulkanBufferPool.PtrFromHandle(handle), nullptr);
                vkFreeMemory(vulkanContextResources->device, *vulkanContextResources->vulkanDeviceMemoryPool.PtrFromHandle(handle), nullptr);
                vulkanContextResources->vulkanBufferPool.Dealloc(handle);
                vulkanContextResources->vulkanDeviceMemoryPool.Dealloc(handle);
            }

            void DestroyStagingBuffer(VulkanContextResources* vulkanContextResources, uint32 handle)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?
                vkDestroyBuffer(vulkanContextResources->device, *vulkanContextResources->vulkanBufferPool.PtrFromHandle(handle), nullptr);
                vkUnmapMemory(vulkanContextResources->device, *vulkanContextResources->vulkanDeviceMemoryPool.PtrFromHandle(handle));
                vkFreeMemory(vulkanContextResources->device, *vulkanContextResources->vulkanDeviceMemoryPool.PtrFromHandle(handle), nullptr);
                vulkanContextResources->vulkanBufferPool.Dealloc(handle);
                vulkanContextResources->vulkanDeviceMemoryPool.Dealloc(handle);
            }

            void DestroyVulkan(VulkanContextResources* vulkanContextResources)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?

                vkDestroyPipeline(vulkanContextResources->device, vulkanContextResources->pipeline, nullptr);
                vkDestroyPipelineLayout(vulkanContextResources->device, vulkanContextResources->pipelineLayout, nullptr);
                vkDestroyRenderPass(vulkanContextResources->device, vulkanContextResources->renderPass, nullptr);

                for (uint32 uiFramebuffer = 0; uiFramebuffer < vulkanContextResources->numSwapChainImages; ++uiFramebuffer)
                {
                    vkDestroyFramebuffer(vulkanContextResources->device, vulkanContextResources->swapChainFramebuffers[uiFramebuffer], nullptr);
                }
                delete vulkanContextResources->swapChainFramebuffers;

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
        }
    }
}

#include "../../Include/Platform/Graphics/Vulkan.h"
#include "../../Include/PlatformGameAPI.h"
#include "../../Include/Core/Utilities/Logging.h"

#include <cstring>
#include <iostream>
// TODO: move this to be a compile define
#define ENABLE_VULKAN_VALIDATION_LAYERS // enables validation layers
#define ENABLE_VULKAN_DEBUG_LABELS // enables marking up vulkan objects/commands with debug labels

// NOTE: The convention in this project to is flip the viewport upside down since a left-handed projection
// matrix is often used. However, doing this flip causes the application to not render properly when run
// in RenderDoc for some reason. To debug with RenderDoc, you should turn on this #define.
#define WORK_WITH_RENDERDOC

namespace Tinker
{
    namespace Platform
    {
        namespace Graphics
        {
            // NOTE: Must correspond the blend state enum in PlatformGameAPI.h
            static VkPipelineColorBlendAttachmentState   VulkanBlendStates[eBlendStateMax] = {};
            static VkPipelineDepthStencilStateCreateInfo VulkanDepthStates[eDepthStateMax] = {};
            static VkImageLayout                         VulkanImageLayouts[eImageLayoutMax] = {};
            static VkFormat                              VulkanImageFormats[eImageFormatMax] = {};
            static VkDescriptorType                      VulkanDescriptorTypes[eDescriptorTypeMax] = {};

            const VkPipelineColorBlendAttachmentState& GetVkBlendState(uint32 gameBlendState)
            {
                TINKER_ASSERT(gameBlendState < eBlendStateMax);
                return VulkanBlendStates[gameBlendState];
            }

            const VkPipelineDepthStencilStateCreateInfo& GetVkDepthState(uint32 gameDepthState)
            {
                TINKER_ASSERT(gameDepthState < eDepthStateMax);
                return VulkanDepthStates[gameDepthState];
            }
            
            const VkImageLayout& GetVkImageLayout(uint32 gameImageLayout)
            {
                TINKER_ASSERT(gameImageLayout < eImageLayoutMax);
                return VulkanImageLayouts[gameImageLayout];
            }

            const VkFormat& GetVkImageFormat(uint32 gameImageFormat)
            {
                TINKER_ASSERT(gameImageFormat < eImageFormatMax);
                return VulkanImageFormats[gameImageFormat];
            }

            const VkDescriptorType& GetVkDescriptorType(uint32 gameDescriptorType)
            {
                TINKER_ASSERT(gameDescriptorType < eDescriptorTypeMax);
                return VulkanDescriptorTypes[gameDescriptorType];
            }

            #if defined(ENABLE_VULKAN_VALIDATION_LAYERS)
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
                const PlatformWindowHandles* platformWindowHandles,
                uint32 width, uint32 height, uint32 numThreads)
            {
                vulkanContextResources->vulkanMemResourcePool.Init(VULKAN_RESOURCE_POOL_MAX, 16);
                vulkanContextResources->vulkanPipelineResourcePool.Init(VULKAN_RESOURCE_POOL_MAX, 16);
                vulkanContextResources->vulkanDescriptorResourcePool.Init(VULKAN_RESOURCE_POOL_MAX, 16);
                vulkanContextResources->vulkanFramebufferResourcePool.Init(VULKAN_RESOURCE_POOL_MAX, 16);

                vulkanContextResources->windowWidth = width;
                vulkanContextResources->windowHeight = height;

                VulkanBlendStates[eBlendStateInvalid] = {};
                VkPipelineColorBlendAttachmentState blendStateProperties = {};
                blendStateProperties.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                    VK_COLOR_COMPONENT_G_BIT |
                    VK_COLOR_COMPONENT_B_BIT |
                    VK_COLOR_COMPONENT_A_BIT;
                blendStateProperties.blendEnable = VK_TRUE;
                blendStateProperties.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                blendStateProperties.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                blendStateProperties.colorBlendOp = VK_BLEND_OP_ADD;
                blendStateProperties.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                blendStateProperties.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                blendStateProperties.alphaBlendOp = VK_BLEND_OP_ADD;
                VulkanBlendStates[eBlendStateAlphaBlend] = blendStateProperties;

                VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
                depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // TODO: strictly less?
                depthStencilState.depthBoundsTestEnable = VK_FALSE;
                depthStencilState.minDepthBounds = 0.0f;
                depthStencilState.maxDepthBounds = 1.0f;
                depthStencilState.stencilTestEnable = VK_FALSE;
                depthStencilState.front = {};
                depthStencilState.back = {};

                depthStencilState.depthTestEnable = VK_FALSE;
                depthStencilState.depthWriteEnable = VK_FALSE;
                VulkanDepthStates[eDepthStateOff] = depthStencilState;
                depthStencilState.depthTestEnable = VK_TRUE;
                depthStencilState.depthWriteEnable = VK_TRUE;
                VulkanDepthStates[eDepthStateTestOnWriteOn] = depthStencilState;
                depthStencilState.depthWriteEnable = VK_FALSE;
                VulkanDepthStates[eDepthStateTestOnWriteOff] = depthStencilState;

                VulkanImageLayouts[eImageLayoutUndefined] = VK_IMAGE_LAYOUT_UNDEFINED;
                VulkanImageLayouts[eImageLayoutShaderRead] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                VulkanImageLayouts[eImageLayoutTransferDst] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                VulkanImageLayouts[eImageLayoutDepthOptimal] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VulkanImageFormats[eImageFormat_BGRA8_SRGB] = VK_FORMAT_B8G8R8A8_SRGB;
                VulkanImageFormats[eImageFormat_RGBA8_SRGB] = VK_FORMAT_R8G8B8A8_SRGB;
                VulkanImageFormats[eImageFormat_Depth_32F] = VK_FORMAT_D32_SFLOAT;
                VulkanImageFormats[eImageFormat_Invalid] = VK_FORMAT_UNDEFINED;

                VulkanDescriptorTypes[eDescriptorTypeBuffer] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                VulkanDescriptorTypes[eDescriptorTypeSampledImage] = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

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

                const uint32 numRequestedExtensions = 
                #if defined(ENABLE_VULKAN_VALIDATION_LAYERS) || defined(ENABLE_VULKAN_DEBUG_LABELS)
                3
                #else
                2
                #endif
                ;
                const char* requestedExtensionNames[numRequestedExtensions] = {
                    VK_KHR_SURFACE_EXTENSION_NAME,

                    #if defined(_WIN32)
                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME
                    #else
                    // TODO: different platform surface extension
                    #endif

                    
                    #if defined(ENABLE_VULKAN_VALIDATION_LAYERS) || defined(ENABLE_VULKAN_DEBUG_LABELS)
                    , VK_EXT_DEBUG_UTILS_EXTENSION_NAME
                    #endif
                };
                Core::Utility::LogMsg("Platform", "******** Requested Instance Extensions: ********", Core::Utility::eLogSeverityInfo);
                for (uint32 uiReqExt = 0; uiReqExt < numRequestedExtensions; ++uiReqExt)
                {
                    Core::Utility::LogMsg("Platform", requestedExtensionNames[uiReqExt], Core::Utility::eLogSeverityInfo);
                }
                
                instanceCreateInfo.enabledExtensionCount = numRequestedExtensions;
                instanceCreateInfo.ppEnabledExtensionNames = requestedExtensionNames;

                instanceCreateInfo.enabledLayerCount = 0;
                instanceCreateInfo.ppEnabledLayerNames = nullptr;
                
                // Get the available extensions
                uint32 numAvailableExtensions = 0;
                vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions, nullptr);
                
                VkExtensionProperties* availableExtensions = new VkExtensionProperties[numAvailableExtensions];
                vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions, availableExtensions);
                Core::Utility::LogMsg("Platform", "******** Available Instance Extensions: ********", Core::Utility::eLogSeverityInfo);
                for (uint32 uiAvailExt = 0; uiAvailExt < numAvailableExtensions; ++uiAvailExt)
                {
                    Core::Utility::LogMsg("Platform", availableExtensions[uiAvailExt].extensionName, Core::Utility::eLogSeverityInfo);
                }
                delete availableExtensions;

                // Validation layers
                #if defined(ENABLE_VULKAN_VALIDATION_LAYERS)
                const uint32 numRequestedLayers = 1;
                const char* requestedLayersStr[numRequestedLayers] =
                {
                    "VK_LAYER_KHRONOS_validation"
                };
                Core::Utility::LogMsg("Platform", "******** Requested Instance Layers: ********", Core::Utility::eLogSeverityInfo);
                for (uint32 uiReqLayer = 0; uiReqLayer < numRequestedLayers; ++uiReqLayer)
                {
                    Core::Utility::LogMsg("Platform", requestedLayersStr[uiReqLayer], Core::Utility::eLogSeverityInfo);
                }

                uint32 numAvailableLayers = 0;
                vkEnumerateInstanceLayerProperties(&numAvailableLayers, nullptr);

                if (numAvailableLayers == 0)
                {
                    Core::Utility::LogMsg("Platform", "Zero available instance layers!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkLayerProperties* availableLayers = new VkLayerProperties[numAvailableLayers];
                vkEnumerateInstanceLayerProperties(&numAvailableLayers, availableLayers);

                Core::Utility::LogMsg("Platform", "******** Available Instance Layers: ********", Core::Utility::eLogSeverityInfo);
                for (uint32 uiAvailLayer = 0; uiAvailLayer < numAvailableLayers; ++uiAvailLayer)
                {
                    Core::Utility::LogMsg("Platform", availableLayers[uiAvailLayer].layerName, Core::Utility::eLogSeverityInfo);
                }

                bool layersSupported[numRequestedLayers] = { false };
                for (uint32 uiReqLayer = 0; uiReqLayer < numRequestedLayers; ++uiReqLayer)
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

                for (uint32 uiReqLayer = 0; uiReqLayer < numRequestedLayers; ++uiReqLayer)
                {
                    if (!layersSupported[uiReqLayer])
                    {
                        Core::Utility::LogMsg("Platform", "Requested instance layer not supported!", Core::Utility::eLogSeverityCritical);
                        Core::Utility::LogMsg("Platform", requestedLayersStr[uiReqLayer], Core::Utility::eLogSeverityCritical);
                        TINKER_ASSERT(0);
                    }
                }

                instanceCreateInfo.enabledLayerCount = numRequestedLayers;
                instanceCreateInfo.ppEnabledLayerNames = requestedLayersStr;
                #endif

                VkResult result = vkCreateInstance(&instanceCreateInfo,
                    nullptr,
                    &vulkanContextResources->instance);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to create Vulkan instance!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                #if defined(ENABLE_VULKAN_VALIDATION_LAYERS)
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
                    Core::Utility::LogMsg("Platform", "Failed to get create debug utils messenger proc addr!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
                #endif

                // Surface
                #if defined(_WIN32)
                VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {};
                win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
                win32SurfaceCreateInfo.hinstance = platformWindowHandles->instance;
                win32SurfaceCreateInfo.hwnd = platformWindowHandles->windowHandle;

                result = vkCreateWin32SurfaceKHR(vulkanContextResources->instance,
                    &win32SurfaceCreateInfo,
                    NULL,
                    &vulkanContextResources->surface);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to create Win32SurfaceKHR!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
                #else
                // TODO: implement other platform surface types
                TINKER_ASSERT(0);
                #endif

                // Physical device
                uint32 numPhysicalDevices = 0;
                vkEnumeratePhysicalDevices(vulkanContextResources->instance, &numPhysicalDevices, nullptr);

                if (numPhysicalDevices == 0)
                {
                    Core::Utility::LogMsg("Platform", "Zero available physical devices!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[numPhysicalDevices];
                vkEnumeratePhysicalDevices(vulkanContextResources->instance, &numPhysicalDevices, physicalDevices);

                const uint32 numRequiredPhysicalDeviceExtensions = 1;
                const char* requiredPhysicalDeviceExtensions[numRequiredPhysicalDeviceExtensions] =
                {
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME
                };
                Core::Utility::LogMsg("Platform", "******** Requested Device Extensions: ********", Core::Utility::eLogSeverityInfo);
                for (uint32 uiReqExt = 0; uiReqExt < numRequiredPhysicalDeviceExtensions; ++uiReqExt)
                {
                    Core::Utility::LogMsg("Platform", requiredPhysicalDeviceExtensions[uiReqExt], Core::Utility::eLogSeverityInfo);
                }

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
                        delete queueFamilyProperties;

                        uint32 numAvailablePhysicalDeviceExtensions = 0;
                        vkEnumerateDeviceExtensionProperties(currPhysicalDevice,
                            nullptr,
                            &numAvailablePhysicalDeviceExtensions,
                            nullptr);

                        if (numAvailablePhysicalDeviceExtensions == 0)
                        {
                            Core::Utility::LogMsg("Platform", "Zero available device extensions!", Core::Utility::eLogSeverityCritical);
                            TINKER_ASSERT(0);
                        }

                        VkExtensionProperties* availablePhysicalDeviceExtensions = new VkExtensionProperties[numAvailablePhysicalDeviceExtensions];
                        vkEnumerateDeviceExtensionProperties(currPhysicalDevice,
                            nullptr,
                            &numAvailablePhysicalDeviceExtensions,
                            availablePhysicalDeviceExtensions);

                        Core::Utility::LogMsg("Platform", "******** Available Device Extensions: ********", Core::Utility::eLogSeverityInfo);
                        for (uint32 uiAvailExt = 0; uiAvailExt < numAvailablePhysicalDeviceExtensions; ++uiAvailExt)
                        {
                            Core::Utility::LogMsg("Platform", availablePhysicalDeviceExtensions[uiAvailExt].extensionName, Core::Utility::eLogSeverityInfo);
                        }

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
                                Core::Utility::LogMsg("Platform", "Requested device extension not supported!", Core::Utility::eLogSeverityCritical);
                                Core::Utility::LogMsg("Platform", requiredPhysicalDeviceExtensions[uiReqExt], Core::Utility::eLogSeverityCritical);
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
                    }
                }
                delete physicalDevices;

                if (vulkanContextResources->physicalDevice == VK_NULL_HANDLE)
                {
                    Core::Utility::LogMsg("Platform", "No physical device chosen!", Core::Utility::eLogSeverityCritical);
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
                    Core::Utility::LogMsg("Platform", "Failed to create Vulkan device!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                // Debug labels
                #if defined(ENABLE_VULKAN_DEBUG_LABELS)
                /*vulkanContextResources->pfnSetDebugUtilsObjectNameEXT =
                    (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(vulkanContextResources->device,
                                                                          "vkSetDebugUtilsObjectNameEXT")*/;
                vulkanContextResources->pfnCmdBeginDebugUtilsLabelEXT =
                    (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(vulkanContextResources->device,
                                                                           "vkCmdBeginDebugUtilsLabelEXT");
                if (!vulkanContextResources->pfnCmdBeginDebugUtilsLabelEXT)
                {
                    Core::Utility::LogMsg("Platform", "Failed to get create debug utils begin label proc addr!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                vulkanContextResources->pfnCmdEndDebugUtilsLabelEXT =
                    (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(vulkanContextResources->device,
                                                                           "vkCmdEndDebugUtilsLabelEXT");
                if (!vulkanContextResources->pfnCmdEndDebugUtilsLabelEXT)
                {
                    Core::Utility::LogMsg("Platform", "Failed to get create debug utils end label proc addr!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                vulkanContextResources->pfnCmdInsertDebugUtilsLabelEXT =
                    (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetDeviceProcAddr(vulkanContextResources->device,
                                                                           "vkCmdInsertDebugUtilsLabelEXT");
                if (!vulkanContextResources->pfnCmdInsertDebugUtilsLabelEXT)
                {
                    Core::Utility::LogMsg("Platform", "Failed to get create debug utils insert label proc addr!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
                #endif

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
                VulkanCreateSwapChain(vulkanContextResources);

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
                    Core::Utility::LogMsg("Platform", "Failed to create Vulkan command pool!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                // Command buffers for per-frame submission
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
                    Core::Utility::LogMsg("Platform", "Failed to allocate Vulkan primary frame command buffers!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                // Secondary command buffers for parallelizing command recording
                // These get executed into the primary command buffers just allocated above
                vulkanContextResources->threaded_secondaryCommandBuffers = new VkCommandBuffer[numThreads];

                commandBufferAllocInfo = {};
                commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                commandBufferAllocInfo.commandPool = vulkanContextResources->commandPool;
                commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
                commandBufferAllocInfo.commandBufferCount = numThreads;

                result = vkAllocateCommandBuffers(vulkanContextResources->device,
                    &commandBufferAllocInfo,
                    vulkanContextResources->threaded_secondaryCommandBuffers);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to allocate Vulkan secondary frame command buffers!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                // Command buffer for immediate submission
                commandBufferAllocInfo = {};
                commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                commandBufferAllocInfo.commandPool = vulkanContextResources->commandPool;
                commandBufferAllocInfo.commandBufferCount = 1;
                result = vkAllocateCommandBuffers(vulkanContextResources->device, &commandBufferAllocInfo, &vulkanContextResources->commandBuffer_Immediate);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to allocate Vulkan command buffers (immediate)!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                // Semaphores
                VkSemaphoreCreateInfo semaphoreCreateInfo = {};
                semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

                for (uint32 uiImage = 0; uiImage < VULKAN_MAX_FRAMES_IN_FLIGHT; ++uiImage)
                {
                    result = vkCreateSemaphore(vulkanContextResources->device,
                        &semaphoreCreateInfo,
                        nullptr,
                        &vulkanContextResources->swapChainImageAvailableSemaphores[uiImage]);
                    if (result != VK_SUCCESS)
                    {
                        Core::Utility::LogMsg("Platform", "Failed to create Vulkan semaphore!", Core::Utility::eLogSeverityCritical);
                    }

                    result = vkCreateSemaphore(vulkanContextResources->device,
                        &semaphoreCreateInfo,
                        nullptr,
                        &vulkanContextResources->renderCompleteSemaphores[uiImage]);
                    if (result != VK_SUCCESS)
                    {
                        Core::Utility::LogMsg("Platform", "Failed to create Vulkan semaphore!", Core::Utility::eLogSeverityCritical);
                    }
                }

                VkFenceCreateInfo fenceCreateInfo = {};
                fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                for (uint32 uiImage = 0; uiImage < VULKAN_MAX_FRAMES_IN_FLIGHT; ++uiImage)
                {
                    result = vkCreateFence(vulkanContextResources->device, &fenceCreateInfo, nullptr, &vulkanContextResources->fences[uiImage]);
                    if (result != VK_SUCCESS)
                    {
                        Core::Utility::LogMsg("Platform", "Failed to create Vulkan fence!", Core::Utility::eLogSeverityCritical);
                    }
                }
                
                vulkanContextResources->imageInFlightFences = new VkFence[vulkanContextResources->numSwapChainImages];
                for (uint32 uiImage = 0; uiImage < vulkanContextResources->numSwapChainImages; ++uiImage)
                {
                    vulkanContextResources->imageInFlightFences[uiImage] = VK_NULL_HANDLE;
                }

                CreateSamplers(vulkanContextResources);

                vulkanContextResources->isInitted = true;

                return 0;
            }

            void VulkanCreateSwapChain(VulkanContextResources* vulkanContextResources)
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
                    numSwapChainImages = min(numSwapChainImages, capabilities.maxImageCount);
                }
                numSwapChainImages = min(numSwapChainImages, VULKAN_MAX_SWAP_CHAIN_IMAGES);

                uint32 numAvailableSurfaceFormats;
                vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContextResources->physicalDevice,
                    vulkanContextResources->surface,
                    &numAvailableSurfaceFormats,
                    nullptr);

                if (numAvailableSurfaceFormats == 0)
                {
                    Core::Utility::LogMsg("Platform", "Zero available Vulkan swap chain surface formats!", Core::Utility::eLogSeverityCritical);
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
                    Core::Utility::LogMsg("Platform", "Zero available Vulkan swap chain present modes!", Core::Utility::eLogSeverityCritical);
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
                    if (availablePresentModes[uiAvailPresMode] == VK_PRESENT_MODE_IMMEDIATE_KHR/*VK_PRESENT_MODE_MAILBOX_KHR*/)
                    {
                        chosenPresentMode = availablePresentModes[uiAvailPresMode];
                    }
                }
                delete availablePresentModes;

                vulkanContextResources->swapChainExtent = optimalExtent;
                vulkanContextResources->swapChainFormat = chosenFormat.format;
                vulkanContextResources->numSwapChainImages = numSwapChainImages;

                VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
                swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                swapChainCreateInfo.surface = vulkanContextResources->surface;
                swapChainCreateInfo.minImageCount = numSwapChainImages;
                swapChainCreateInfo.imageFormat = chosenFormat.format;
                swapChainCreateInfo.imageColorSpace = chosenFormat.colorSpace;
                swapChainCreateInfo.imageExtent = optimalExtent;
                swapChainCreateInfo.imageArrayLayers = 1;
                swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                swapChainCreateInfo.preTransform = capabilities.currentTransform;
                swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                swapChainCreateInfo.presentMode = chosenPresentMode;

                const uint32 numQueueFamilyIndices = 2;
                const uint32 queueFamilyIndices[numQueueFamilyIndices] =
                {
                    vulkanContextResources->graphicsQueueIndex,
                    vulkanContextResources->presentationQueueIndex
                };

                if (vulkanContextResources->graphicsQueueIndex != vulkanContextResources->presentationQueueIndex)
                {
                    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                    swapChainCreateInfo.queueFamilyIndexCount = numQueueFamilyIndices;
                    swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
                }
                else
                {
                    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    swapChainCreateInfo.queueFamilyIndexCount = 0;
                    swapChainCreateInfo.pQueueFamilyIndices = nullptr;
                }

                VkResult result = vkCreateSwapchainKHR(vulkanContextResources->device,
                    &swapChainCreateInfo,
                    nullptr,
                    &vulkanContextResources->swapChain);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to create Vulkan swap chain!", Core::Utility::eLogSeverityCritical);
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

                vulkanContextResources->swapChainImageViews = new VkImageView[numSwapChainImages];
                for (uint32 uiImageView = 0; uiImageView < numSwapChainImages; ++uiImageView)
                {
                    CreateImageView(vulkanContextResources,
                                    vulkanContextResources->swapChainImages[uiImageView],
                                    &vulkanContextResources->swapChainImageViews[uiImageView]);
                }

                // Swap chain framebuffers
                uint32 newFramebufferHandle = vulkanContextResources->vulkanFramebufferResourcePool.Alloc();
                TINKER_ASSERT(newFramebufferHandle != TINKER_INVALID_HANDLE);
                vulkanContextResources->swapChainFramebufferHandle = FramebufferHandle(newFramebufferHandle);

                for (uint32 uiFramebuffer = 0; uiFramebuffer < vulkanContextResources->numSwapChainImages; ++uiFramebuffer)
                {
                    VulkanFramebufferResource* newFramebuffer =
                        &vulkanContextResources->vulkanFramebufferResourcePool.PtrFromHandle(newFramebufferHandle)->resourceChain[uiFramebuffer];

                    // Render pass
                    const uint32 numColorRTs = 1; // TODO: support multiple RTs
                    if (uiFramebuffer == 0)
                    {
                        const VkFormat depthFormat = VK_FORMAT_UNDEFINED; // no depth buffer for swap chain
                        CreateRenderPass(vulkanContextResources, numColorRTs, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, depthFormat, &newFramebuffer->renderPass);
                    }
                    else
                    {
                        newFramebuffer->renderPass = vulkanContextResources->vulkanFramebufferResourcePool.PtrFromHandle(newFramebufferHandle)->resourceChain[0].renderPass;
                    }
                    
                    for (uint32 i = 0; i < numColorRTs; ++i)
                    {
                        newFramebuffer->clearValues[i] = { 0.0f, 0.0f, 0.0f, 1.0f };
                    }
                    newFramebuffer->numClearValues = numColorRTs;

                    // Framebuffer
                    const VkImageView depthImageView = VK_NULL_HANDLE; // no depth buffer for swap chain
                    CreateFramebuffer(vulkanContextResources, &vulkanContextResources->swapChainImageViews[uiFramebuffer], 1, depthImageView,
                        vulkanContextResources->swapChainExtent.width, vulkanContextResources->swapChainExtent.height, newFramebuffer->renderPass, &newFramebuffer->framebuffer);
                }

                vulkanContextResources->isSwapChainValid = true;
            }

            void VulkanDestroySwapChain(VulkanContextResources* vulkanContextResources)
            {
                vulkanContextResources->isSwapChainValid = false;
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?

                VulkanDestroyFramebuffer(vulkanContextResources, vulkanContextResources->swapChainFramebufferHandle);

                for (uint32 uiImageView = 0; uiImageView < vulkanContextResources->numSwapChainImages; ++uiImageView)
                {
                    vkDestroyImageView(vulkanContextResources->device, vulkanContextResources->swapChainImageViews[uiImageView], nullptr);
                }
                delete vulkanContextResources->swapChainImageViews;
                delete vulkanContextResources->swapChainImages;
                
                vkDestroySwapchainKHR(vulkanContextResources->device, vulkanContextResources->swapChain, nullptr);
            }

            void CreateImageView(VulkanContextResources* vulkanContextResources, VkImage image, VkImageView* imageView)
            {
                VkImageViewCreateInfo imageViewCreateInfo = {};
                imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewCreateInfo.image = image;
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
                    imageView);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to create Vulkan image view!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
            }

            void CreateFramebuffer(VulkanContextResources* vulkanContextResources, VkImageView* colorRTs, uint32 numColorRTs, VkImageView depthRT,
                uint32 width, uint32 height, VkRenderPass renderPass, VkFramebuffer* framebuffer)
            {
                VkImageView* attachments = nullptr;
                uint32 numAttachments = numColorRTs + (depthRT != VK_NULL_HANDLE ? 1 : 0);

                if (numAttachments == 0)
                {
                    Core::Utility::LogMsg("Platform", "No attachments specified for framebuffer!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
                else
                {
                    attachments = new VkImageView[numAttachments];
                }

                if (attachments)
                {
                    memcpy(attachments, colorRTs, sizeof(VkImageView) * numColorRTs); // memcpy the color render targets in

                    if (depthRT != VK_NULL_HANDLE)
                    {
                        attachments[numAttachments - 1] = depthRT;
                    }
                }

                VkFramebufferCreateInfo framebufferCreateInfo = {};
                framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferCreateInfo.renderPass = renderPass;
                framebufferCreateInfo.attachmentCount = numAttachments;
                framebufferCreateInfo.pAttachments = attachments;
                framebufferCreateInfo.width = width;
                framebufferCreateInfo.height = height;
                framebufferCreateInfo.layers = 1;

                VkResult result = vkCreateFramebuffer(vulkanContextResources->device,
                    &framebufferCreateInfo,
                    nullptr,
                    framebuffer);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to create Vulkan framebuffer!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                delete attachments;
            }

            void CreateRenderPass(VulkanContextResources* vulkanContextResources, uint32 numColorAttachments, VkFormat colorFormat,
                VkImageLayout startLayout, VkImageLayout endLayout, VkFormat depthFormat, VkRenderPass* renderPass)
            {
                VkAttachmentDescription attachments[VULKAN_MAX_RENDERTARGETS_WITH_DEPTH] = {};
                VkAttachmentReference colorAttachmentRefs[VULKAN_MAX_RENDERTARGETS] = {};
                VkAttachmentReference depthAttachmentRef = {};

                for (uint32 uiRT = 0; uiRT < numColorAttachments; ++uiRT)
                {
                    attachments[uiRT] = {};
                    attachments[uiRT].format = colorFormat;
                    attachments[uiRT].samples = VK_SAMPLE_COUNT_1_BIT;
                    attachments[uiRT].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    attachments[uiRT].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                    attachments[uiRT].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    attachments[uiRT].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    attachments[uiRT].initialLayout = startLayout;
                    attachments[uiRT].finalLayout = endLayout;

                    colorAttachmentRefs[uiRT] = {};
                    colorAttachmentRefs[uiRT].attachment = uiRT;
                    colorAttachmentRefs[uiRT].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }

                bool hasDepthAttachment = depthFormat != VK_FORMAT_UNDEFINED;

                attachments[numColorAttachments].format = depthFormat;
                attachments[numColorAttachments].samples = VK_SAMPLE_COUNT_1_BIT;
                attachments[numColorAttachments].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachments[numColorAttachments].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[numColorAttachments].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[numColorAttachments].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments[numColorAttachments].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                attachments[numColorAttachments].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                depthAttachmentRef.attachment = numColorAttachments;
                depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                // Subpass
                VkSubpassDescription subpass = {};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = numColorAttachments;
                subpass.pColorAttachments = colorAttachmentRefs;
                subpass.pDepthStencilAttachment = hasDepthAttachment ? &depthAttachmentRef : nullptr;

                const uint32 numSubpassDependencies = 2;
                VkSubpassDependency subpassDependencies[numSubpassDependencies] = {};
                subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                subpassDependencies[0].srcAccessMask = 0;
                subpassDependencies[0].dstSubpass = 0;
                subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | (hasDepthAttachment ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);

                subpassDependencies[1].srcSubpass = 0;
                subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | (hasDepthAttachment ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
                subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
                subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                subpassDependencies[1].dstAccessMask = 0;
                // TODO: check if these subpass dependencies are correct at some point

                VkRenderPassCreateInfo renderPassCreateInfo = {};
                renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                renderPassCreateInfo.attachmentCount = hasDepthAttachment ? numColorAttachments + 1 : numColorAttachments;
                renderPassCreateInfo.pAttachments = attachments;
                renderPassCreateInfo.subpassCount = 1;
                renderPassCreateInfo.pSubpasses = &subpass;
                renderPassCreateInfo.dependencyCount = numSubpassDependencies;
                renderPassCreateInfo.pDependencies = subpassDependencies;

                VkResult result = vkCreateRenderPass(vulkanContextResources->device, &renderPassCreateInfo, nullptr, renderPass);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to create Vulkan render pass!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                /*#if defined(ENABLE_VULKAN_DEBUG_LABELS)
                const VkDebugUtilsObjectNameInfoEXT debugNameInfo =
                {
                    VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                    NULL,
                    VK_OBJECT_TYPE_RENDER_PASS,
                    (uint64_t)*newRenderPass,
                    "Joe's Render Pass!!!",
                };
                vulkanContextResources->pfnSetDebugUtilsObjectNameEXT(vulkanContextResources->device, &debugNameInfo);
                #endif*/
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
                    Core::Utility::LogMsg("Platform", "Failed to create Vulkan shader module!", Core::Utility::eLogSeverityCritical);
                    return VK_NULL_HANDLE;
                }
                return shaderModule;
            }

            ShaderHandle VulkanCreateGraphicsPipeline(VulkanContextResources* vulkanContextResources,
                void* vertexShaderCode, uint32 numVertexShaderBytes,
                void* fragmentShaderCode, uint32 numFragmentShaderBytes,
                uint32 blendState, uint32 depthState, uint32 viewportWidth, uint32 viewportHeight,
                FramebufferHandle framebufferHandle, DescriptorHandle descriptorHandle)
            {
                VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
                VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;

                uint32 numStages = 0;
                if (numVertexShaderBytes > 0)
                {
                    vertexShaderModule = CreateShaderModule((const char*)vertexShaderCode, numVertexShaderBytes, vulkanContextResources);
                    ++numStages;
                }
                if (numFragmentShaderBytes > 0)
                {
                    fragmentShaderModule = CreateShaderModule((const char*)fragmentShaderCode, numFragmentShaderBytes, vulkanContextResources);
                    ++numStages;
                }
                
                // Programmable shader stages
                const uint32 maxNumStages = 2;
                VkPipelineShaderStageCreateInfo shaderStages[maxNumStages] = {};
                shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
                shaderStages[0].module = vertexShaderModule;
                shaderStages[0].pName = "main";

                shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                shaderStages[1].module = fragmentShaderModule;
                shaderStages[1].pName = "main";

                // Fixed function
                const uint32 numBindings = 3;
                const uint32 vertPositionBinding = 0;
                const uint32 vertUVBinding = 1;
                const uint32 vertNormalBinding = 2;
                VkVertexInputBindingDescription vertexInputBindDescs[numBindings] = {};
                vertexInputBindDescs[0].binding = vertPositionBinding;
                vertexInputBindDescs[0].stride = sizeof(VulkanVertexPosition);
                vertexInputBindDescs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                vertexInputBindDescs[1].binding = vertUVBinding;
                vertexInputBindDescs[1].stride = sizeof(VulkanVertexUV);
                vertexInputBindDescs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                vertexInputBindDescs[2].binding = vertNormalBinding;
                vertexInputBindDescs[2].stride = sizeof(VulkanVertexNormal);
                vertexInputBindDescs[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                const uint32 numAttributes = 3;
                uint32 locationCounter = 0;
                VkVertexInputAttributeDescription vertexInputAttrDescs[numAttributes] = {};
                vertexInputAttrDescs[0].binding = vertPositionBinding;
                vertexInputAttrDescs[0].location = locationCounter++;
                vertexInputAttrDescs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
                vertexInputAttrDescs[0].offset = 0;

                vertexInputAttrDescs[1].binding = vertUVBinding;
                vertexInputAttrDescs[1].location = locationCounter++;
                vertexInputAttrDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
                vertexInputAttrDescs[1].offset = 0;

                vertexInputAttrDescs[2].binding = vertNormalBinding;
                vertexInputAttrDescs[2].location = locationCounter++;
                vertexInputAttrDescs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertexInputAttrDescs[2].offset = 0;

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

                #ifdef WORK_WITH_RENDERDOC
                viewport.y = 0.0f;
                viewport.height = (float)vulkanContextResources->swapChainExtent.height;
                #else
                viewport.y = (float)vulkanContextResources->swapChainExtent.height;
                viewport.height = -(float)vulkanContextResources->swapChainExtent.height;
                #endif

                viewport.x = 0.0f;
                viewport.width = (float)vulkanContextResources->swapChainExtent.width;
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
                #ifdef WORK_WITH_RENDERDOC
                rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
                #else
                rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                #endif
                rasterizer.depthBiasEnable = VK_FALSE;
                rasterizer.depthBiasConstantFactor = 0.0f;
                rasterizer.depthBiasClamp = 0.0f;
                rasterizer.depthBiasSlopeFactor = 0.0f;

                VkPipelineDepthStencilStateCreateInfo depthStencilState = GetVkDepthState(depthState);

                VkPipelineMultisampleStateCreateInfo multisampling = {};
                multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampling.sampleShadingEnable = VK_FALSE;
                multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
                multisampling.minSampleShading = 1.0f;
                multisampling.pSampleMask = nullptr;
                multisampling.alphaToCoverageEnable = VK_FALSE;
                multisampling.alphaToOneEnable = VK_FALSE;

                VkPipelineColorBlendStateCreateInfo colorBlending = {};
                colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlending.logicOpEnable = VK_FALSE;
                colorBlending.logicOp = VK_LOGIC_OP_COPY;
                colorBlending.blendConstants[0] = 0.0f;
                colorBlending.blendConstants[1] = 0.0f;
                colorBlending.blendConstants[2] = 0.0f;
                colorBlending.blendConstants[3] = 0.0f;

                VkPipelineColorBlendAttachmentState colorBlendAttachment = GetVkBlendState(blendState);

                if (blendState == eBlendStateInvalid)
                {
                    colorBlending.attachmentCount = 0;
                    colorBlending.pAttachments = nullptr;
                }
                else
                {
                    // TODO: support multiple RTs here too
                    colorBlending.attachmentCount = 1;
                    colorBlending.pAttachments = &colorBlendAttachment;
                }

                VkDynamicState dynamicStates[] =
                {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_LINE_WIDTH
                };

                VkPipelineDynamicStateCreateInfo dynamicState = {};
                dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicState.dynamicStateCount = 2;
                dynamicState.pDynamicStates = dynamicStates;

                uint32 newPipelineHandle = vulkanContextResources->vulkanPipelineResourcePool.Alloc();

                VkDescriptorSetLayout* descriptorSetLayout = nullptr;
                if (descriptorHandle != DefaultDescHandle_Invalid)
                {
                    descriptorSetLayout = &vulkanContextResources->vulkanDescriptorResourcePool.PtrFromHandle(descriptorHandle.m_hDesc)->resourceChain[0].descriptorLayout;
                }

                VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = descriptorSetLayout ? 1 : 0;
                pipelineLayoutInfo.pSetLayouts = descriptorSetLayout;
                pipelineLayoutInfo.pushConstantRangeCount = 0;
                pipelineLayoutInfo.pPushConstantRanges = nullptr;

                VkPipelineLayout* pipelineLayout = &vulkanContextResources->vulkanPipelineResourcePool.PtrFromHandle(newPipelineHandle)->pipelineLayout;

                VkResult result = vkCreatePipelineLayout(vulkanContextResources->device,
                    &pipelineLayoutInfo,
                    nullptr,
                    pipelineLayout);

                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to create Vulkan pipeline layout!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
                pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                pipelineCreateInfo.stageCount = numStages;
                pipelineCreateInfo.pStages = shaderStages;
                pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
                pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
                pipelineCreateInfo.pViewportState = &viewportState;
                pipelineCreateInfo.pRasterizationState = &rasterizer;
                pipelineCreateInfo.pMultisampleState = &multisampling;
                pipelineCreateInfo.pDepthStencilState = &depthStencilState;
                pipelineCreateInfo.pColorBlendState = &colorBlending;
                pipelineCreateInfo.pDynamicState = nullptr;
                pipelineCreateInfo.layout = *pipelineLayout;

                if (framebufferHandle == DefaultFramebufferHandle_Invalid)
                {
                    pipelineCreateInfo.renderPass =
                        vulkanContextResources->vulkanFramebufferResourcePool.PtrFromHandle(vulkanContextResources->swapChainFramebufferHandle.m_hFramebuffer)->resourceChain[0].renderPass; // All swap chain images have the same render pass data
                }
                else
                {
                    pipelineCreateInfo.renderPass =
                        vulkanContextResources->vulkanFramebufferResourcePool.PtrFromHandle(framebufferHandle.m_hFramebuffer)->resourceChain[0].renderPass; // All swap chain images have the same render pass data
                }
                pipelineCreateInfo.subpass = 0;
                pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
                pipelineCreateInfo.basePipelineIndex = -1;

                result = vkCreateGraphicsPipelines(vulkanContextResources->device,
                    VK_NULL_HANDLE,
                    1,
                    &pipelineCreateInfo,
                    nullptr,
                    &vulkanContextResources->vulkanPipelineResourcePool.PtrFromHandle(newPipelineHandle)->graphicsPipeline);

                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to create Vulkan graphics pipeline!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                vkDestroyShaderModule(vulkanContextResources->device, vertexShaderModule, nullptr);
                vkDestroyShaderModule(vulkanContextResources->device, fragmentShaderModule, nullptr);

                return ShaderHandle(newPipelineHandle);
            }

            void VulkanSubmitFrame(VulkanContextResources* vulkanContextResources)
            {
                // Submit
                VkSubmitInfo submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphores[1] = { vulkanContextResources->swapChainImageAvailableSemaphores[vulkanContextResources->currentFrame] };
                VkPipelineStageFlags waitStages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage];

                VkSemaphore signalSemaphores[1] = { vulkanContextResources->renderCompleteSemaphores[vulkanContextResources->currentFrame] };
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                vkResetFences(vulkanContextResources->device, 1, &vulkanContextResources->fences[vulkanContextResources->currentFrame]);
                VkResult result = vkQueueSubmit(vulkanContextResources->graphicsQueue, 1, &submitInfo, vulkanContextResources->fences[vulkanContextResources->currentFrame]);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to submit command buffer to queue!", Core::Utility::eLogSeverityCritical);
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
                    Core::Utility::LogMsg("Platform", "Failed to present swap chain!", Core::Utility::eLogSeverityInfo);
                    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
                    {
                        TINKER_ASSERT(0);
                        /*Core::Utility::LogMsg("Platform", "Recreating swap chain!", Core::Utility::eLogSeverityInfo);
                        VulkanDestroySwapChain(vulkanContextResources);
                        VulkanCreateSwapChain(vulkanContextResources);
                        return; // Don't present on this frame*/
                    }
                    else
                    {
                        Core::Utility::LogMsg("Platform", "Not recreating swap chain!", Core::Utility::eLogSeverityCritical);
                        TINKER_ASSERT(0);
                    }
                }

                vulkanContextResources->currentFrame = (vulkanContextResources->currentFrame + 1) % VULKAN_MAX_FRAMES_IN_FLIGHT;

                vkQueueWaitIdle(vulkanContextResources->presentationQueue);
            }

            void* VulkanMapResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle)
            {
                VulkanMemResource* resource =
                    &vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[vulkanContextResources->currentSwapChainImage];

                void* newMappedMem;
                VkResult result = vkMapMemory(vulkanContextResources->device, resource->deviceMemory, 0, VK_WHOLE_SIZE, 0, &newMappedMem);

                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to map gpu memory!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                    return nullptr;
                }

                return newMappedMem;
            }

            void VulkanUnmapResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle)
            {
                VulkanMemResource* resource =
                    &vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[vulkanContextResources->currentSwapChainImage];

                // Flush right before unmapping
                VkMappedMemoryRange memoryRange = {};
                memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                memoryRange.memory = resource->deviceMemory;
                memoryRange.offset = 0;
                memoryRange.size = VK_WHOLE_SIZE;

                VkResult result = vkFlushMappedMemoryRanges(vulkanContextResources->device, 1, &memoryRange);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to flush mapped gpu memory!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                vkUnmapMemory(vulkanContextResources->device, resource->deviceMemory);
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
                    Core::Utility::LogMsg("Platform", "Failed to find memory property flags!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkMemoryAllocateInfo memAllocInfo = {};
                memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                memAllocInfo.allocationSize = memRequirements.size;
                memAllocInfo.memoryTypeIndex = memTypeIndex;
                VkResult result = vkAllocateMemory(vulkanContextResources->device, &memAllocInfo, nullptr, deviceMemory);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to allocate gpu memory!", Core::Utility::eLogSeverityCritical);
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
                    Core::Utility::LogMsg("Platform", "Failed to allocate Vulkan buffer!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkMemoryRequirements memRequirements;
                vkGetBufferMemoryRequirements(vulkanContextResources->device, buffer, &memRequirements);

                AllocGPUMemory(vulkanContextResources, &deviceMemory, memRequirements, propertyFlags);

                vkBindBufferMemory(vulkanContextResources->device, buffer, deviceMemory, 0);
            }

            ResourceHandle VulkanCreateBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes, uint32 bufferUsage)
            {
                uint32 newResourceHandle =
                    vulkanContextResources->vulkanMemResourcePool.Alloc();
                TINKER_ASSERT(newResourceHandle != TINKER_INVALID_HANDLE);
                VulkanMemResourceChain* newResourceChain = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(newResourceHandle);
                *newResourceChain = {};

                // TODO: do or do not have a resource chain for buffers.
                bool oneBufferOnly = false;
                bool perSwapChainSize = false;

                for (uint32 uiImage = 0; uiImage < vulkanContextResources->numSwapChainImages && !oneBufferOnly; ++uiImage)
                {
                    VulkanMemResource* newResource = &newResourceChain->resourceChain[uiImage];

                    VkBufferUsageFlags usageFlags = {};
                    VkMemoryPropertyFlagBits propertyFlags = {};

                    switch (bufferUsage)
                    {
                        case eBufferUsageVertex:
                        {
                            oneBufferOnly = true;
                            perSwapChainSize = true;
                            usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                            propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                            break;
                        }

                        case eBufferUsageIndex:
                        {
                            oneBufferOnly = true;
                            perSwapChainSize = true;
                            usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                            propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                            break;
                        }

                        case eBufferUsageStaging:
                        {
                            oneBufferOnly = true;
                            perSwapChainSize = false;
                            usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                            propertyFlags = (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
                            break;
                        }

                        case eBufferUsageUniform:
                        {
                            usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                            propertyFlags = (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                            break;
                        }

                        default:
                        {
                            Core::Utility::LogMsg("Platform", "Invalid buffer usage specified!", Core::Utility::eLogSeverityCritical);
                            TINKER_ASSERT(0);
                            break;
                        }
                    }

                    // allocate buffer resource size in bytes equal to one buffer per swap chain image
                    uint32 numBytesToAllocate = perSwapChainSize ? sizeInBytes * vulkanContextResources->numSwapChainImages : sizeInBytes;

                    CreateBuffer(vulkanContextResources,
                        numBytesToAllocate,
                        usageFlags,
                        propertyFlags,
                        newResource->buffer,
                        newResource->deviceMemory);
                }

                return ResourceHandle(newResourceHandle);
            }

            DescriptorHandle VulkanCreateDescriptor(VulkanContextResources* vulkanContextResources, DescriptorLayout* descLayout)
            {
                if (vulkanContextResources->descriptorPool == VK_NULL_HANDLE)
                {
                    // Allocate the descriptor pool
                    InitDescriptorPool(vulkanContextResources);
                }
                
                // Descriptor layout
                VkDescriptorSetLayoutBinding descLayoutBinding[MAX_DESCRIPTOR_SETS_PER_SHADER][MAX_DESCRIPTORS_PER_SET] = {};
                uint32 descriptorCount = 0;

                for (uint32 uiDescSet = 0; uiDescSet < MAX_DESCRIPTOR_SETS_PER_SHADER; ++uiDescSet)
                {
                    for (uint32 uiDesc = 0; uiDesc < MAX_DESCRIPTORS_PER_SET; ++uiDesc)
                    {
                        if (descLayout->descriptorLayoutParams[uiDescSet][uiDesc].type != TINKER_INVALID_HANDLE)
                        {
                            descLayoutBinding[uiDescSet][uiDesc].descriptorType = GetVkDescriptorType(descLayout->descriptorLayoutParams[uiDescSet][uiDesc].type);
                            descLayoutBinding[uiDescSet][uiDesc].descriptorCount = descLayout->descriptorLayoutParams[uiDescSet][uiDesc].amount;
                            descLayoutBinding[uiDescSet][uiDesc].binding = uiDesc + uiDescSet * MAX_DESCRIPTORS_PER_SET;
                            descLayoutBinding[uiDescSet][uiDesc].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                            descLayoutBinding[uiDescSet][uiDesc].pImmutableSamplers = nullptr;
                            ++descriptorCount;
                        }
                    }
                }

                uint32 newDescriptorHandle = TINKER_INVALID_HANDLE;

                VkResult result = VK_ERROR_UNKNOWN;
                if (descriptorCount)
                {
                    newDescriptorHandle = vulkanContextResources->vulkanDescriptorResourcePool.Alloc();
                }
                else
                {
                    Core::Utility::LogMsg("Platform", "No descriptors passed to VulkanCreateDescriptor()!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkDescriptorSetLayout* descriptorSetLayout = nullptr;
                for (uint32 uiImage = 0; uiImage < vulkanContextResources->numSwapChainImages; ++uiImage)
                {
                    descriptorSetLayout = &vulkanContextResources->vulkanDescriptorResourcePool.PtrFromHandle(newDescriptorHandle)->resourceChain[uiImage].descriptorLayout;
                    *descriptorSetLayout = VK_NULL_HANDLE;

                    VkDescriptorSetLayoutCreateInfo descLayoutInfo = {};
                    descLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                    descLayoutInfo.bindingCount = descriptorCount;
                    descLayoutInfo.pBindings = &descLayoutBinding[0][0];

                    result = vkCreateDescriptorSetLayout(vulkanContextResources->device, &descLayoutInfo, nullptr, descriptorSetLayout);
                    if (result != VK_SUCCESS)
                    {
                        Core::Utility::LogMsg("Platform", "Failed to create Vulkan descriptor set layout!", Core::Utility::eLogSeverityCritical);
                        TINKER_ASSERT(0);
                    }

                    if (descriptorSetLayout != VK_NULL_HANDLE)
                    {
                        VkDescriptorSetAllocateInfo descSetAllocInfo = {};
                        descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                        descSetAllocInfo.descriptorPool = vulkanContextResources->descriptorPool;
                        descSetAllocInfo.descriptorSetCount = 1;
                        descSetAllocInfo.pSetLayouts = descriptorSetLayout;

                        result = vkAllocateDescriptorSets(vulkanContextResources->device,
                            &descSetAllocInfo,
                            &vulkanContextResources->vulkanDescriptorResourcePool.PtrFromHandle(newDescriptorHandle)->resourceChain[uiImage].descriptorSet);
                        if (result != VK_SUCCESS)
                        {
                            Core::Utility::LogMsg("Platform", "Failed to create Vulkan descriptor set!", Core::Utility::eLogSeverityCritical);
                            TINKER_ASSERT(0);
                        }
                    }
                }

                return DescriptorHandle(newDescriptorHandle);
            }

            void VulkanDestroyDescriptor(VulkanContextResources* vulkanContextResources, DescriptorHandle handle)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?
                for (uint32 uiImage = 0; uiImage < vulkanContextResources->numSwapChainImages; ++uiImage)
                {
                    VkDescriptorSetLayout* descSetLayout =
                        &vulkanContextResources->vulkanDescriptorResourcePool.PtrFromHandle(handle.m_hDesc)->resourceChain[uiImage].descriptorLayout;
                    vkDestroyDescriptorSetLayout(vulkanContextResources->device, *descSetLayout, nullptr);
                }
                vulkanContextResources->vulkanDescriptorResourcePool.Dealloc(handle.m_hDesc);
            }

            void VulkanDestroyAllDescriptors(VulkanContextResources* vulkanContextResources)
            {
                vkDestroyDescriptorPool(vulkanContextResources->device, vulkanContextResources->descriptorPool, nullptr);
                vulkanContextResources->descriptorPool = VK_NULL_HANDLE;
            }

            void VulkanWriteDescriptor(VulkanContextResources* vulkanContextResources, DescriptorLayout* descLayout, DescriptorHandle descSetHandle, DescriptorSetDataHandles* descSetHandles)
            {
                TINKER_ASSERT(descSetHandle != DefaultDescHandle_Invalid);

                for (uint32 uiImage = 0; uiImage < vulkanContextResources->numSwapChainImages; ++uiImage)
                {
                    // Descriptor layout
                    VkWriteDescriptorSet descSetWrites[MAX_DESCRIPTORS_PER_SET] = {};
                    uint32 descriptorCount = 0;

                    // TODO: not hard-coded descriptor set index
                    const uint32 uiDescSet = 0;
                    VkDescriptorSet* descriptorSet =
                        &vulkanContextResources->vulkanDescriptorResourcePool.PtrFromHandle(descSetHandle.m_hDesc)->resourceChain[uiImage].descriptorSet;

                    // Desc set info data
                    VkDescriptorBufferInfo descBufferInfo = {};
                    VkDescriptorImageInfo descImageInfo = {};

                    for (uint32 uiDesc = 0; uiDesc < MAX_DESCRIPTORS_PER_SET; ++uiDesc)
                    {
                        descSetWrites[uiDesc].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

                        if (descLayout->descriptorLayoutParams[uiDescSet][uiDesc].type != TINKER_INVALID_HANDLE)
                        {
                            switch (descLayout->descriptorLayoutParams[uiDescSet][uiDesc].type)
                            {
                                case eDescriptorTypeBuffer:
                                {
                                    VkBuffer* buffer = &vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(descSetHandles->handles[uiDesc].m_hRes)->resourceChain[uiImage].buffer;

                                    descBufferInfo.buffer = *buffer;
                                    descBufferInfo.offset = 0;
                                    descBufferInfo.range = VK_WHOLE_SIZE;

                                    descSetWrites[uiDesc].dstSet = *descriptorSet;
                                    descSetWrites[uiDesc].dstBinding = 0;
                                    descSetWrites[uiDesc].dstArrayElement = 0;
                                    descSetWrites[uiDesc].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                    descSetWrites[uiDesc].descriptorCount = descriptorCount + 1;
                                    descSetWrites[uiDesc].pBufferInfo = &descBufferInfo;
                                    break;
                                }

                                case eDescriptorTypeSampledImage:
                                {
                                    VkImageView imageView = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(descSetHandles->handles[uiDesc].m_hRes)->resourceChain[uiImage].imageView;

                                    descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                    descImageInfo.imageView = imageView;
                                    descImageInfo.sampler = vulkanContextResources->linearSampler;

                                    descSetWrites[uiDesc].dstSet = *descriptorSet;
                                    descSetWrites[uiDesc].dstBinding = 0;
                                    descSetWrites[uiDesc].dstArrayElement = 0;
                                    descSetWrites[uiDesc].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                                    descSetWrites[uiDesc].descriptorCount = descriptorCount + 1;
                                    descSetWrites[uiDesc].pImageInfo = &descImageInfo;
                                    break;
                                }

                                default:
                                {
                                    break;
                                }
                            }
                            ++descriptorCount;
                        }
                    }

                    vkUpdateDescriptorSets(vulkanContextResources->device, descriptorCount, descSetWrites, 0, nullptr);
                }
            }

            void InitDescriptorPool(VulkanContextResources* vulkanContextResources)
            {
                // * vulkanContextResources->numSwapChainImages; // TODO: per-swapchain image descriptors
                VkDescriptorPoolSize descPoolSizes[VULKAN_NUM_SUPPORTED_DESCRIPTOR_TYPES] = {};
                descPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descPoolSizes[0].descriptorCount = VULKAN_DESCRIPTOR_POOL_MAX_UNIFORM_BUFFERS;
                descPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descPoolSizes[1].descriptorCount = VULKAN_DESCRIPTOR_POOL_MAX_SAMPLED_IMAGES;

                VkDescriptorPoolCreateInfo descPoolCreateInfo = {};
                descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                descPoolCreateInfo.poolSizeCount = VULKAN_NUM_SUPPORTED_DESCRIPTOR_TYPES;
                descPoolCreateInfo.pPoolSizes = descPoolSizes;
                descPoolCreateInfo.maxSets = VULKAN_DESCRIPTOR_POOL_MAX_UNIFORM_BUFFERS + VULKAN_DESCRIPTOR_POOL_MAX_SAMPLED_IMAGES;

                VkResult result = vkCreateDescriptorPool(vulkanContextResources->device, &descPoolCreateInfo, nullptr, &vulkanContextResources->descriptorPool);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to create descriptor pool!", Core::Utility::eLogSeverityInfo);
                    return;
                }
            }

            void CreateSamplers(VulkanContextResources* vulkanContextResources)
            {
                VkSamplerCreateInfo samplerCreateInfo = {};
                samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
                samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
                samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerCreateInfo.anisotropyEnable = VK_FALSE; // VK_TRUE;
                samplerCreateInfo.maxAnisotropy = 1.0f;
                //samplerCreateInfo.maxAnisotropy = 16.0f;
                samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
                samplerCreateInfo.compareEnable = VK_FALSE;
                samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
                samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerCreateInfo.mipLodBias = 0.0f;
                samplerCreateInfo.minLod = 0.0f;
                samplerCreateInfo.maxLod = 0.0f;

                VkResult result = vkCreateSampler(vulkanContextResources->device, &samplerCreateInfo, nullptr, &vulkanContextResources->linearSampler);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to create sampler!", Core::Utility::eLogSeverityCritical);
                    return;
                }
            }

            void AcquireFrame(VulkanContextResources* vulkanContextResources)
            {
                vkWaitForFences(vulkanContextResources->device, 1, &vulkanContextResources->fences[vulkanContextResources->currentFrame], VK_TRUE, (uint64)-1);

                uint32 currentSwapChainImageIndex = TINKER_INVALID_HANDLE;

                VkResult result = vkAcquireNextImageKHR(vulkanContextResources->device,
                    vulkanContextResources->swapChain,
                    (uint64)-1,
                    vulkanContextResources->swapChainImageAvailableSemaphores[vulkanContextResources->currentFrame],
                    VK_NULL_HANDLE,
                    &currentSwapChainImageIndex);
                
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to acquire next swap chain image!", Core::Utility::eLogSeverityInfo);
                    if (result == VK_ERROR_OUT_OF_DATE_KHR)
                    {
                        Core::Utility::LogMsg("Platform", "Recreating swap chain!", Core::Utility::eLogSeverityInfo);
                        VulkanDestroySwapChain(vulkanContextResources);
                        VulkanCreateSwapChain(vulkanContextResources);
                        return; // Don't present on this frame
                    }
                    else
                    {
                        Core::Utility::LogMsg("Platform", "Not recreating swap chain!", Core::Utility::eLogSeverityCritical);
                        TINKER_ASSERT(0);
                    }
                }

                if (vulkanContextResources->imageInFlightFences[currentSwapChainImageIndex] != VK_NULL_HANDLE) {
                    vkWaitForFences(vulkanContextResources->device, 1, &vulkanContextResources->imageInFlightFences[currentSwapChainImageIndex], VK_TRUE, (uint64)-1);
                }
                vulkanContextResources->imageInFlightFences[currentSwapChainImageIndex] = vulkanContextResources->fences[vulkanContextResources->currentFrame];

                vulkanContextResources->currentSwapChainImage = currentSwapChainImageIndex;
            }

            void BeginVulkanCommandRecording(VulkanContextResources* vulkanContextResources)
            {
                VkCommandBufferBeginInfo commandBufferBeginInfo = {};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                commandBufferBeginInfo.flags = 0;
                commandBufferBeginInfo.pInheritanceInfo = nullptr;

                VkResult result = vkBeginCommandBuffer(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage], &commandBufferBeginInfo);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to begin Vulkan command buffer!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
            }

            void EndVulkanCommandRecording(VulkanContextResources* vulkanContextResources)
            {
                if (vulkanContextResources->currentSwapChainImage == TINKER_INVALID_HANDLE)
                {
                    TINKER_ASSERT(0);
                }

                VkResult result = vkEndCommandBuffer(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage]);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to end Vulkan command buffer!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
            }

            void BeginVulkanCommandRecordingImmediate(VulkanContextResources* vulkanContextResources)
            {
                VkCommandBufferBeginInfo commandBufferBeginInfo = {};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                VkResult result = vkBeginCommandBuffer(vulkanContextResources->commandBuffer_Immediate, &commandBufferBeginInfo);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to begin Vulkan command buffer (immediate)!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
            }

            void EndVulkanCommandRecordingImmediate(VulkanContextResources* vulkanContextResources)
            {
                VkResult result = vkEndCommandBuffer(vulkanContextResources->commandBuffer_Immediate);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to end Vulkan command buffer (immediate)!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }

                VkSubmitInfo submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &vulkanContextResources->commandBuffer_Immediate;

                result = vkQueueSubmit(vulkanContextResources->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
                if (result != VK_SUCCESS)
                {
                    Core::Utility::LogMsg("Platform", "Failed to submit command buffer to queue!", Core::Utility::eLogSeverityCritical);
                }
                vkQueueWaitIdle(vulkanContextResources->graphicsQueue);
            }

            VkCommandBuffer ChooseAppropriateCommandBuffer(VulkanContextResources* vulkanContextResources, bool immediateSubmit)
            {
                VkCommandBuffer commandBuffer = vulkanContextResources->commandBuffer_Immediate;

                if (!immediateSubmit)
                {
                    TINKER_ASSERT(vulkanContextResources->currentSwapChainImage != TINKER_INVALID_HANDLE);
                    commandBuffer = vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage];
                }

                return commandBuffer;
            }

            void VulkanRecordCommandDrawCall(VulkanContextResources* vulkanContextResources,
                ResourceHandle positionBufferHandle, ResourceHandle uvBufferHandle,
                ResourceHandle normalBufferHandle, ResourceHandle indexBufferHandle, uint32 numIndices,
                const char* debugLabel, bool immediateSubmit)
            {
                TINKER_ASSERT(positionBufferHandle != DefaultResHandle_Invalid);
                TINKER_ASSERT(uvBufferHandle != DefaultResHandle_Invalid);
                TINKER_ASSERT(normalBufferHandle != DefaultResHandle_Invalid);

                VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(vulkanContextResources, immediateSubmit);

                uint32 numSwapChainImages = vulkanContextResources->numSwapChainImages;
                uint32 currentSwapChainImage = vulkanContextResources->currentSwapChainImage;

                // Vertex buffers
                const uint32 numVertexBufferBindings = 3;
                VulkanMemResourceChain* vertexBufferResources[numVertexBufferBindings] = {};
                vertexBufferResources[0] = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(positionBufferHandle.m_hRes);
                vertexBufferResources[1] = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(uvBufferHandle.m_hRes);
                vertexBufferResources[2] = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(normalBufferHandle.m_hRes);

                uint32 vertexBufferSizes[numVertexBufferBindings] = {};
                for (uint32 uiBuf = 0; uiBuf < numVertexBufferBindings; ++uiBuf)
                {
                    vertexBufferSizes[uiBuf] = vertexBufferResources[uiBuf]->resDesc.dims.x;
                }

                VkBuffer vertexBuffers[numVertexBufferBindings] =
                {
                    //TODO: bad indexing
                    vertexBufferResources[0]->resourceChain[0/*currentSwapChainImage*/].buffer,
                    vertexBufferResources[1]->resourceChain[0/*currentSwapChainImage*/].buffer,
                    vertexBufferResources[2]->resourceChain[0/*currentSwapChainImage*/].buffer
                };
                VkDeviceSize offsets[numVertexBufferBindings] = {};
                for (uint32 uiBuf = 0; uiBuf < numVertexBufferBindings; ++uiBuf)
                {
                    offsets[uiBuf] = vertexBufferSizes[uiBuf] * currentSwapChainImage; // offset into the buffer based on current swap chain image
                }
                vkCmdBindVertexBuffers(commandBuffer, 0, numVertexBufferBindings, vertexBuffers, offsets);

                // Index buffer
                VulkanMemResourceChain* indexBufferResource = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(indexBufferHandle.m_hRes);
                uint32 indexBufferSize = indexBufferResource->resDesc.dims.x;
                //TODO: bad indexing
                VkBuffer& indexBuffer = indexBufferResource->resourceChain[0/*vulkanContextResources->currentSwapChainImage*/].buffer;
                vkCmdBindIndexBuffer(commandBuffer, indexBuffer, indexBufferSize * currentSwapChainImage, VK_INDEX_TYPE_UINT32);

                #if defined(ENABLE_VULKAN_DEBUG_LABELS)
                VkDebugUtilsLabelEXT label =
                {
                    VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
                    NULL,
                    debugLabel,
                    { 0.0f, 0.0f, 0.0f, 0.0f },
                };
                vulkanContextResources->pfnCmdInsertDebugUtilsLabelEXT(commandBuffer, &label);
                #endif
                vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, 0, 0);
            }

            void VulkanRecordCommandBindShader(VulkanContextResources* vulkanContextResources,
                ShaderHandle shaderHandle, const DescriptorSetDescHandles* descSetHandles, bool immediateSubmit)
            {
                TINKER_ASSERT(shaderHandle != DefaultShaderHandle_Invalid);

                VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(vulkanContextResources, immediateSubmit);

                VulkanPipelineResource* pipelineResource = vulkanContextResources->vulkanPipelineResourcePool.PtrFromHandle(shaderHandle.m_hShader);

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineResource->graphicsPipeline);

                // TODO: bind multiple descriptor sets
                if (descSetHandles->handles[0] != DefaultDescHandle_Invalid)
                {
                    VkDescriptorSet* descSet =
                        &vulkanContextResources->vulkanDescriptorResourcePool.PtrFromHandle(descSetHandles->handles[0].m_hDesc)->resourceChain[vulkanContextResources->currentSwapChainImage].descriptorSet;
                    vkCmdBindDescriptorSets(vulkanContextResources->commandBuffers[vulkanContextResources->currentSwapChainImage], VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelineResource->pipelineLayout, 0, 1, descSet, 0, nullptr);
                }
            }

            void VulkanRecordCommandMemoryTransfer(VulkanContextResources* vulkanContextResources,
                uint32 sizeInBytes, ResourceHandle srcBufferHandle, ResourceHandle dstBufferHandle,
                const char* debugLabel, bool immediateSubmit)
            {
                VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(vulkanContextResources, immediateSubmit);

                #if defined(ENABLE_VULKAN_DEBUG_LABELS)
                VkDebugUtilsLabelEXT label =
                {
                    VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
                    NULL,
                    debugLabel,
                    { 0.0f, 0.0f, 0.0f, 0.0f },
                };
                vulkanContextResources->pfnCmdInsertDebugUtilsLabelEXT(commandBuffer, &label);
                #endif

                VulkanMemResourceChain* dstResourceChain = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(dstBufferHandle.m_hRes);
                VulkanMemResource* dstResource = &dstResourceChain->resourceChain[vulkanContextResources->currentSwapChainImage];

                switch (dstResourceChain->resDesc.resourceType)
                {
                    case eResourceTypeBuffer1D:
                    {
                        VkBufferCopy bufferCopies[VULKAN_MAX_SWAP_CHAIN_IMAGES] = {};

                        for (uint32 uiBuf = 0; uiBuf < vulkanContextResources->numSwapChainImages; ++uiBuf)
                        {
                            bufferCopies[uiBuf].srcOffset = 0; // TODO: make this a function param?
                            bufferCopies[uiBuf].size = sizeInBytes;

                            bufferCopies[uiBuf].dstOffset = sizeInBytes * uiBuf;
                        }

                        VkBuffer srcBuffer = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(srcBufferHandle.m_hRes)->resourceChain[0].buffer;
                        VkBuffer dstBuffer = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(dstBufferHandle.m_hRes)->resourceChain[0].buffer;
                        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, vulkanContextResources->numSwapChainImages, bufferCopies);

                        break;
                    }

                    case eResourceTypeImage2D:
                    {
                        uint32 bytesPerPixel = 0;
                        switch (dstResourceChain->resDesc.imageFormat)
                        {
                            case eImageFormat_BGRA8_SRGB:
                            case eImageFormat_RGBA8_SRGB:
                            {
                                bytesPerPixel = 32 / 8;
                                break;
                            }

                            default:
                            {
                                Core::Utility::LogMsg("Platform", "Unsupported image copy dst format!", Core::Utility::eLogSeverityCritical);
                                TINKER_ASSERT(0);
                                return;
                            }
                        }

                        uint32 width = dstResourceChain->resDesc.dims.x;
                        uint32 height = (sizeInBytes / bytesPerPixel) / width;

                        VkBuffer& srcBuffer = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(srcBufferHandle.m_hRes)->resourceChain[vulkanContextResources->currentSwapChainImage].buffer;
                        VkImage& dstImage = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(dstBufferHandle.m_hRes)->resourceChain[vulkanContextResources->currentSwapChainImage].image;

                        // TODO: make some of these function params
                        VkBufferImageCopy region = {};
                        region.bufferOffset = 0;
                        region.bufferRowLength = 0;
                        region.bufferImageHeight = 0;
                        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        region.imageSubresource.mipLevel = 0;
                        region.imageSubresource.baseArrayLayer = 0;
                        region.imageSubresource.layerCount = 1;
                        region.imageOffset = { 0, 0, 0 };
                        region.imageExtent = { width, height, 1 };

                        vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
                        break;
                    }
                }
            }

            void VulkanRecordCommandRenderPassBegin(VulkanContextResources* vulkanContextResources,
                FramebufferHandle framebufferHandle, uint32 renderWidth, uint32 renderHeight,
                const char* debugLabel, bool immediateSubmit)
            {
                VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(vulkanContextResources, immediateSubmit);

                VkRenderPassBeginInfo renderPassBeginInfo = {};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderArea.extent = VkExtent2D({ renderWidth, renderHeight });

                FramebufferHandle framebuffer = framebufferHandle;
                if (framebuffer == DefaultFramebufferHandle_Invalid)
                {
                    framebuffer = vulkanContextResources->swapChainFramebufferHandle;
                    renderPassBeginInfo.renderArea.extent = vulkanContextResources->swapChainExtent;
                }
                VulkanFramebufferResource* framebufferPtr =
                    &vulkanContextResources->vulkanFramebufferResourcePool.PtrFromHandle(framebuffer.m_hFramebuffer)->resourceChain[vulkanContextResources->currentSwapChainImage];
                renderPassBeginInfo.framebuffer = framebufferPtr->framebuffer;
                renderPassBeginInfo.renderPass = framebufferPtr->renderPass;

                renderPassBeginInfo.clearValueCount = framebufferPtr->numClearValues;
                renderPassBeginInfo.pClearValues = framebufferPtr->clearValues;

                #if defined(ENABLE_VULKAN_DEBUG_LABELS)
                VkDebugUtilsLabelEXT label =
                {
                    VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
                    NULL,
                    debugLabel,
                    { 0.0f, 0.0f, 0.0f, 0.0f },
                };
                vulkanContextResources->pfnCmdBeginDebugUtilsLabelEXT(commandBuffer, &label);
                #endif

                vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            }

            void VulkanRecordCommandRenderPassEnd(VulkanContextResources* vulkanContextResources, bool immediateSubmit)
            {
                VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(vulkanContextResources, immediateSubmit);

                vkCmdEndRenderPass(commandBuffer);

                #if defined(ENABLE_VULKAN_DEBUG_LABELS)
                vulkanContextResources->pfnCmdEndDebugUtilsLabelEXT(commandBuffer);
                #endif
            }

            void VulkanRecordCommandTransitionLayout(VulkanContextResources* vulkanContextResources, ResourceHandle imageHandle,
                uint32 startLayout, uint32 endLayout, const char* debugLabel, bool immediateSubmit)
            {
                if (startLayout == endLayout)
                {
                    // Useless transition, don't record it
                    TINKER_ASSERT(0);
                    return;
                }

                VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(vulkanContextResources, immediateSubmit);

                #if defined(ENABLE_VULKAN_DEBUG_LABELS)
                VkDebugUtilsLabelEXT label =
                {
                    VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
                    NULL,
                    debugLabel,
                    { 0.0f, 0.0f, 0.0f, 0.0f },
                };
                vulkanContextResources->pfnCmdInsertDebugUtilsLabelEXT(commandBuffer, &label);
                #endif

                VulkanMemResourceChain* memResourceChain = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(imageHandle.m_hRes);
                VulkanMemResource* memResource = &memResourceChain->resourceChain[vulkanContextResources->currentSwapChainImage];

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.oldLayout = GetVkImageLayout(startLayout);
                barrier.newLayout = GetVkImageLayout(endLayout);

                VkPipelineStageFlags srcStage;
                VkPipelineStageFlags dstStage;

                switch (startLayout)
                {
                    case eImageLayoutUndefined:
                    {
                        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        barrier.srcAccessMask = 0;
                        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                        break;
                    }

                    case eImageLayoutShaderRead:
                    {
                        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                        break;
                    }

                    case eImageLayoutTransferDst:
                    {
                        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                        break;
                    }

                    case eImageLayoutDepthOptimal:
                    {
                        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                        break;
                    }

                    default:
                    {
                        Core::Utility::LogMsg("Platform", "Invalid dst image resource layout specified for layout transition!", Core::Utility::eLogSeverityCritical);
                        TINKER_ASSERT(0);
                        return;
                    }
                }

                switch (endLayout)
                {
                    case eImageLayoutUndefined:
                    {
                        TINKER_ASSERT(0);
                        // Can't transition to undefined according to Vulkan spec
                        return;
                    }

                    case eImageLayoutShaderRead:
                    {
                        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                        break;
                    }

                    case eImageLayoutTransferDst:
                    {
                        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                        break;
                    }

                    case eImageLayoutDepthOptimal:
                    {
                        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        dstStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                        break;
                    }

                    default:
                    {
                        Core::Utility::LogMsg("Platform", "Invalid src image resource layout specified for layout transition!", Core::Utility::eLogSeverityCritical);
                        TINKER_ASSERT(0);
                        return;
                    }
                }

                switch (memResourceChain->resDesc.imageFormat)
                {
                    case eImageFormat_BGRA8_SRGB:
                    case eImageFormat_RGBA8_SRGB:
                    {
                        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        break;
                    }

                    case eImageFormat_Depth_32F:
                    {
                        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                        break;
                    }

                    default:
                    {
                        Core::Utility::LogMsg("Platform", "Invalid image format for layout transition command!", Core::Utility::eLogSeverityCritical);
                        TINKER_ASSERT(0);
                        return;
                    }
                }

                barrier.image = memResource->image;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;

                vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            }

            void VulkanRecordCommandClearImage(VulkanContextResources* vulkanContextResources, ResourceHandle imageHandle,
                const Core::Math::v4f& clearValue, const char* debugLabel, bool immediateSubmit)
            {
                VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(vulkanContextResources, immediateSubmit);

                #if defined(ENABLE_VULKAN_DEBUG_LABELS)
                VkDebugUtilsLabelEXT label =
                {
                    VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
                    NULL,
                    debugLabel,
                    { 0.0f, 0.0f, 0.0f, 0.0f },
                };
                vulkanContextResources->pfnCmdInsertDebugUtilsLabelEXT(commandBuffer, &label);
                #endif

                VulkanMemResourceChain* memResourceChain = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(imageHandle.m_hRes);
                VulkanMemResource* memResource = &memResourceChain->resourceChain[vulkanContextResources->currentSwapChainImage];

                VkClearColorValue clearColor = {};
                VkClearDepthStencilValue clearDepth = {};

                VkImageSubresourceRange range = {};
                range.baseMipLevel = 0;
                range.levelCount = 1;
                range.baseArrayLayer = 0;
                range.layerCount = 1;

                switch (memResourceChain->resDesc.imageFormat)
                {
                    case eImageFormat_BGRA8_SRGB:
                    {
                        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        for (uint32 i = 0; i < 4; ++i)
                        {
                            clearColor.uint32[i] = (uint32)clearValue[i];
                        }

                        vkCmdClearColorImage(commandBuffer,
                            memResource->image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

                        break;
                    }

                    case eImageFormat_RGBA8_SRGB:
                    {
                        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        for (uint32 i = 0; i < 4; ++i)
                        {
                            clearColor.uint32[i] = (uint32)clearValue[i];
                        }

                        vkCmdClearColorImage(commandBuffer,
                            memResource->image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

                        break;
                    }

                    case eImageFormat_Depth_32F:
                    {
                        range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                        clearDepth.depth = clearValue.x;
                        clearDepth.stencil = (uint32)clearValue.y;

                        vkCmdClearDepthStencilImage(commandBuffer,
                            memResource->image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepth, 1, &range);

                        break;
                    }

                    default:
                    {
                        Core::Utility::LogMsg("Platform", "Invalid image format for clear command!", Core::Utility::eLogSeverityCritical);
                        TINKER_ASSERT(0);
                        return;
                    }
                }
            }

            FramebufferHandle VulkanCreateFramebuffer(VulkanContextResources* vulkanContextResources,
                ResourceHandle* rtColorHandles, uint32 numRTColorHandles, ResourceHandle rtDepthHandle,
                uint32 colorEndLayout, uint32 width, uint32 height)
            {
                TINKER_ASSERT(numRTColorHandles <= VULKAN_MAX_RENDERTARGETS);

                bool hasDepth = rtDepthHandle != DefaultResHandle_Invalid;

                // Alloc handle
                uint32 newFramebufferHandle = vulkanContextResources->vulkanFramebufferResourcePool.Alloc();
                TINKER_ASSERT(newFramebufferHandle != TINKER_INVALID_HANDLE);

                VkImageView* attachments = nullptr;
                if (numRTColorHandles > 0)
                {
                    attachments = new VkImageView[numRTColorHandles];
                }

                for (uint32 uiImage = 0; uiImage < vulkanContextResources->numSwapChainImages; ++uiImage)
                {
                    VulkanFramebufferResource* newFramebuffer =
                        &vulkanContextResources->vulkanFramebufferResourcePool.PtrFromHandle(newFramebufferHandle)->resourceChain[uiImage];

                    // Create render pass
                    uint32 colorFormat = eImageFormat_Invalid;
                    if (numRTColorHandles > 0)
                    {
                        // TODO: multiple RTs here
                        colorFormat = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(rtColorHandles[0].m_hRes)->resDesc.imageFormat;
                    }
                    uint32 depthFormat = eImageFormat_Invalid;
                    if (hasDepth)
                    {
                        depthFormat = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(rtDepthHandle.m_hRes)->resDesc.imageFormat;
                    }

                    if (uiImage == 0)
                    {
                        // TODO: multiple RTs here
                        CreateRenderPass(vulkanContextResources, numRTColorHandles, GetVkImageFormat(colorFormat), VK_IMAGE_LAYOUT_UNDEFINED, GetVkImageLayout(colorEndLayout), GetVkImageFormat(depthFormat), &newFramebuffer->renderPass);
                    }
                    else
                    {
                        newFramebuffer->renderPass = vulkanContextResources->vulkanFramebufferResourcePool.PtrFromHandle(newFramebufferHandle)->resourceChain[0].renderPass;
                    }

                    // Create framebuffer
                    for (uint32 uiImageView = 0; uiImageView < numRTColorHandles; ++uiImageView)
                    {
                        attachments[uiImageView] =
                            vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(rtColorHandles[uiImageView].m_hRes)->resourceChain[uiImage].imageView;
                    }

                    VkImageView depthImageView = VK_NULL_HANDLE;
                    if (hasDepth)
                    {
                        depthImageView = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(rtDepthHandle.m_hRes)->resourceChain[uiImage].imageView;
                    }

                    CreateFramebuffer(vulkanContextResources, attachments, numRTColorHandles, depthImageView, width, height, newFramebuffer->renderPass, &newFramebuffer->framebuffer);

                    for (uint32 uiRT = 0; uiRT < numRTColorHandles; ++uiRT)
                    {
                        // TODO: pass clear value as parameter
                        newFramebuffer->clearValues[uiRT].color = { 0.0f, 0.0f, 0.0f, 1.0f };
                    }
                    uint32 numClearValues = numRTColorHandles;

                    if (hasDepth)
                    {
                        newFramebuffer->clearValues[numRTColorHandles].depthStencil = { 1.0f, 0 };
                        ++numClearValues;
                    }

                    newFramebuffer->numClearValues = numClearValues;
                }
                delete attachments;

                return FramebufferHandle(newFramebufferHandle);
            }
            
            ResourceHandle VulkanCreateImageResource(VulkanContextResources* vulkanContextResources, uint32 imageFormat, uint32 width, uint32 height)
            {
                uint32 newResourceHandle = vulkanContextResources->vulkanMemResourcePool.Alloc();

                for (uint32 uiImage = 0; uiImage < vulkanContextResources->numSwapChainImages; ++uiImage)
                {
                    VulkanMemResourceChain* newResourceChain =
                        vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(newResourceHandle);
                    VulkanMemResource* newResource = &newResourceChain->resourceChain[uiImage];

                    // Create image
                    VkImageCreateInfo imageCreateInfo = {};
                    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
                    imageCreateInfo.extent.width = width;
                    imageCreateInfo.extent.height = height;
                    imageCreateInfo.extent.depth = 1;
                    imageCreateInfo.mipLevels = 1;
                    imageCreateInfo.arrayLayers = 1;
                    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                    imageCreateInfo.format = GetVkImageFormat(imageFormat);
                    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                    switch (imageFormat)
                    {
                        case eImageFormat_BGRA8_SRGB:
                        case eImageFormat_RGBA8_SRGB:
                        {
                            imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // TODO: make this a parameter?
                            break;
                        }

                        case eImageFormat_Depth_32F:
                        {
                            imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                            break;
                        }

                        case eImageFormat_Invalid:
                        default:
                        {
                            Core::Utility::LogMsg("Platform", "Invalid image resource format specified!", Core::Utility::eLogSeverityCritical);
                            TINKER_ASSERT(0);
                            break;
                        }
                    }

                    VkResult result = vkCreateImage(vulkanContextResources->device,
                        &imageCreateInfo,
                        nullptr,
                        &newResource->image);

                    VkMemoryRequirements memRequirements = {};
                    vkGetImageMemoryRequirements(vulkanContextResources->device, newResource->image, &memRequirements);
                    AllocGPUMemory(vulkanContextResources, &newResource->deviceMemory, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                    vkBindImageMemory(vulkanContextResources->device, newResource->image, newResource->deviceMemory, 0);

                    // Create image view
                    VkImageViewCreateInfo imageViewCreateInfo = {};
                    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    imageViewCreateInfo.image = newResource->image;
                    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    imageViewCreateInfo.format = GetVkImageFormat(imageFormat);
                    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
                    imageViewCreateInfo.subresourceRange.levelCount = 1;
                    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
                    imageViewCreateInfo.subresourceRange.layerCount = 1;

                    switch (imageFormat)
                    {
                        case eImageFormat_BGRA8_SRGB:
                        case eImageFormat_RGBA8_SRGB:
                        {
                            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                            break;
                        }

                        case eImageFormat_Depth_32F:
                        {
                            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                            break;
                        }

                        case eImageFormat_Invalid:
                        default:
                        {
                            Core::Utility::LogMsg("Platform", "Invalid image resource format specified!", Core::Utility::eLogSeverityCritical);
                            TINKER_ASSERT(0);
                            break;
                        }
                    }

                    result = vkCreateImageView(vulkanContextResources->device,
                        &imageViewCreateInfo,
                        nullptr,
                        &newResource->imageView);
                    if (result != VK_SUCCESS)
                    {
                        Core::Utility::LogMsg("Platform", "Failed to create Vulkan image view!", Core::Utility::eLogSeverityCritical);
                        TINKER_ASSERT(0);
                    }
                }

                return ResourceHandle(newResourceHandle);
            }

            ResourceHandle VulkanCreateResource(VulkanContextResources* vulkanContextResources, const ResourceDesc& resDesc)
            {
                ResourceHandle newHandle = DefaultResHandle_Invalid;

                switch (resDesc.resourceType)
                {
                    case eResourceTypeBuffer1D:
                    {
                        newHandle = VulkanCreateBuffer(vulkanContextResources, resDesc.dims.x, resDesc.bufferUsage);
                        break;
                    }

                    case eResourceTypeImage2D:
                    {
                        newHandle = VulkanCreateImageResource(vulkanContextResources, resDesc.imageFormat, resDesc.dims.x, resDesc.dims.y);
                        break;
                    }

                    default:
                    {
                        Core::Utility::LogMsg("Platform", "Invalid resource description type!", Core::Utility::eLogSeverityCritical);
                        return newHandle;
                    }
                }

                // Set the internal resdesc to the one provided
                VulkanMemResourceChain* newResourceChain = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(newHandle.m_hRes);
                newResourceChain->resDesc = resDesc;
                
                return newHandle;
            }

            void VulkanDestroyGraphicsPipeline(VulkanContextResources* vulkanContextResources, ShaderHandle handle)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?

                VulkanPipelineResource* pipelineResource = vulkanContextResources->vulkanPipelineResourcePool.PtrFromHandle(handle.m_hShader);
                vkDestroyPipeline(vulkanContextResources->device, pipelineResource->graphicsPipeline, nullptr);
                vkDestroyPipelineLayout(vulkanContextResources->device, pipelineResource->pipelineLayout, nullptr);

                vulkanContextResources->vulkanPipelineResourcePool.Dealloc(handle.m_hShader);
            }

            void VulkanDestroyImageResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?

                for (uint32 uiImage = 0; uiImage < vulkanContextResources->numSwapChainImages; ++uiImage)
                {
                    VulkanMemResource* resource = &vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[uiImage];
                    vkDestroyImage(vulkanContextResources->device, resource->image, nullptr);
                    vkFreeMemory(vulkanContextResources->device, resource->deviceMemory, nullptr);
                    vkDestroyImageView(vulkanContextResources->device, resource->imageView, nullptr);
                }

                vulkanContextResources->vulkanMemResourcePool.Dealloc(handle.m_hRes);
            }

            void VulkanDestroyFramebuffer(VulkanContextResources* vulkanContextResources, FramebufferHandle handle)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?

                for (uint32 uiImage = 0; uiImage < vulkanContextResources->numSwapChainImages; ++uiImage)
                {
                    VulkanFramebufferResource* framebuffer = &vulkanContextResources->vulkanFramebufferResourcePool.PtrFromHandle(handle.m_hFramebuffer)->resourceChain[uiImage];
                    vkDestroyFramebuffer(vulkanContextResources->device, framebuffer->framebuffer, nullptr);

                    if (uiImage == 0)
                    {
                        vkDestroyRenderPass(vulkanContextResources->device, framebuffer->renderPass, nullptr);
                    }
                }
                
                vulkanContextResources->vulkanFramebufferResourcePool.Dealloc(handle.m_hFramebuffer);
            }

            void VulkanDestroyBuffer(VulkanContextResources* vulkanContextResources, ResourceHandle handle, uint32 bufferUsage)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?

                for (uint32 uiImage = 0; uiImage < vulkanContextResources->numSwapChainImages; ++uiImage)
                {
                    VulkanMemResource* resource = &vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[uiImage];
                    vkDestroyBuffer(vulkanContextResources->device, resource->buffer, nullptr);
                    vkFreeMemory(vulkanContextResources->device, resource->deviceMemory, nullptr);
                }

                vulkanContextResources->vulkanMemResourcePool.Dealloc(handle.m_hRes);
            }

            void VulkanDestroyResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle)
            {
                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?

                VulkanMemResourceChain* resourceChain = vulkanContextResources->vulkanMemResourcePool.PtrFromHandle(handle.m_hRes);

                switch (resourceChain->resDesc.resourceType)
                {
                    case eResourceTypeBuffer1D:
                    {
                        VulkanDestroyBuffer(vulkanContextResources, handle, resourceChain->resDesc.bufferUsage);
                        break;
                    }

                    case eResourceTypeImage2D:
                    {
                        VulkanDestroyImageResource(vulkanContextResources, handle);
                        break;
                    }
                }
            }

            void DestroyVulkan(VulkanContextResources* vulkanContextResources)
            {
                if (!vulkanContextResources->isInitted)
                {
                    return;
                }

                vkDeviceWaitIdle(vulkanContextResources->device); // TODO: move this?

                VulkanDestroySwapChain(vulkanContextResources);

                vkDestroyCommandPool(vulkanContextResources->device, vulkanContextResources->commandPool, nullptr);
                delete vulkanContextResources->commandBuffers;
                delete vulkanContextResources->threaded_secondaryCommandBuffers;

                for (uint32 uiImage = 0; uiImage < VULKAN_MAX_FRAMES_IN_FLIGHT; ++uiImage)
                {
                    vkDestroySemaphore(vulkanContextResources->device, vulkanContextResources->renderCompleteSemaphores[uiImage], nullptr);
                    vkDestroySemaphore(vulkanContextResources->device, vulkanContextResources->swapChainImageAvailableSemaphores[uiImage], nullptr);
                    vkDestroyFence(vulkanContextResources->device, vulkanContextResources->fences[uiImage], nullptr);
                }
                delete vulkanContextResources->imageInFlightFences;

                vkDestroySampler(vulkanContextResources->device, vulkanContextResources->linearSampler, nullptr);

                #if defined(ENABLE_VULKAN_VALIDATION_LAYERS)
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
                    Core::Utility::LogMsg("Platform", "Failed to get destroy debug utils messenger proc addr!", Core::Utility::eLogSeverityCritical);
                    TINKER_ASSERT(0);
                }
                #endif

                vkDestroyDevice(vulkanContextResources->device, nullptr);
                vkDestroySurfaceKHR(vulkanContextResources->instance, vulkanContextResources->surface, nullptr);
                vkDestroyInstance(vulkanContextResources->instance, nullptr);

                vulkanContextResources->vulkanMemResourcePool.ExplicitFree();
                vulkanContextResources->vulkanPipelineResourcePool.ExplicitFree();
                vulkanContextResources->vulkanDescriptorResourcePool.ExplicitFree();
                vulkanContextResources->vulkanFramebufferResourcePool.ExplicitFree();
            }
        }
    }
}

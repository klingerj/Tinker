#include "Graphics/Vulkan/VulkanTypes.h"
#include "Graphics/Vulkan/Vulkan.h"
#include "Platform/PlatformCommon.h"
#include "Utility/Logging.h"

#include <iostream>
// TODO: move this to be a compile define
#define ENABLE_VULKAN_VALIDATION_LAYERS // enables validation layers
#define ENABLE_VULKAN_DEBUG_LABELS // enables marking up vulkan objects/commands with debug labels

#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// NOTE: The convention in this project to is flip the viewport upside down since a left-handed projection
// matrix is often used. However, doing this flip causes the application to not render properly when run
// in RenderDoc for some reason. To debug with RenderDoc, you should turn on this #define.
#define WORK_WITH_RENDERDOC

namespace Tk
{
namespace Platform
{
namespace Graphics
{

VulkanContextResources g_vulkanContextResources;

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
    const Tk::Platform::PlatformWindowHandles* platformWindowHandles,
    uint32 width, uint32 height)
{
    vulkanContextResources->resources = new VkResources{};

    vulkanContextResources->resources->vulkanMemResourcePool.Init(VULKAN_RESOURCE_POOL_MAX, 16);
    vulkanContextResources->resources->vulkanDescriptorResourcePool.Init(VULKAN_RESOURCE_POOL_MAX, 16);
    vulkanContextResources->resources->vulkanFramebufferResourcePool.Init(VULKAN_RESOURCE_POOL_MAX, 16);

    vulkanContextResources->resources->windowWidth = width;
    vulkanContextResources->resources->windowHeight = height;

    InitVulkanDataTypesPerEnum();

    // Shader pso permutations
    for (uint32 sid = 0; sid < VkResources::eMaxShaders; ++sid)
    {
        VkPipelineLayout& pipelineLayout = vulkanContextResources->resources->psoPermutations.pipelineLayout[sid];
        pipelineLayout = VK_NULL_HANDLE;

        for (uint32 bs = 0; bs < VkResources::eMaxBlendStates; ++bs)
        {
            for (uint32 ds = 0; ds < VkResources::eMaxDepthStates; ++ds)
            {
                VkPipeline& graphicsPipeline = vulkanContextResources->resources->psoPermutations.graphicsPipeline[sid][bs][ds];
                graphicsPipeline = VK_NULL_HANDLE;
            }
        }
    }
    //-----

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
    Core::Utility::LogMsg("Platform", "******** Requested Instance Extensions: ********", Core::Utility::LogSeverity::eInfo);
    for (uint32 uiReqExt = 0; uiReqExt < numRequestedExtensions; ++uiReqExt)
    {
        Core::Utility::LogMsg("Platform", requestedExtensionNames[uiReqExt], Core::Utility::LogSeverity::eInfo);
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
    Core::Utility::LogMsg("Platform", "******** Available Instance Extensions: ********", Core::Utility::LogSeverity::eInfo);
    for (uint32 uiAvailExt = 0; uiAvailExt < numAvailableExtensions; ++uiAvailExt)
    {
        Core::Utility::LogMsg("Platform", availableExtensions[uiAvailExt].extensionName, Core::Utility::LogSeverity::eInfo);
    }
    delete availableExtensions;

    // Validation layers
    #if defined(ENABLE_VULKAN_VALIDATION_LAYERS)
    const uint32 numRequestedLayers = 1;
    const char* requestedLayersStr[numRequestedLayers] =
    {
        "VK_LAYER_KHRONOS_validation"
    };
    Core::Utility::LogMsg("Platform", "******** Requested Instance Layers: ********", Core::Utility::LogSeverity::eInfo);
    for (uint32 uiReqLayer = 0; uiReqLayer < numRequestedLayers; ++uiReqLayer)
    {
        Core::Utility::LogMsg("Platform", requestedLayersStr[uiReqLayer], Core::Utility::LogSeverity::eInfo);
    }

    uint32 numAvailableLayers = 0;
    vkEnumerateInstanceLayerProperties(&numAvailableLayers, nullptr);

    if (numAvailableLayers == 0)
    {
        Core::Utility::LogMsg("Platform", "Zero available instance layers!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkLayerProperties* availableLayers = new VkLayerProperties[numAvailableLayers];
    vkEnumerateInstanceLayerProperties(&numAvailableLayers, availableLayers);

    Core::Utility::LogMsg("Platform", "******** Available Instance Layers: ********", Core::Utility::LogSeverity::eInfo);
    for (uint32 uiAvailLayer = 0; uiAvailLayer < numAvailableLayers; ++uiAvailLayer)
    {
        Core::Utility::LogMsg("Platform", availableLayers[uiAvailLayer].layerName, Core::Utility::LogSeverity::eInfo);
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
            Core::Utility::LogMsg("Platform", "Requested instance layer not supported!", Core::Utility::LogSeverity::eCritical);
            Core::Utility::LogMsg("Platform", requestedLayersStr[uiReqLayer], Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
        }
    }

    instanceCreateInfo.enabledLayerCount = numRequestedLayers;
    instanceCreateInfo.ppEnabledLayerNames = requestedLayersStr;
    #endif

    VkResult result = vkCreateInstance(&instanceCreateInfo,
        nullptr,
        &vulkanContextResources->resources->instance);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan instance!", Core::Utility::LogSeverity::eCritical);
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
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanContextResources->resources->instance,
                                                                  "vkCreateDebugUtilsMessengerEXT");
    if (dbgCreateFunc)
    {
        dbgCreateFunc(vulkanContextResources->resources->instance,
            &dbgUtilsMsgCreateInfo,
            nullptr,
            &vulkanContextResources->resources->debugMessenger);
    }
    else
    {
        Core::Utility::LogMsg("Platform", "Failed to get create debug utils messenger proc addr!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
    #endif

    // Surface
    #if defined(_WIN32)
    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {};
    win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32SurfaceCreateInfo.hinstance = platformWindowHandles->instance;
    win32SurfaceCreateInfo.hwnd = platformWindowHandles->windowHandle;

    result = vkCreateWin32SurfaceKHR(vulkanContextResources->resources->instance,
        &win32SurfaceCreateInfo,
        NULL,
        &vulkanContextResources->resources->surface);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Win32SurfaceKHR!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
    #else
    // TODO: implement other platform surface types
    TINKER_ASSERT(0);
    #endif

    // Physical device
    uint32 numPhysicalDevices = 0;
    vkEnumeratePhysicalDevices(vulkanContextResources->resources->instance, &numPhysicalDevices, nullptr);

    if (numPhysicalDevices == 0)
    {
        Core::Utility::LogMsg("Platform", "Zero available physical devices!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[numPhysicalDevices];
    vkEnumeratePhysicalDevices(vulkanContextResources->resources->instance, &numPhysicalDevices, physicalDevices);

    const uint32 numRequiredPhysicalDeviceExtensions = 1;
    const char* requiredPhysicalDeviceExtensions[numRequiredPhysicalDeviceExtensions] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    Core::Utility::LogMsg("Platform", "******** Requested Device Extensions: ********", Core::Utility::LogSeverity::eInfo);
    for (uint32 uiReqExt = 0; uiReqExt < numRequiredPhysicalDeviceExtensions; ++uiReqExt)
    {
        Core::Utility::LogMsg("Platform", requiredPhysicalDeviceExtensions[uiReqExt], Core::Utility::LogSeverity::eInfo);
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
                    vulkanContextResources->resources->graphicsQueueIndex = uiQueueFamily;
                }

                VkBool32 presentSupport;
                vkGetPhysicalDeviceSurfaceSupportKHR(currPhysicalDevice,
                    uiQueueFamily,
                    vulkanContextResources->resources->surface,
                    &presentSupport);
                if (presentSupport)
                {
                    presentationSupport = true;
                    vulkanContextResources->resources->presentationQueueIndex = uiQueueFamily;
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
                Core::Utility::LogMsg("Platform", "Zero available device extensions!", Core::Utility::LogSeverity::eCritical);
                TINKER_ASSERT(0);
            }

            VkExtensionProperties* availablePhysicalDeviceExtensions = new VkExtensionProperties[numAvailablePhysicalDeviceExtensions];
            vkEnumerateDeviceExtensionProperties(currPhysicalDevice,
                nullptr,
                &numAvailablePhysicalDeviceExtensions,
                availablePhysicalDeviceExtensions);

            Core::Utility::LogMsg("Platform", "******** Available Device Extensions: ********", Core::Utility::LogSeverity::eInfo);
            for (uint32 uiAvailExt = 0; uiAvailExt < numAvailablePhysicalDeviceExtensions; ++uiAvailExt)
            {
                Core::Utility::LogMsg("Platform", availablePhysicalDeviceExtensions[uiAvailExt].extensionName, Core::Utility::LogSeverity::eInfo);
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
                    Core::Utility::LogMsg("Platform", "Requested device extension not supported!", Core::Utility::LogSeverity::eCritical);
                    Core::Utility::LogMsg("Platform", requiredPhysicalDeviceExtensions[uiReqExt], Core::Utility::LogSeverity::eCritical);
                    TINKER_ASSERT(0);
                }
            }

            // Extension support assumed at this point
            if (graphicsSupport && presentationSupport)
            {
                // Select the current physical device
                vulkanContextResources->resources->physicalDevice = currPhysicalDevice;
                break;
            }
        }
    }
    delete physicalDevices;

    if (vulkanContextResources->resources->physicalDevice == VK_NULL_HANDLE)
    {
        Core::Utility::LogMsg("Platform", "No physical device chosen!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    // Physical device memory heaps
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    vkGetPhysicalDeviceMemoryProperties(vulkanContextResources->resources->physicalDevice, &memoryProperties);
    {
        Core::Utility::LogMsg("Platform", "******** Device Memory Properties: ********", Core::Utility::LogSeverity::eInfo);

        for (uint32 uiMemType = 0; uiMemType < memoryProperties.memoryTypeCount; ++uiMemType)
        {
            Core::Utility::LogMsg("Platform", "****************", Core::Utility::LogSeverity::eInfo);

            // Heap properties
            // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkMemoryPropertyFlagBits.html
            VkMemoryPropertyFlags propertyFlags = memoryProperties.memoryTypes[uiMemType].propertyFlags;

            Core::Utility::LogMsg("Platform", "Heap Properties:", Core::Utility::LogSeverity::eInfo);
            if (propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            {
                Core::Utility::LogMsg("Platform", "- Device local", Core::Utility::LogSeverity::eInfo);
            }
            if (propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            {
                Core::Utility::LogMsg("Platform", "- Host visible", Core::Utility::LogSeverity::eInfo);
            }
            if (propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            {
                Core::Utility::LogMsg("Platform", "- Host coherent", Core::Utility::LogSeverity::eInfo);
            }
            if (propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
            {
                Core::Utility::LogMsg("Platform", "- Host cached", Core::Utility::LogSeverity::eInfo);
            }
            if (propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
            {
                Core::Utility::LogMsg("Platform", "- Lazily allocated", Core::Utility::LogSeverity::eInfo);
            }

            // Heap size
            uint32 heapIndex = memoryProperties.memoryTypes[uiMemType].heapIndex;
            char buffer[16] = {};
            _ui64toa_s(memoryProperties.memoryHeaps[heapIndex].size, buffer, 16, 10);

            Core::Utility::LogMsg("Platform", "Heap Size:", Core::Utility::LogSeverity::eInfo);
            Core::Utility::LogMsg("Platform", buffer, Core::Utility::LogSeverity::eInfo);

            // Heap flags
            // memoryProperties.memoryHeaps[heapIndex].flags
            // Don't really care about these I think
            // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkMemoryHeapFlagBits.html
        }
        Core::Utility::LogMsg("Platform", "****************", Core::Utility::LogSeverity::eInfo);
    }

    // Logical device
    const uint32 numQueues = 2;
    VkDeviceQueueCreateInfo deviceQueueCreateInfos[numQueues] = {};

    // Create graphics queue
    deviceQueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfos[0].queueFamilyIndex = vulkanContextResources->resources->graphicsQueueIndex;
    deviceQueueCreateInfos[0].queueCount = 1;
    float graphicsQueuePriority = 1.0f;
    deviceQueueCreateInfos[0].pQueuePriorities = &graphicsQueuePriority;

    // Create presentation queue
    deviceQueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfos[1].queueFamilyIndex = vulkanContextResources->resources->presentationQueueIndex;
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

    result = vkCreateDevice(vulkanContextResources->resources->physicalDevice,
        &deviceCreateInfo,
        nullptr,
        &vulkanContextResources->resources->device);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan device!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    // Debug labels
    #if defined(ENABLE_VULKAN_DEBUG_LABELS)
    /*vulkanContextResources->resources->pfnSetDebugUtilsObjectNameEXT =
        (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(vulkanContextResources->resources->device,
                                                              "vkSetDebugUtilsObjectNameEXT")*/;
    vulkanContextResources->resources->pfnCmdBeginDebugUtilsLabelEXT =
        (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(vulkanContextResources->resources->device,
                                                               "vkCmdBeginDebugUtilsLabelEXT");
    if (!vulkanContextResources->resources->pfnCmdBeginDebugUtilsLabelEXT)
    {
        Core::Utility::LogMsg("Platform", "Failed to get create debug utils begin label proc addr!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    vulkanContextResources->resources->pfnCmdEndDebugUtilsLabelEXT =
        (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(vulkanContextResources->resources->device,
                                                               "vkCmdEndDebugUtilsLabelEXT");
    if (!vulkanContextResources->resources->pfnCmdEndDebugUtilsLabelEXT)
    {
        Core::Utility::LogMsg("Platform", "Failed to get create debug utils end label proc addr!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    vulkanContextResources->resources->pfnCmdInsertDebugUtilsLabelEXT =
        (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetDeviceProcAddr(vulkanContextResources->resources->device,
                                                               "vkCmdInsertDebugUtilsLabelEXT");
    if (!vulkanContextResources->resources->pfnCmdInsertDebugUtilsLabelEXT)
    {
        Core::Utility::LogMsg("Platform", "Failed to get create debug utils insert label proc addr!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
    #endif

    // Queues
    vkGetDeviceQueue(vulkanContextResources->resources->device,
        vulkanContextResources->resources->graphicsQueueIndex,
        0,
        &vulkanContextResources->resources->graphicsQueue);
    vkGetDeviceQueue(vulkanContextResources->resources->device,
        vulkanContextResources->resources->presentationQueueIndex,
        0, 
        &vulkanContextResources->resources->presentationQueue);

    // Swap chain
    VulkanCreateSwapChain(vulkanContextResources);

    // Command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = vulkanContextResources->resources->graphicsQueueIndex;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                  VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    result = vkCreateCommandPool(vulkanContextResources->resources->device,
        &commandPoolCreateInfo,
        nullptr,
        &vulkanContextResources->resources->commandPool);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan command pool!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    // Command buffers for per-frame submission
    vulkanContextResources->resources->commandBuffers = new VkCommandBuffer[vulkanContextResources->resources->numSwapChainImages];

    VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.commandPool = vulkanContextResources->resources->commandPool;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandBufferCount = vulkanContextResources->resources->numSwapChainImages;

    result = vkAllocateCommandBuffers(vulkanContextResources->resources->device,
        &commandBufferAllocInfo,
        vulkanContextResources->resources->commandBuffers);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to allocate Vulkan primary frame command buffers!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    // Command buffer for immediate submission
    commandBufferAllocInfo = {};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandPool = vulkanContextResources->resources->commandPool;
    commandBufferAllocInfo.commandBufferCount = 1;
    result = vkAllocateCommandBuffers(vulkanContextResources->resources->device, &commandBufferAllocInfo, &vulkanContextResources->resources->commandBuffer_Immediate);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to allocate Vulkan command buffers (immediate)!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    // Semaphores
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (uint32 uiImage = 0; uiImage < VULKAN_MAX_FRAMES_IN_FLIGHT; ++uiImage)
    {
        result = vkCreateSemaphore(vulkanContextResources->resources->device,
            &semaphoreCreateInfo,
            nullptr,
            &vulkanContextResources->resources->swapChainImageAvailableSemaphores[uiImage]);
        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Platform", "Failed to create Vulkan semaphore!", Core::Utility::LogSeverity::eCritical);
        }

        result = vkCreateSemaphore(vulkanContextResources->resources->device,
            &semaphoreCreateInfo,
            nullptr,
            &vulkanContextResources->resources->renderCompleteSemaphores[uiImage]);
        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Platform", "Failed to create Vulkan semaphore!", Core::Utility::LogSeverity::eCritical);
        }
    }

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32 uiImage = 0; uiImage < VULKAN_MAX_FRAMES_IN_FLIGHT; ++uiImage)
    {
        result = vkCreateFence(vulkanContextResources->resources->device, &fenceCreateInfo, nullptr, &vulkanContextResources->resources->fences[uiImage]);
        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Platform", "Failed to create Vulkan fence!", Core::Utility::LogSeverity::eCritical);
        }
    }
    
    vulkanContextResources->resources->imageInFlightFences = new VkFence[vulkanContextResources->resources->numSwapChainImages];
    for (uint32 uiImage = 0; uiImage < vulkanContextResources->resources->numSwapChainImages; ++uiImage)
    {
        vulkanContextResources->resources->imageInFlightFences[uiImage] = VK_NULL_HANDLE;
    }

    CreateSamplers(vulkanContextResources);

    vulkanContextResources->isInitted = true;

    return 0;
}

void VulkanCreateSwapChain(VulkanContextResources* vulkanContextResources)
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanContextResources->resources->physicalDevice,
        vulkanContextResources->resources->surface,
        &capabilities);

    VkExtent2D optimalExtent = {};
    if (capabilities.currentExtent.width != 0xffffffff)
    {
        optimalExtent = capabilities.currentExtent;
    }
    else
    {
        optimalExtent.width =
            CLAMP(vulkanContextResources->resources->windowWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        optimalExtent.height =
            CLAMP(vulkanContextResources->resources->windowHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    uint32 numSwapChainImages = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) // 0 can indicate no maximum
    {
        numSwapChainImages = min(numSwapChainImages, capabilities.maxImageCount);
    }
    numSwapChainImages = min(numSwapChainImages, VULKAN_MAX_SWAP_CHAIN_IMAGES);

    uint32 numAvailableSurfaceFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContextResources->resources->physicalDevice,
        vulkanContextResources->resources->surface,
        &numAvailableSurfaceFormats,
        nullptr);

    if (numAvailableSurfaceFormats == 0)
    {
        Core::Utility::LogMsg("Platform", "Zero available Vulkan swap chain surface formats!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkSurfaceFormatKHR* availableSurfaceFormats = new VkSurfaceFormatKHR[numAvailableSurfaceFormats];
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContextResources->resources->physicalDevice,
        vulkanContextResources->resources->surface,
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
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanContextResources->resources->physicalDevice,
        vulkanContextResources->resources->surface,
        &numAvailablePresentModes,
        nullptr);

    if (numAvailablePresentModes == 0)
    {
        Core::Utility::LogMsg("Platform", "Zero available Vulkan swap chain present modes!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkPresentModeKHR* availablePresentModes = new VkPresentModeKHR[numAvailablePresentModes];
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanContextResources->resources->physicalDevice,
        vulkanContextResources->resources->surface,
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

    vulkanContextResources->resources->swapChainExtent = optimalExtent;
    vulkanContextResources->resources->swapChainFormat = chosenFormat.format;
    vulkanContextResources->resources->numSwapChainImages = numSwapChainImages;

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = vulkanContextResources->resources->surface;
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
        vulkanContextResources->resources->graphicsQueueIndex,
        vulkanContextResources->resources->presentationQueueIndex
    };

    if (vulkanContextResources->resources->graphicsQueueIndex != vulkanContextResources->resources->presentationQueueIndex)
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

    VkResult result = vkCreateSwapchainKHR(vulkanContextResources->resources->device,
        &swapChainCreateInfo,
        nullptr,
        &vulkanContextResources->resources->swapChain);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan swap chain!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    vkGetSwapchainImagesKHR(vulkanContextResources->resources->device,
        vulkanContextResources->resources->swapChain,
        &numSwapChainImages,
        nullptr);

    for (uint32 i = 0; i < ARRAYCOUNT(vulkanContextResources->resources->swapChainImages); ++i)
        vulkanContextResources->resources->swapChainImages[i] = VK_NULL_HANDLE;
    vkGetSwapchainImagesKHR(vulkanContextResources->resources->device,
        vulkanContextResources->resources->swapChain,
        &numSwapChainImages,
        vulkanContextResources->resources->swapChainImages);

    for (uint32 i = 0; i < ARRAYCOUNT(vulkanContextResources->resources->swapChainImages); ++i)
        vulkanContextResources->resources->swapChainImageViews[i] = VK_NULL_HANDLE;
    for (uint32 uiImageView = 0; uiImageView < numSwapChainImages; ++uiImageView)
    {
        CreateImageView(vulkanContextResources->resources->device,
            vulkanContextResources->resources->swapChainFormat,
            VK_IMAGE_ASPECT_COLOR_BIT,
            vulkanContextResources->resources->swapChainImages[uiImageView],
            &vulkanContextResources->resources->swapChainImageViews[uiImageView],
            1);
    }

    // Swap chain framebuffers
    const uint32 numColorRTs = 1; // TODO: support multiple RTs

    // Render pass
    VulkanRenderPass& renderPass = vulkanContextResources->resources->renderPasses[RENDERPASS_ID_SWAP_CHAIN_BLIT];
    renderPass.numColorRTs = 1;
    renderPass.hasDepth = false;
    const VkFormat depthFormat = VK_FORMAT_UNDEFINED; // no depth buffer for swap chain
    CreateRenderPass(vulkanContextResources->resources->device, numColorRTs, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, depthFormat, &renderPass.renderPassVk);

    uint32 newFramebufferHandle = vulkanContextResources->resources->vulkanFramebufferResourcePool.Alloc();
    TINKER_ASSERT(newFramebufferHandle != TINKER_INVALID_HANDLE);
    vulkanContextResources->resources->swapChainFramebufferHandle = FramebufferHandle(newFramebufferHandle);

    for (uint32 uiFramebuffer = 0; uiFramebuffer < vulkanContextResources->resources->numSwapChainImages; ++uiFramebuffer)
    {
        VulkanFramebufferResource* newFramebuffer =
            &vulkanContextResources->resources->vulkanFramebufferResourcePool.PtrFromHandle(newFramebufferHandle)->resourceChain[uiFramebuffer];

        for (uint32 i = 0; i < numColorRTs; ++i)
        {
            newFramebuffer->clearValues[i] = { 0.0f, 0.0f, 0.0f, 1.0f };
        }
        newFramebuffer->numClearValues = numColorRTs;

        // Framebuffer
        const VkImageView depthImageView = VK_NULL_HANDLE; // no depth buffer for swap chain
        CreateFramebuffer(vulkanContextResources->resources->device, &vulkanContextResources->resources->swapChainImageViews[uiFramebuffer], 1, depthImageView,
            vulkanContextResources->resources->swapChainExtent.width, vulkanContextResources->resources->swapChainExtent.height, vulkanContextResources->resources->renderPasses[RENDERPASS_ID_SWAP_CHAIN_BLIT].renderPassVk, &newFramebuffer->framebuffer);
    }

    vulkanContextResources->isSwapChainValid = true;
}

void VulkanDestroySwapChain(VulkanContextResources* vulkanContextResources)
{
    vulkanContextResources->isSwapChainValid = false;
    vkDeviceWaitIdle(vulkanContextResources->resources->device); // TODO: move this?

    VulkanDestroyFramebuffer(vulkanContextResources, vulkanContextResources->resources->swapChainFramebufferHandle);

    for (uint32 uiImageView = 0; uiImageView < vulkanContextResources->resources->numSwapChainImages; ++uiImageView)
    {
        vkDestroyImageView(vulkanContextResources->resources->device, vulkanContextResources->resources->swapChainImageViews[uiImageView], nullptr);
    }
    
    vkDestroySwapchainKHR(vulkanContextResources->resources->device, vulkanContextResources->resources->swapChain, nullptr);
}

bool VulkanCreateGraphicsPipeline(VulkanContextResources* vulkanContextResources,
    void* vertexShaderCode, uint32 numVertexShaderBytes,
    void* fragmentShaderCode, uint32 numFragmentShaderBytes,
    uint32 shaderID, uint32 viewportWidth, uint32 viewportHeight, uint32 renderPassID,
    uint32* descriptorLayoutHandles, uint32 numDescriptorLayoutHandles)
{
    VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;

    uint32 numStages = 0;
    if (numVertexShaderBytes > 0)
    {
        vertexShaderModule = CreateShaderModule((const char*)vertexShaderCode, numVertexShaderBytes, vulkanContextResources->resources->device);
        ++numStages;
    }
    if (numFragmentShaderBytes > 0)
    {
        fragmentShaderModule = CreateShaderModule((const char*)fragmentShaderCode, numFragmentShaderBytes, vulkanContextResources->resources->device);
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

    // NOTE: no vertex buffers
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};

    #ifdef WORK_WITH_RENDERDOC
    viewport.y = 0.0f;
    viewport.height = (float)viewportHeight; //TODO: this looks wrong, doesnt use the params
    #else
    viewport.y = (float)viewportHeight;
    viewport.height = -viewportHeight;
    #endif

    viewport.x = 0.0f;
    viewport.width = (float)viewportWidth;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { viewportWidth, viewportHeight };

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

    const uint32 numDynamicStates = 2;
    VkDynamicState dynamicStates[numDynamicStates] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = numDynamicStates;
    dynamicState.pDynamicStates = dynamicStates;

    // Descriptor layouts
    TINKER_ASSERT(numDescriptorLayoutHandles <= MAX_DESCRIPTOR_SETS_PER_SHADER);

    VkDescriptorSetLayout descriptorSetLayouts[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    for (uint32 uiDesc = 0; uiDesc < numDescriptorLayoutHandles; ++uiDesc)
    {
        uint32 descLayoutID = descriptorLayoutHandles[uiDesc];
        if (descLayoutID != DESCLAYOUT_ID_MAX)
            descriptorSetLayouts[uiDesc] = vulkanContextResources->resources->descLayouts[descLayoutID].layout;
    }

    // Push constants
    const uint32 numPushConstants = 1;
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(uint32) * 4;
    // TODO: use maximum available size, or get the size as a parameter

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = numDescriptorLayoutHandles;
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = numPushConstants;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    VkPipelineLayout& pipelineLayout = vulkanContextResources->resources->psoPermutations.pipelineLayout[shaderID];
    VkResult result = vkCreatePipelineLayout(vulkanContextResources->resources->device,
        &pipelineLayoutInfo,
        nullptr,
        &pipelineLayout);

    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan pipeline layout!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    const VulkanRenderPass& renderPass = vulkanContextResources->resources->renderPasses[renderPassID];
    for (uint32 blendState = 0; blendState < VkResources::eMaxBlendStates; ++blendState)
    {
        for (uint32 depthState = 0; depthState < VkResources::eMaxDepthStates; ++depthState)
        {
            VkPipeline& graphicsPipeline = vulkanContextResources->resources->psoPermutations.graphicsPipeline[shaderID][blendState][depthState];

            VkPipelineDepthStencilStateCreateInfo depthStencilState = GetVkDepthState(depthState);
            VkPipelineColorBlendAttachmentState colorBlendAttachment = GetVkBlendState(blendState);

            if (blendState == BlendState::eNoColorAttachment)
            {
                if (renderPass.numColorRTs > 0)
                    continue;
                colorBlending.attachmentCount = 0;
                colorBlending.pAttachments = nullptr;
            }
            else
            {
                // TODO: support multiple RTs here too
                colorBlending.attachmentCount = 1;
                colorBlending.pAttachments = &colorBlendAttachment;
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
            pipelineCreateInfo.layout = pipelineLayout;

            pipelineCreateInfo.renderPass = renderPass.renderPassVk;
            pipelineCreateInfo.subpass = 0;
            pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineCreateInfo.basePipelineIndex = -1;

            result = vkCreateGraphicsPipelines(vulkanContextResources->resources->device,
                VK_NULL_HANDLE,
                1,
                &pipelineCreateInfo,
                nullptr,
                &graphicsPipeline);

            if (result != VK_SUCCESS)
            {
                Core::Utility::LogMsg("Platform", "Failed to create Vulkan graphics pipeline!", Core::Utility::LogSeverity::eCritical);
                TINKER_ASSERT(0);
            }
        }
    }

    vkDestroyShaderModule(vulkanContextResources->resources->device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(vulkanContextResources->resources->device, fragmentShaderModule, nullptr);

    return true;
}

bool VulkanCreateDescriptorLayout(VulkanContextResources* vulkanContextResources, uint32 descriptorLayoutID, const Platform::Graphics::DescriptorLayout* descriptorLayout)
{
    // Descriptor layout
    VkDescriptorSetLayoutBinding descLayoutBinding[MAX_BINDINGS_PER_SET] = {};
    uint32 numBindings = 0;

    for (uint32 uiDesc = 0; uiDesc < MAX_BINDINGS_PER_SET; ++uiDesc)
    {
        uint32 type = descriptorLayout->params[uiDesc].type;
        if (type < DescriptorType::eMax)
        {
            descLayoutBinding[numBindings].descriptorType = GetVkDescriptorType(type);
            descLayoutBinding[numBindings].descriptorCount = descriptorLayout->params[uiDesc].amount;
            descLayoutBinding[numBindings].binding = uiDesc;
            descLayoutBinding[numBindings].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            descLayoutBinding[numBindings].pImmutableSamplers = nullptr;
            ++numBindings;
        }
        else
            break;
    }

    uint32 newDescriptorHandle = TINKER_INVALID_HANDLE;
    if (numBindings > 0)
    {
        newDescriptorHandle = vulkanContextResources->resources->vulkanDescriptorResourcePool.Alloc();
    }
    else
    {
        Core::Utility::LogMsg("Platform", "No descriptors passed to VulkanCreateDescriptorLayout()!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkDescriptorSetLayout* descriptorSetLayout = &vulkanContextResources->resources->descLayouts[descriptorLayoutID].layout;
    *descriptorSetLayout = VK_NULL_HANDLE;
    
    VkDescriptorSetLayoutCreateInfo descLayoutInfo = {};
    descLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descLayoutInfo.bindingCount = numBindings;
    descLayoutInfo.pBindings = &descLayoutBinding[0];

    VkResult result = vkCreateDescriptorSetLayout(vulkanContextResources->resources->device, &descLayoutInfo, nullptr, descriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan descriptor set layout!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    memcpy(&vulkanContextResources->resources->descLayouts[descriptorLayoutID].bindings, &descriptorLayout->params[0], sizeof(Platform::Graphics::DescriptorLayout));
    return true;
}

void DestroyPSOPerms(VulkanContextResources* vulkanContextResources, uint32 shaderID)
{
    vkDeviceWaitIdle(vulkanContextResources->resources->device); // TODO: move this?

    VkPipelineLayout& pipelineLayout = vulkanContextResources->resources->psoPermutations.pipelineLayout[shaderID];
    vkDestroyPipelineLayout(vulkanContextResources->resources->device, pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;

    for (uint32 bs = 0; bs < VkResources::eMaxBlendStates; ++bs)
    {
        for (uint32 ds = 0; ds < VkResources::eMaxDepthStates; ++ds)
        {
            VkPipeline& graphicsPipeline = vulkanContextResources->resources->psoPermutations.graphicsPipeline[shaderID][bs][ds];

            vkDestroyPipeline(vulkanContextResources->resources->device, graphicsPipeline, nullptr);

            graphicsPipeline = VK_NULL_HANDLE;
        }
    }
}

void VulkanDestroyAllPSOPerms(VulkanContextResources* vulkanContextResources)
{
    vkDeviceWaitIdle(vulkanContextResources->resources->device); // TODO: move this?

    for (uint32 sid = 0; sid < VkResources::eMaxShaders; ++sid)
    {
        VkPipelineLayout& pipelineLayout = vulkanContextResources->resources->psoPermutations.pipelineLayout[sid];
        if (pipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(vulkanContextResources->resources->device, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }

        for (uint32 bs = 0; bs < VkResources::eMaxBlendStates; ++bs)
        {
            for (uint32 ds = 0; ds < VkResources::eMaxDepthStates; ++ds)
            {
                VkPipeline& graphicsPipeline = vulkanContextResources->resources->psoPermutations.graphicsPipeline[sid][bs][ds];
                if (graphicsPipeline != VK_NULL_HANDLE)
                {
                    vkDestroyPipeline(vulkanContextResources->resources->device, graphicsPipeline, nullptr);
                    graphicsPipeline = VK_NULL_HANDLE;
                }
            }
        }
    }
}

void DestroyAllDescLayouts(VulkanContextResources* vulkanContextResources)
{
    for (uint32 desc = 0; desc < VkResources::eMaxDescLayouts; ++desc)
    {
        VkDescriptorSetLayout& descLayout = vulkanContextResources->resources->descLayouts[desc].layout;
        if (descLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(vulkanContextResources->resources->device, descLayout, nullptr);
            descLayout = VK_NULL_HANDLE;
        }
    }
}

void VulkanDestroyAllRenderPasses(VulkanContextResources* vulkanContextResources)
{
    for (uint32 rp = 0; rp < VkResources::eMaxRenderPasses; ++rp)
    {
        VkRenderPass& renderPass = vulkanContextResources->resources->renderPasses[rp].renderPassVk;
        if (renderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(vulkanContextResources->resources->device, renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }
    }
}

void* VulkanMapResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle)
{
    VulkanMemResource* resource =
        &vulkanContextResources->resources->vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[vulkanContextResources->resources->currentSwapChainImage];

    void* newMappedMem;
    VkResult result = vkMapMemory(vulkanContextResources->resources->device, resource->deviceMemory, 0, VK_WHOLE_SIZE, 0, &newMappedMem);

    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to map gpu memory!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
        return nullptr;
    }

    return newMappedMem;
}

void VulkanUnmapResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle)
{
    VulkanMemResource* resource =
        &vulkanContextResources->resources->vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[vulkanContextResources->resources->currentSwapChainImage];

    // Flush right before unmapping
    VkMappedMemoryRange memoryRange = {};
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.memory = resource->deviceMemory;
    memoryRange.offset = 0;
    memoryRange.size = VK_WHOLE_SIZE;

    VkResult result = vkFlushMappedMemoryRanges(vulkanContextResources->resources->device, 1, &memoryRange);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to flush mapped gpu memory!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    vkUnmapMemory(vulkanContextResources->resources->device, resource->deviceMemory);
}

ResourceHandle VulkanCreateBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes, uint32 bufferUsage)
{
    uint32 newResourceHandle =
        vulkanContextResources->resources->vulkanMemResourcePool.Alloc();
    TINKER_ASSERT(newResourceHandle != TINKER_INVALID_HANDLE);
    VulkanMemResourceChain* newResourceChain = vulkanContextResources->resources->vulkanMemResourcePool.PtrFromHandle(newResourceHandle);
    *newResourceChain = {};

    // TODO: do or do not have a resource chain for buffers.
    bool oneBufferOnly = false;
    bool perSwapChainSize = false;

    for (uint32 uiImage = 0; uiImage < vulkanContextResources->resources->numSwapChainImages && !oneBufferOnly; ++uiImage)
    {
        VulkanMemResource* newResource = &newResourceChain->resourceChain[uiImage];

        VkBufferUsageFlags usageFlags = {};
        VkMemoryPropertyFlagBits propertyFlags = {};

        switch (bufferUsage)
        {
            case BufferUsage::eVertex:
            {
                usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; // vertex buffers are actually SSBOs for now
                propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                break;
            }

            case BufferUsage::eIndex:
            {
                //oneBufferOnly = true;
                //perSwapChainSize = true;
                usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                break;
            }

            case BufferUsage::eTransientVertex:
            {
                usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; // vertex buffers are actually SSBOs for now
                propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                break;
            }

            case BufferUsage::eTransientIndex:
            {
                usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                break;
            }

            case BufferUsage::eStaging:
            {
                oneBufferOnly = true;
                perSwapChainSize = false;
                usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                break;
            }

            case BufferUsage::eUniform:
            {
                usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                break;
            }

            default:
            {
                Core::Utility::LogMsg("Platform", "Invalid buffer usage specified!", Core::Utility::LogSeverity::eCritical);
                TINKER_ASSERT(0);
                break;
            }
        }

        // allocate buffer resource size in bytes equal to one buffer per swap chain image
        uint32 numBytesToAllocate = perSwapChainSize ? sizeInBytes * vulkanContextResources->resources->numSwapChainImages : sizeInBytes;

        CreateBuffer(vulkanContextResources->resources->physicalDevice,
            vulkanContextResources->resources->device,
            numBytesToAllocate,
            usageFlags,
            propertyFlags,
            newResource->buffer,
            newResource->deviceMemory);
    }

    return ResourceHandle(newResourceHandle);
}

DescriptorHandle VulkanCreateDescriptor(VulkanContextResources* vulkanContextResources, uint32 descriptorLayoutID)
{
    if (vulkanContextResources->resources->descriptorPool == VK_NULL_HANDLE)
    {
        // Allocate the descriptor pool
        InitDescriptorPool(vulkanContextResources);
    }
    
    uint32 newDescriptorHandle = vulkanContextResources->resources->vulkanDescriptorResourcePool.Alloc();

    for (uint32 uiImage = 0; uiImage < vulkanContextResources->resources->numSwapChainImages; ++uiImage)
    {
        const VkDescriptorSetLayout& descriptorSetLayout = vulkanContextResources->resources->descLayouts[descriptorLayoutID].layout;
        TINKER_ASSERT(descriptorSetLayout != VK_NULL_HANDLE);

        if (descriptorSetLayout != VK_NULL_HANDLE)
        {
            VkDescriptorSetAllocateInfo descSetAllocInfo = {};
            descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descSetAllocInfo.descriptorPool = vulkanContextResources->resources->descriptorPool;
            descSetAllocInfo.descriptorSetCount = 1;
            descSetAllocInfo.pSetLayouts = &descriptorSetLayout;

            VkResult result = vkAllocateDescriptorSets(vulkanContextResources->resources->device,
                &descSetAllocInfo,
                &vulkanContextResources->resources->vulkanDescriptorResourcePool.PtrFromHandle(newDescriptorHandle)->resourceChain[uiImage].descriptorSet);
            if (result != VK_SUCCESS)
            {
                Core::Utility::LogMsg("Platform", "Failed to create Vulkan descriptor set!", Core::Utility::LogSeverity::eCritical);
                TINKER_ASSERT(0);
            }
        }
    }

    return DescriptorHandle(newDescriptorHandle);
}

void VulkanDestroyDescriptor(VulkanContextResources* vulkanContextResources, DescriptorHandle handle)
{
    vkDeviceWaitIdle(vulkanContextResources->resources->device); // TODO: move this?
    for (uint32 uiImage = 0; uiImage < vulkanContextResources->resources->numSwapChainImages; ++uiImage)
    {
        // TODO: destroy something?
    }
    vulkanContextResources->resources->vulkanDescriptorResourcePool.Dealloc(handle.m_hDesc);
}

void VulkanDestroyAllDescriptors(VulkanContextResources* vulkanContextResources)
{
    vkDestroyDescriptorPool(vulkanContextResources->resources->device, vulkanContextResources->resources->descriptorPool, nullptr);
    vulkanContextResources->resources->descriptorPool = VK_NULL_HANDLE;
}

void VulkanWriteDescriptor(VulkanContextResources* vulkanContextResources, uint32 descriptorLayoutID, DescriptorHandle* descSetHandles, uint32 descSetCount, DescriptorSetDataHandles* descSetDataHandles, uint32 descSetDataCount)
{
    TINKER_ASSERT(descSetCount <= MAX_DESCRIPTOR_SETS_PER_SHADER);
    TINKER_ASSERT(descSetDataCount <= MAX_BINDINGS_PER_SET);

    DescriptorLayout* descLayout = &vulkanContextResources->resources->descLayouts[descriptorLayoutID].bindings;

    for (uint32 uiImage = 0; uiImage < vulkanContextResources->resources->numSwapChainImages; ++uiImage)
    {
        for (uint32 uiSet = 0; uiSet < descSetCount; ++uiSet)
        {
            if (descSetHandles[uiSet] == DefaultDescHandle_Invalid) continue;
            VkDescriptorSet* descriptorSet =
                &vulkanContextResources->resources->vulkanDescriptorResourcePool.PtrFromHandle(descSetHandles[uiSet].m_hDesc)->resourceChain[uiImage].descriptorSet;

            // Descriptor layout
            VkWriteDescriptorSet descSetWrites[MAX_BINDINGS_PER_SET] = {};
            uint32 descriptorCount = 0;

            // Desc set info data
            VkDescriptorBufferInfo descBufferInfo[MAX_BINDINGS_PER_SET] = {};
            VkDescriptorImageInfo descImageInfo[MAX_BINDINGS_PER_SET] = {};

            for (uint32 uiDesc = 0; uiDesc < MAX_BINDINGS_PER_SET; ++uiDesc)
            {
                descSetWrites[uiDesc].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

                uint32 type = descLayout->params[uiDesc].type;
                if (type != DescriptorType::eMax)
                {
                    VulkanMemResource* res = &vulkanContextResources->resources->vulkanMemResourcePool.PtrFromHandle(descSetDataHandles[uiSet].handles[uiDesc].m_hRes)->resourceChain[uiImage];

                    switch (type)
                    {
                        case DescriptorType::eBuffer:
                        case DescriptorType::eSSBO:
                        {
                            VkBuffer* buffer = &res->buffer;

                            descBufferInfo[descriptorCount].buffer = *buffer;
                            descBufferInfo[descriptorCount].offset = 0;
                            descBufferInfo[descriptorCount].range = VK_WHOLE_SIZE;

                            descSetWrites[descriptorCount].dstSet = *descriptorSet;
                            descSetWrites[descriptorCount].dstBinding = descriptorCount;
                            descSetWrites[descriptorCount].dstArrayElement = 0;
                            descSetWrites[descriptorCount].descriptorType = GetVkDescriptorType(type);
                            descSetWrites[descriptorCount].descriptorCount = 1;
                            descSetWrites[descriptorCount].pBufferInfo = &descBufferInfo[descriptorCount];
                            break;
                        }

                        case DescriptorType::eSampledImage:
                        {
                            VkImageView* imageView = &res->imageView;

                            descImageInfo[descriptorCount].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            descImageInfo[descriptorCount].imageView = *imageView;
                            descImageInfo[descriptorCount].sampler = vulkanContextResources->resources->linearSampler;

                            descSetWrites[descriptorCount].dstSet = *descriptorSet;
                            descSetWrites[descriptorCount].dstBinding = descriptorCount;
                            descSetWrites[descriptorCount].dstArrayElement = 0;
                            descSetWrites[descriptorCount].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                            descSetWrites[descriptorCount].descriptorCount = 1;
                            descSetWrites[descriptorCount].pImageInfo = &descImageInfo[descriptorCount];
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

            if (descriptorCount > 0)
            {
                vkUpdateDescriptorSets(vulkanContextResources->resources->device, descriptorCount, descSetWrites, 0, nullptr);
            }

        }
    }
}

void InitDescriptorPool(VulkanContextResources* vulkanContextResources)
{
    VkDescriptorPoolSize descPoolSizes[VULKAN_NUM_SUPPORTED_DESCRIPTOR_TYPES] = {};
    descPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descPoolSizes[0].descriptorCount = VULKAN_DESCRIPTOR_POOL_MAX_UNIFORM_BUFFERS;
    descPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descPoolSizes[1].descriptorCount = VULKAN_DESCRIPTOR_POOL_MAX_SAMPLED_IMAGES;
    descPoolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descPoolSizes[2].descriptorCount = VULKAN_DESCRIPTOR_POOL_MAX_STORAGE_BUFFERS;

    VkDescriptorPoolCreateInfo descPoolCreateInfo = {};
    descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolCreateInfo.poolSizeCount = VULKAN_NUM_SUPPORTED_DESCRIPTOR_TYPES;
    descPoolCreateInfo.pPoolSizes = descPoolSizes;
    descPoolCreateInfo.maxSets = VULKAN_DESCRIPTOR_POOL_MAX_UNIFORM_BUFFERS + VULKAN_DESCRIPTOR_POOL_MAX_SAMPLED_IMAGES;

    VkResult result = vkCreateDescriptorPool(vulkanContextResources->resources->device, &descPoolCreateInfo, nullptr, &vulkanContextResources->resources->descriptorPool);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create descriptor pool!", Core::Utility::LogSeverity::eInfo);
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

    VkResult result = vkCreateSampler(vulkanContextResources->resources->device, &samplerCreateInfo, nullptr, &vulkanContextResources->resources->linearSampler);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create sampler!", Core::Utility::LogSeverity::eCritical);
        return;
    }
}

bool VulkanCreateRenderPass(VulkanContextResources* vulkanContextResources, uint32 renderPassID, uint32 numColorRTs, uint32 colorFormat, uint32 startLayout, uint32 endLayout, uint32 depthFormat)
{
    VulkanRenderPass& renderPass = vulkanContextResources->resources->renderPasses[renderPassID];
    CreateRenderPass(vulkanContextResources->resources->device, numColorRTs, GetVkImageFormat(colorFormat), GetVkImageLayout(startLayout), GetVkImageLayout(endLayout), GetVkImageFormat(depthFormat), &renderPass.renderPassVk);
    renderPass.numColorRTs = numColorRTs;
    renderPass.hasDepth = (depthFormat != ImageFormat::Invalid);
    return true;
}

FramebufferHandle VulkanCreateFramebuffer(VulkanContextResources* vulkanContextResources,
    ResourceHandle* rtColorHandles, uint32 numRTColorHandles, ResourceHandle rtDepthHandle,
    uint32 width, uint32 height, uint32 renderPassID)
{
    TINKER_ASSERT(numRTColorHandles <= VULKAN_MAX_RENDERTARGETS);

    bool hasDepth = rtDepthHandle != DefaultResHandle_Invalid;

    // Alloc handle
    uint32 newFramebufferHandle = vulkanContextResources->resources->vulkanFramebufferResourcePool.Alloc();
    TINKER_ASSERT(newFramebufferHandle != TINKER_INVALID_HANDLE);

    VkImageView* attachments = nullptr;
    if (numRTColorHandles > 0)
    {
        attachments = new VkImageView[numRTColorHandles];
    }

    for (uint32 uiImage = 0; uiImage < vulkanContextResources->resources->numSwapChainImages; ++uiImage)
    {
        VulkanFramebufferResource* newFramebuffer =
            &vulkanContextResources->resources->vulkanFramebufferResourcePool.PtrFromHandle(newFramebufferHandle)->resourceChain[uiImage];

        // Create framebuffer
        for (uint32 uiImageView = 0; uiImageView < numRTColorHandles; ++uiImageView)
        {
            attachments[uiImageView] =
                vulkanContextResources->resources->vulkanMemResourcePool.PtrFromHandle(rtColorHandles[uiImageView].m_hRes)->resourceChain[uiImage].imageView;
        }

        VkImageView depthImageView = VK_NULL_HANDLE;
        if (hasDepth)
        {
            depthImageView = vulkanContextResources->resources->vulkanMemResourcePool.PtrFromHandle(rtDepthHandle.m_hRes)->resourceChain[uiImage].imageView;
        }

        CreateFramebuffer(vulkanContextResources->resources->device, attachments, numRTColorHandles, depthImageView, width, height, vulkanContextResources->resources->renderPasses[renderPassID].renderPassVk, &newFramebuffer->framebuffer);

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

ResourceHandle VulkanCreateImageResource(VulkanContextResources* vulkanContextResources, uint32 imageFormat, uint32 width, uint32 height, uint32 numArrayEles)
{
    uint32 newResourceHandle = vulkanContextResources->resources->vulkanMemResourcePool.Alloc();

    for (uint32 uiImage = 0; uiImage < vulkanContextResources->resources->numSwapChainImages; ++uiImage)
    {
        VulkanMemResourceChain* newResourceChain =
            vulkanContextResources->resources->vulkanMemResourcePool.PtrFromHandle(newResourceHandle);
        VulkanMemResource* newResource = &newResourceChain->resourceChain[uiImage];

        // Create image
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = width;
        imageCreateInfo.extent.height = height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = numArrayEles;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.format = GetVkImageFormat(imageFormat);
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        switch (imageFormat)
        {
            case ImageFormat::BGRA8_SRGB:
            case ImageFormat::RGBA8_SRGB:
            {
                imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // TODO: make this a parameter?
                break;
            }

            case ImageFormat::Depth_32F:
            {
                imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                break;
            }

            case ImageFormat::Invalid:
            default:
            {
                Core::Utility::LogMsg("Platform", "Invalid image resource format specified!", Core::Utility::LogSeverity::eCritical);
                TINKER_ASSERT(0);
                break;
            }
        }

        VkResult result = vkCreateImage(vulkanContextResources->resources->device,
            &imageCreateInfo,
            nullptr,
            &newResource->image);

        VkMemoryRequirements memRequirements = {};
        vkGetImageMemoryRequirements(vulkanContextResources->resources->device, newResource->image, &memRequirements);
        AllocGPUMemory(vulkanContextResources->resources->physicalDevice,
            vulkanContextResources->resources->device,
            &newResource->deviceMemory,
            memRequirements,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vkBindImageMemory(vulkanContextResources->resources->device, newResource->image, newResource->deviceMemory, 0);

        // Create image view
        VkImageAspectFlags aspectMask = {};
        switch (imageFormat)
        {
            case ImageFormat::BGRA8_SRGB:
            case ImageFormat::RGBA8_SRGB:
            {
                aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
                break;
            }

            case ImageFormat::Depth_32F:
            {
                aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
                break;
            }

            case ImageFormat::Invalid:
            default:
            {
                Core::Utility::LogMsg("Platform", "Invalid image resource format specified!", Core::Utility::LogSeverity::eCritical);
                TINKER_ASSERT(0);
                break;
            }
        }

        CreateImageView(vulkanContextResources->resources->device,
            GetVkImageFormat(imageFormat),
            aspectMask,
            newResource->image,
            &newResource->imageView,
            numArrayEles);
    }

    return ResourceHandle(newResourceHandle);
}

ResourceHandle VulkanCreateResource(VulkanContextResources* vulkanContextResources, const ResourceDesc& resDesc)
{
    ResourceHandle newHandle = DefaultResHandle_Invalid;

    switch (resDesc.resourceType)
    {
        case ResourceType::eBuffer1D:
        {
            newHandle = VulkanCreateBuffer(vulkanContextResources, resDesc.dims.x, resDesc.bufferUsage);
            break;
        }

        case ResourceType::eImage2D:
        {
            newHandle = VulkanCreateImageResource(vulkanContextResources, resDesc.imageFormat, resDesc.dims.x, resDesc.dims.y, resDesc.arrayEles);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid resource description type!", Core::Utility::LogSeverity::eCritical);
            return newHandle;
        }
    }

    // Set the internal resdesc to the one provided
    VulkanMemResourceChain* newResourceChain = vulkanContextResources->resources->vulkanMemResourcePool.PtrFromHandle(newHandle.m_hRes);
    newResourceChain->resDesc = resDesc;
    
    return newHandle;
}

void VulkanDestroyImageResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle)
{
    vkDeviceWaitIdle(vulkanContextResources->resources->device); // TODO: move this?

    for (uint32 uiImage = 0; uiImage < vulkanContextResources->resources->numSwapChainImages; ++uiImage)
    {
        VulkanMemResource* resource = &vulkanContextResources->resources->vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[uiImage];
        vkDestroyImage(vulkanContextResources->resources->device, resource->image, nullptr);
        vkFreeMemory(vulkanContextResources->resources->device, resource->deviceMemory, nullptr);
        vkDestroyImageView(vulkanContextResources->resources->device, resource->imageView, nullptr);
    }

    vulkanContextResources->resources->vulkanMemResourcePool.Dealloc(handle.m_hRes);
}

void VulkanDestroyFramebuffer(VulkanContextResources* vulkanContextResources, FramebufferHandle handle)
{
    vkDeviceWaitIdle(vulkanContextResources->resources->device); // TODO: move this?

    for (uint32 uiImage = 0; uiImage < vulkanContextResources->resources->numSwapChainImages; ++uiImage)
    {
        VulkanFramebufferResource* framebuffer = &vulkanContextResources->resources->vulkanFramebufferResourcePool.PtrFromHandle(handle.m_hFramebuffer)->resourceChain[uiImage];
        vkDestroyFramebuffer(vulkanContextResources->resources->device, framebuffer->framebuffer, nullptr);
    }
    
    vulkanContextResources->resources->vulkanFramebufferResourcePool.Dealloc(handle.m_hFramebuffer);
}

void VulkanDestroyBuffer(VulkanContextResources* vulkanContextResources, ResourceHandle handle, uint32 bufferUsage)
{
    vkDeviceWaitIdle(vulkanContextResources->resources->device); // TODO: move this?

    for (uint32 uiImage = 0; uiImage < vulkanContextResources->resources->numSwapChainImages; ++uiImage)
    {
        VulkanMemResource* resource = &vulkanContextResources->resources->vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[uiImage];

        // Only destroy/free memory if it's a validly created buffer
        if (resource->buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(vulkanContextResources->resources->device, resource->buffer, nullptr);
            vkFreeMemory(vulkanContextResources->resources->device, resource->deviceMemory, nullptr);
        }
        else
        {
            // TODO: this is currently happening since vertex buffers are only alloc'd once, but
            // currently the architecture stores a chain of buffer objects.
            //Core::Utility::LogMsg("Platform", "Freeing a null buffer", Core::Utility::LogSeverity::eWarning);
        }
    }

    vulkanContextResources->resources->vulkanMemResourcePool.Dealloc(handle.m_hRes);
}

void VulkanDestroyResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle)
{
    vkDeviceWaitIdle(vulkanContextResources->resources->device); // TODO: move this?

    VulkanMemResourceChain* resourceChain = vulkanContextResources->resources->vulkanMemResourcePool.PtrFromHandle(handle.m_hRes);

    switch (resourceChain->resDesc.resourceType)
    {
        case ResourceType::eBuffer1D:
        {
            VulkanDestroyBuffer(vulkanContextResources, handle, resourceChain->resDesc.bufferUsage);
            break;
        }

        case ResourceType::eImage2D:
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

    vkDeviceWaitIdle(vulkanContextResources->resources->device); // TODO: move this?

    VulkanDestroySwapChain(vulkanContextResources);

    vkDestroyCommandPool(vulkanContextResources->resources->device, vulkanContextResources->resources->commandPool, nullptr);
    delete vulkanContextResources->resources->commandBuffers;
    vulkanContextResources->resources->commandBuffers = nullptr;

    VulkanDestroyAllPSOPerms(vulkanContextResources);
    DestroyAllDescLayouts(vulkanContextResources);
    VulkanDestroyAllRenderPasses(vulkanContextResources);

    for (uint32 uiImage = 0; uiImage < VULKAN_MAX_FRAMES_IN_FLIGHT; ++uiImage)
    {
        vkDestroySemaphore(vulkanContextResources->resources->device, vulkanContextResources->resources->renderCompleteSemaphores[uiImage], nullptr);
        vkDestroySemaphore(vulkanContextResources->resources->device, vulkanContextResources->resources->swapChainImageAvailableSemaphores[uiImage], nullptr);
        vkDestroyFence(vulkanContextResources->resources->device, vulkanContextResources->resources->fences[uiImage], nullptr);
    }
    delete vulkanContextResources->resources->imageInFlightFences;

    vkDestroySampler(vulkanContextResources->resources->device, vulkanContextResources->resources->linearSampler, nullptr);

    #if defined(ENABLE_VULKAN_VALIDATION_LAYERS)
    // Debug utils messenger
    PFN_vkDestroyDebugUtilsMessengerEXT dbgDestroyFunc =
        (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vulkanContextResources->resources->instance,
                                                                    "vkDestroyDebugUtilsMessengerEXT");
    if (dbgDestroyFunc)
    {
        dbgDestroyFunc(vulkanContextResources->resources->instance, vulkanContextResources->resources->debugMessenger, nullptr);
    }
    else
    {
        Core::Utility::LogMsg("Platform", "Failed to get destroy debug utils messenger proc addr!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
    #endif

    vkDestroyDevice(vulkanContextResources->resources->device, nullptr);
    vkDestroySurfaceKHR(vulkanContextResources->resources->instance, vulkanContextResources->resources->surface, nullptr);
    vkDestroyInstance(vulkanContextResources->resources->instance, nullptr);

    vulkanContextResources->resources->vulkanMemResourcePool.ExplicitFree();
    vulkanContextResources->resources->vulkanDescriptorResourcePool.ExplicitFree();
    vulkanContextResources->resources->vulkanFramebufferResourcePool.ExplicitFree();

    delete vulkanContextResources->resources;
    vulkanContextResources->resources = nullptr;
}

}
}
}

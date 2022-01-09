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
namespace Core
{
namespace Graphics
{

static const uint32 VULKAN_SCRATCH_MEM_SIZE = 1024 * 1024 * 2;
static Tk::Core::LinearAllocator<VULKAN_SCRATCH_MEM_SIZE> g_VulkanDataAllocator;

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

int InitVulkan(const Tk::Platform::PlatformWindowHandles* platformWindowHandles, uint32 width, uint32 height)
{
    g_vulkanContextResources.vulkanMemResourcePool.Init(VULKAN_RESOURCE_POOL_MAX, 16);
    g_vulkanContextResources.vulkanDescriptorResourcePool.Init(VULKAN_RESOURCE_POOL_MAX, 16);
    g_vulkanContextResources.vulkanFramebufferResourcePool.Init(VULKAN_RESOURCE_POOL_MAX, 16);

    g_vulkanContextResources.windowWidth = width;
    g_vulkanContextResources.windowHeight = height;

    InitVulkanDataTypesPerEnum();

    // Shader pso permutations
    for (uint32 sid = 0; sid < VulkanContextResources::eMaxShaders; ++sid)
    {
        VkPipelineLayout& pipelineLayout = g_vulkanContextResources.psoPermutations.pipelineLayout[sid];
        pipelineLayout = VK_NULL_HANDLE;

        for (uint32 bs = 0; bs < VulkanContextResources::eMaxBlendStates; ++bs)
        {
            for (uint32 ds = 0; ds < VulkanContextResources::eMaxDepthStates; ++ds)
            {
                VkPipeline& graphicsPipeline = g_vulkanContextResources.psoPermutations.graphicsPipeline[sid][bs][ds];
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
    
    VkExtensionProperties* availableExtensions = (VkExtensionProperties*)g_VulkanDataAllocator.Alloc(sizeof(VkExtensionProperties) * numAvailableExtensions, 1);
    vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions, availableExtensions);
    Core::Utility::LogMsg("Platform", "******** Available Instance Extensions: ********", Core::Utility::LogSeverity::eInfo);
    for (uint32 uiAvailExt = 0; uiAvailExt < numAvailableExtensions; ++uiAvailExt)
    {
        Core::Utility::LogMsg("Platform", availableExtensions[uiAvailExt].extensionName, Core::Utility::LogSeverity::eInfo);
    }

    // Validation layers
    #if defined(ENABLE_VULKAN_VALIDATION_LAYERS)
    const uint32 numRequestedLayers = 2;
    const char* requestedLayersStr[numRequestedLayers] =
    {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_monitor"
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

    VkLayerProperties* availableLayers = (VkLayerProperties*)g_VulkanDataAllocator.Alloc(sizeof(VkLayerProperties) * numAvailableLayers, 1);
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
        &g_vulkanContextResources.instance);
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
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_vulkanContextResources.instance,
                                                                  "vkCreateDebugUtilsMessengerEXT");
    if (dbgCreateFunc)
    {
        dbgCreateFunc(g_vulkanContextResources.instance,
            &dbgUtilsMsgCreateInfo,
            nullptr,
            &g_vulkanContextResources.debugMessenger);
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

    result = vkCreateWin32SurfaceKHR(g_vulkanContextResources.instance,
        &win32SurfaceCreateInfo,
        NULL,
        &g_vulkanContextResources.surface);
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
    vkEnumeratePhysicalDevices(g_vulkanContextResources.instance, &numPhysicalDevices, nullptr);

    if (numPhysicalDevices == 0)
    {
        Core::Utility::LogMsg("Platform", "Zero available physical devices!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkPhysicalDevice* physicalDevices = (VkPhysicalDevice*)g_VulkanDataAllocator.Alloc(sizeof(VkPhysicalDevice) * numPhysicalDevices, 1);
    vkEnumeratePhysicalDevices(g_vulkanContextResources.instance, &numPhysicalDevices, physicalDevices);

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

            VkQueueFamilyProperties* queueFamilyProperties = (VkQueueFamilyProperties*)g_VulkanDataAllocator.Alloc(sizeof(VkQueueFamilyProperties) * numQueueFamilies, 1);
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
                    g_vulkanContextResources.graphicsQueueIndex = uiQueueFamily;
                }

                VkBool32 presentSupport;
                vkGetPhysicalDeviceSurfaceSupportKHR(currPhysicalDevice,
                    uiQueueFamily,
                    g_vulkanContextResources.surface,
                    &presentSupport);
                if (presentSupport)
                {
                    presentationSupport = true;
                    g_vulkanContextResources.presentationQueueIndex = uiQueueFamily;
                }
            }

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

            VkExtensionProperties* availablePhysicalDeviceExtensions = (VkExtensionProperties*)g_VulkanDataAllocator.Alloc(sizeof(VkExtensionProperties) * numAvailablePhysicalDeviceExtensions, 1);
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
                g_vulkanContextResources.physicalDevice = currPhysicalDevice;
                break;
            }
        }
    }

    if (g_vulkanContextResources.physicalDevice == VK_NULL_HANDLE)
    {
        Core::Utility::LogMsg("Platform", "No physical device chosen!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    // Physical device memory heaps
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    vkGetPhysicalDeviceMemoryProperties(g_vulkanContextResources.physicalDevice, &memoryProperties);
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
    deviceQueueCreateInfos[0].queueFamilyIndex = g_vulkanContextResources.graphicsQueueIndex;
    deviceQueueCreateInfos[0].queueCount = 1;
    float graphicsQueuePriority = 1.0f;
    deviceQueueCreateInfos[0].pQueuePriorities = &graphicsQueuePriority;

    // Create presentation queue
    deviceQueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfos[1].queueFamilyIndex = g_vulkanContextResources.presentationQueueIndex;
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

    result = vkCreateDevice(g_vulkanContextResources.physicalDevice,
        &deviceCreateInfo,
        nullptr,
        &g_vulkanContextResources.device);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan device!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    // Debug labels
    #if defined(ENABLE_VULKAN_DEBUG_LABELS)
    /*g_vulkanContextResources.pfnSetDebugUtilsObjectNameEXT =
        (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(g_vulkanContextResources.device,
                                                              "vkSetDebugUtilsObjectNameEXT")*/;
    g_vulkanContextResources.pfnCmdBeginDebugUtilsLabelEXT =
        (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(g_vulkanContextResources.device,
                                                               "vkCmdBeginDebugUtilsLabelEXT");
    if (!g_vulkanContextResources.pfnCmdBeginDebugUtilsLabelEXT)
    {
        Core::Utility::LogMsg("Platform", "Failed to get create debug utils begin label proc addr!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    g_vulkanContextResources.pfnCmdEndDebugUtilsLabelEXT =
        (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(g_vulkanContextResources.device,
                                                               "vkCmdEndDebugUtilsLabelEXT");
    if (!g_vulkanContextResources.pfnCmdEndDebugUtilsLabelEXT)
    {
        Core::Utility::LogMsg("Platform", "Failed to get create debug utils end label proc addr!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    g_vulkanContextResources.pfnCmdInsertDebugUtilsLabelEXT =
        (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetDeviceProcAddr(g_vulkanContextResources.device,
                                                               "vkCmdInsertDebugUtilsLabelEXT");
    if (!g_vulkanContextResources.pfnCmdInsertDebugUtilsLabelEXT)
    {
        Core::Utility::LogMsg("Platform", "Failed to get create debug utils insert label proc addr!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
    #endif

    // Queues
    vkGetDeviceQueue(g_vulkanContextResources.device,
        g_vulkanContextResources.graphicsQueueIndex,
        0,
        &g_vulkanContextResources.graphicsQueue);
    vkGetDeviceQueue(g_vulkanContextResources.device,
        g_vulkanContextResources.presentationQueueIndex,
        0, 
        &g_vulkanContextResources.presentationQueue);

    // Swap chain
    VulkanCreateSwapChain();

    // Command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = g_vulkanContextResources.graphicsQueueIndex;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                  VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    result = vkCreateCommandPool(g_vulkanContextResources.device,
        &commandPoolCreateInfo,
        nullptr,
        &g_vulkanContextResources.commandPool);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan command pool!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    // Command buffers for per-frame submission
    g_vulkanContextResources.commandBuffers = (VkCommandBuffer*)g_VulkanDataAllocator.Alloc(sizeof(VkCommandBuffer) * g_vulkanContextResources.numSwapChainImages, 1);

    VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.commandPool = g_vulkanContextResources.commandPool;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandBufferCount = g_vulkanContextResources.numSwapChainImages;

    result = vkAllocateCommandBuffers(g_vulkanContextResources.device,
        &commandBufferAllocInfo,
        g_vulkanContextResources.commandBuffers);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to allocate Vulkan primary frame command buffers!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    // Command buffer for immediate submission
    commandBufferAllocInfo = {};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandPool = g_vulkanContextResources.commandPool;
    commandBufferAllocInfo.commandBufferCount = 1;
    result = vkAllocateCommandBuffers(g_vulkanContextResources.device, &commandBufferAllocInfo, &g_vulkanContextResources.commandBuffer_Immediate);
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
        result = vkCreateSemaphore(g_vulkanContextResources.device,
            &semaphoreCreateInfo,
            nullptr,
            &g_vulkanContextResources.swapChainImageAvailableSemaphores[uiImage]);
        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Platform", "Failed to create Vulkan semaphore!", Core::Utility::LogSeverity::eCritical);
        }

        result = vkCreateSemaphore(g_vulkanContextResources.device,
            &semaphoreCreateInfo,
            nullptr,
            &g_vulkanContextResources.renderCompleteSemaphores[uiImage]);
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
        result = vkCreateFence(g_vulkanContextResources.device, &fenceCreateInfo, nullptr, &g_vulkanContextResources.fences[uiImage]);
        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Platform", "Failed to create Vulkan fence!", Core::Utility::LogSeverity::eCritical);
        }
    }
    
    g_vulkanContextResources.imageInFlightFences = (VkFence*)g_VulkanDataAllocator.Alloc(sizeof(VkFence) * g_vulkanContextResources.numSwapChainImages, 1);
    for (uint32 uiImage = 0; uiImage < g_vulkanContextResources.numSwapChainImages; ++uiImage)
    {
        g_vulkanContextResources.imageInFlightFences[uiImage] = VK_NULL_HANDLE;
    }

    CreateSamplers();

    g_vulkanContextResources.isInitted = true;

    return 0;
}

void VulkanCreateSwapChain()
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_vulkanContextResources.physicalDevice,
        g_vulkanContextResources.surface,
        &capabilities);

    VkExtent2D optimalExtent = {};
    if (capabilities.currentExtent.width != 0xffffffff)
    {
        optimalExtent = capabilities.currentExtent;
    }
    else
    {
        optimalExtent.width =
            CLAMP(g_vulkanContextResources.windowWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        optimalExtent.height =
            CLAMP(g_vulkanContextResources.windowHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    uint32 numSwapChainImages = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) // 0 can indicate no maximum
    {
        numSwapChainImages = min(numSwapChainImages, capabilities.maxImageCount);
    }
    numSwapChainImages = min(numSwapChainImages, VULKAN_MAX_SWAP_CHAIN_IMAGES);

    uint32 numAvailableSurfaceFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(g_vulkanContextResources.physicalDevice,
        g_vulkanContextResources.surface,
        &numAvailableSurfaceFormats,
        nullptr);

    if (numAvailableSurfaceFormats == 0)
    {
        Core::Utility::LogMsg("Platform", "Zero available Vulkan swap chain surface formats!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkSurfaceFormatKHR* availableSurfaceFormats = (VkSurfaceFormatKHR*)g_VulkanDataAllocator.Alloc(sizeof(VkSurfaceFormatKHR) * numAvailableSurfaceFormats, 1);
    vkGetPhysicalDeviceSurfaceFormatsKHR(g_vulkanContextResources.physicalDevice,
        g_vulkanContextResources.surface,
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

    uint32 numAvailablePresentModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(g_vulkanContextResources.physicalDevice,
        g_vulkanContextResources.surface,
        &numAvailablePresentModes,
        nullptr);

    if (numAvailablePresentModes == 0)
    {
        Core::Utility::LogMsg("Platform", "Zero available Vulkan swap chain present modes!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkPresentModeKHR* availablePresentModes = (VkPresentModeKHR*)g_VulkanDataAllocator.Alloc(sizeof(VkPresentModeKHR) * numAvailablePresentModes, 1);
    vkGetPhysicalDeviceSurfacePresentModesKHR(g_vulkanContextResources.physicalDevice,
        g_vulkanContextResources.surface,
        &numAvailablePresentModes,
        availablePresentModes);

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR; // required to be supported!
    for (uint32 uiAvailPresMode = 0; uiAvailPresMode < numAvailablePresentModes; ++uiAvailPresMode)
    {
        /*if (availablePresentModes[uiAvailPresMode] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            chosenPresentMode = availablePresentModes[uiAvailPresMode];
            break;
        }*/
    }

    g_vulkanContextResources.swapChainExtent = optimalExtent;
    g_vulkanContextResources.swapChainFormat = chosenFormat.format;
    g_vulkanContextResources.numSwapChainImages = numSwapChainImages;

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = g_vulkanContextResources.surface;
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
        g_vulkanContextResources.graphicsQueueIndex,
        g_vulkanContextResources.presentationQueueIndex
    };

    if (g_vulkanContextResources.graphicsQueueIndex != g_vulkanContextResources.presentationQueueIndex)
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

    VkResult result = vkCreateSwapchainKHR(g_vulkanContextResources.device,
        &swapChainCreateInfo,
        nullptr,
        &g_vulkanContextResources.swapChain);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan swap chain!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    vkGetSwapchainImagesKHR(g_vulkanContextResources.device,
        g_vulkanContextResources.swapChain,
        &numSwapChainImages,
        nullptr);

    for (uint32 i = 0; i < ARRAYCOUNT(g_vulkanContextResources.swapChainImages); ++i)
        g_vulkanContextResources.swapChainImages[i] = VK_NULL_HANDLE;
    vkGetSwapchainImagesKHR(g_vulkanContextResources.device,
        g_vulkanContextResources.swapChain,
        &numSwapChainImages,
        g_vulkanContextResources.swapChainImages);

    for (uint32 i = 0; i < ARRAYCOUNT(g_vulkanContextResources.swapChainImages); ++i)
        g_vulkanContextResources.swapChainImageViews[i] = VK_NULL_HANDLE;
    for (uint32 uiImageView = 0; uiImageView < numSwapChainImages; ++uiImageView)
    {
        CreateImageView(g_vulkanContextResources.device,
            g_vulkanContextResources.swapChainFormat,
            VK_IMAGE_ASPECT_COLOR_BIT,
            g_vulkanContextResources.swapChainImages[uiImageView],
            &g_vulkanContextResources.swapChainImageViews[uiImageView],
            1);
    }

    // Swap chain framebuffers
    const uint32 numColorRTs = 1; // TODO: support multiple RTs

    // Render pass
    VulkanRenderPass& renderPass = g_vulkanContextResources.renderPasses[RENDERPASS_ID_SWAP_CHAIN_BLIT];
    renderPass.numColorRTs = 1;
    renderPass.hasDepth = false;
    const VkFormat depthFormat = VK_FORMAT_UNDEFINED; // no depth buffer for swap chain
    CreateRenderPass(g_vulkanContextResources.device, numColorRTs, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, depthFormat, &renderPass.renderPassVk);

    uint32 newFramebufferHandle = g_vulkanContextResources.vulkanFramebufferResourcePool.Alloc();
    TINKER_ASSERT(newFramebufferHandle != TINKER_INVALID_HANDLE);
    g_vulkanContextResources.swapChainFramebufferHandle = FramebufferHandle(newFramebufferHandle);

    for (uint32 uiFramebuffer = 0; uiFramebuffer < g_vulkanContextResources.numSwapChainImages; ++uiFramebuffer)
    {
        VulkanFramebufferResource* newFramebuffer =
            &g_vulkanContextResources.vulkanFramebufferResourcePool.PtrFromHandle(newFramebufferHandle)->resourceChain[uiFramebuffer];

        for (uint32 i = 0; i < numColorRTs; ++i)
        {
            newFramebuffer->clearValues[i] = { 0.0f, 0.0f, 0.0f, 1.0f };
        }
        newFramebuffer->numClearValues = numColorRTs;

        // Framebuffer
        const VkImageView depthImageView = VK_NULL_HANDLE; // no depth buffer for swap chain
        CreateFramebuffer(g_vulkanContextResources.device, &g_vulkanContextResources.swapChainImageViews[uiFramebuffer], 1, depthImageView,
            g_vulkanContextResources.swapChainExtent.width, g_vulkanContextResources.swapChainExtent.height, g_vulkanContextResources.renderPasses[RENDERPASS_ID_SWAP_CHAIN_BLIT].renderPassVk, &newFramebuffer->framebuffer);
    }

    g_vulkanContextResources.isSwapChainValid = true;
}

void VulkanDestroySwapChain()
{
    g_vulkanContextResources.isSwapChainValid = false;
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    VulkanDestroyFramebuffer(g_vulkanContextResources.swapChainFramebufferHandle);

    for (uint32 uiImageView = 0; uiImageView < g_vulkanContextResources.numSwapChainImages; ++uiImageView)
    {
        vkDestroyImageView(g_vulkanContextResources.device, g_vulkanContextResources.swapChainImageViews[uiImageView], nullptr);
    }
    
    vkDestroySwapchainKHR(g_vulkanContextResources.device, g_vulkanContextResources.swapChain, nullptr);
}

bool VulkanCreateGraphicsPipeline(
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
        vertexShaderModule = CreateShaderModule((const char*)vertexShaderCode, numVertexShaderBytes, g_vulkanContextResources.device);
        ++numStages;
    }
    if (numFragmentShaderBytes > 0)
    {
        fragmentShaderModule = CreateShaderModule((const char*)fragmentShaderCode, numFragmentShaderBytes, g_vulkanContextResources.device);
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
            descriptorSetLayouts[uiDesc] = g_vulkanContextResources.descLayouts[descLayoutID].layout;
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

    VkPipelineLayout& pipelineLayout = g_vulkanContextResources.psoPermutations.pipelineLayout[shaderID];
    VkResult result = vkCreatePipelineLayout(g_vulkanContextResources.device,
        &pipelineLayoutInfo,
        nullptr,
        &pipelineLayout);

    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan pipeline layout!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    const VulkanRenderPass& renderPass = g_vulkanContextResources.renderPasses[renderPassID];
    for (uint32 blendState = 0; blendState < VulkanContextResources::eMaxBlendStates; ++blendState)
    {
        for (uint32 depthState = 0; depthState < VulkanContextResources::eMaxDepthStates; ++depthState)
        {
            VkPipeline& graphicsPipeline = g_vulkanContextResources.psoPermutations.graphicsPipeline[shaderID][blendState][depthState];

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

            result = vkCreateGraphicsPipelines(g_vulkanContextResources.device,
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

    vkDestroyShaderModule(g_vulkanContextResources.device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(g_vulkanContextResources.device, fragmentShaderModule, nullptr);

    return true;
}

bool VulkanCreateDescriptorLayout( uint32 descriptorLayoutID, const DescriptorLayout* descriptorLayout)
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
        newDescriptorHandle = g_vulkanContextResources.vulkanDescriptorResourcePool.Alloc();
    }
    else
    {
        Core::Utility::LogMsg("Platform", "No descriptors passed to VulkanCreateDescriptorLayout()!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkDescriptorSetLayout* descriptorSetLayout = &g_vulkanContextResources.descLayouts[descriptorLayoutID].layout;
    *descriptorSetLayout = VK_NULL_HANDLE;
    
    VkDescriptorSetLayoutCreateInfo descLayoutInfo = {};
    descLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descLayoutInfo.bindingCount = numBindings;
    descLayoutInfo.pBindings = &descLayoutBinding[0];

    VkResult result = vkCreateDescriptorSetLayout(g_vulkanContextResources.device, &descLayoutInfo, nullptr, descriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan descriptor set layout!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    memcpy(&g_vulkanContextResources.descLayouts[descriptorLayoutID].bindings, &descriptorLayout->params[0], sizeof(DescriptorLayout));
    return true;
}

void DestroyPSOPerms( uint32 shaderID)
{
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    VkPipelineLayout& pipelineLayout = g_vulkanContextResources.psoPermutations.pipelineLayout[shaderID];
    vkDestroyPipelineLayout(g_vulkanContextResources.device, pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;

    for (uint32 bs = 0; bs < VulkanContextResources::eMaxBlendStates; ++bs)
    {
        for (uint32 ds = 0; ds < VulkanContextResources::eMaxDepthStates; ++ds)
        {
            VkPipeline& graphicsPipeline = g_vulkanContextResources.psoPermutations.graphicsPipeline[shaderID][bs][ds];

            vkDestroyPipeline(g_vulkanContextResources.device, graphicsPipeline, nullptr);

            graphicsPipeline = VK_NULL_HANDLE;
        }
    }
}

void VulkanDestroyAllPSOPerms()
{
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    for (uint32 sid = 0; sid < VulkanContextResources::eMaxShaders; ++sid)
    {
        VkPipelineLayout& pipelineLayout = g_vulkanContextResources.psoPermutations.pipelineLayout[sid];
        if (pipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(g_vulkanContextResources.device, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }

        for (uint32 bs = 0; bs < VulkanContextResources::eMaxBlendStates; ++bs)
        {
            for (uint32 ds = 0; ds < VulkanContextResources::eMaxDepthStates; ++ds)
            {
                VkPipeline& graphicsPipeline = g_vulkanContextResources.psoPermutations.graphicsPipeline[sid][bs][ds];
                if (graphicsPipeline != VK_NULL_HANDLE)
                {
                    vkDestroyPipeline(g_vulkanContextResources.device, graphicsPipeline, nullptr);
                    graphicsPipeline = VK_NULL_HANDLE;
                }
            }
        }
    }
}

void DestroyAllDescLayouts()
{
    for (uint32 desc = 0; desc < VulkanContextResources::eMaxDescLayouts; ++desc)
    {
        VkDescriptorSetLayout& descLayout = g_vulkanContextResources.descLayouts[desc].layout;
        if (descLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(g_vulkanContextResources.device, descLayout, nullptr);
            descLayout = VK_NULL_HANDLE;
        }
    }
}

void VulkanDestroyAllRenderPasses()
{
    for (uint32 rp = 0; rp < VulkanContextResources::eMaxRenderPasses; ++rp)
    {
        VkRenderPass& renderPass = g_vulkanContextResources.renderPasses[rp].renderPassVk;
        if (renderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(g_vulkanContextResources.device, renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }
    }
}

void* VulkanMapResource( ResourceHandle handle)
{
    VulkanMemResource* resource =
        &g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[g_vulkanContextResources.currentSwapChainImage];

    void* newMappedMem;
    VkResult result = vkMapMemory(g_vulkanContextResources.device, resource->deviceMemory, 0, VK_WHOLE_SIZE, 0, &newMappedMem);

    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to map gpu memory!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
        return nullptr;
    }

    return newMappedMem;
}

void VulkanUnmapResource( ResourceHandle handle)
{
    VulkanMemResource* resource =
        &g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[g_vulkanContextResources.currentSwapChainImage];

    // Flush right before unmapping
    VkMappedMemoryRange memoryRange = {};
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.memory = resource->deviceMemory;
    memoryRange.offset = 0;
    memoryRange.size = VK_WHOLE_SIZE;

    VkResult result = vkFlushMappedMemoryRanges(g_vulkanContextResources.device, 1, &memoryRange);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to flush mapped gpu memory!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    vkUnmapMemory(g_vulkanContextResources.device, resource->deviceMemory);
}

ResourceHandle VulkanCreateBuffer( uint32 sizeInBytes, uint32 bufferUsage)
{
    uint32 newResourceHandle =
        g_vulkanContextResources.vulkanMemResourcePool.Alloc();
    TINKER_ASSERT(newResourceHandle != TINKER_INVALID_HANDLE);
    VulkanMemResourceChain* newResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(newResourceHandle);
    *newResourceChain = {};

    // TODO: do or do not have a resource chain for buffers.
    bool oneBufferOnly = false;
    bool perSwapChainSize = false;

    for (uint32 uiImage = 0; uiImage < g_vulkanContextResources.numSwapChainImages && !oneBufferOnly; ++uiImage)
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
        uint32 numBytesToAllocate = perSwapChainSize ? sizeInBytes * g_vulkanContextResources.numSwapChainImages : sizeInBytes;

        CreateBuffer(g_vulkanContextResources.physicalDevice,
            g_vulkanContextResources.device,
            numBytesToAllocate,
            usageFlags,
            propertyFlags,
            &newResource->buffer,
            &newResource->deviceMemory);
    }

    return ResourceHandle(newResourceHandle);
}

DescriptorHandle VulkanCreateDescriptor( uint32 descriptorLayoutID)
{
    if (g_vulkanContextResources.descriptorPool == VK_NULL_HANDLE)
    {
        // Allocate the descriptor pool
        InitDescriptorPool();
    }
    
    uint32 newDescriptorHandle = g_vulkanContextResources.vulkanDescriptorResourcePool.Alloc();

    for (uint32 uiImage = 0; uiImage < g_vulkanContextResources.numSwapChainImages; ++uiImage)
    {
        const VkDescriptorSetLayout& descriptorSetLayout = g_vulkanContextResources.descLayouts[descriptorLayoutID].layout;
        TINKER_ASSERT(descriptorSetLayout != VK_NULL_HANDLE);

        if (descriptorSetLayout != VK_NULL_HANDLE)
        {
            VkDescriptorSetAllocateInfo descSetAllocInfo = {};
            descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descSetAllocInfo.descriptorPool = g_vulkanContextResources.descriptorPool;
            descSetAllocInfo.descriptorSetCount = 1;
            descSetAllocInfo.pSetLayouts = &descriptorSetLayout;

            VkResult result = vkAllocateDescriptorSets(g_vulkanContextResources.device,
                &descSetAllocInfo,
                &g_vulkanContextResources.vulkanDescriptorResourcePool.PtrFromHandle(newDescriptorHandle)->resourceChain[uiImage].descriptorSet);
            if (result != VK_SUCCESS)
            {
                Core::Utility::LogMsg("Platform", "Failed to create Vulkan descriptor set!", Core::Utility::LogSeverity::eCritical);
                TINKER_ASSERT(0);
            }
        }
    }

    return DescriptorHandle(newDescriptorHandle);
}

void VulkanDestroyDescriptor( DescriptorHandle handle)
{
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?
    for (uint32 uiImage = 0; uiImage < g_vulkanContextResources.numSwapChainImages; ++uiImage)
    {
        // TODO: destroy something?
    }
    g_vulkanContextResources.vulkanDescriptorResourcePool.Dealloc(handle.m_hDesc);
}

void VulkanDestroyAllDescriptors()
{
    vkDestroyDescriptorPool(g_vulkanContextResources.device, g_vulkanContextResources.descriptorPool, nullptr);
    g_vulkanContextResources.descriptorPool = VK_NULL_HANDLE;
}

void VulkanWriteDescriptor( uint32 descriptorLayoutID, DescriptorHandle descSetHandle, const DescriptorSetDataHandles* descSetDataHandles, uint32 descSetDataCount)
{
    TINKER_ASSERT(descSetDataCount <= MAX_BINDINGS_PER_SET);

    DescriptorLayout* descLayout = &g_vulkanContextResources.descLayouts[descriptorLayoutID].bindings;

    for (uint32 uiImage = 0; uiImage < g_vulkanContextResources.numSwapChainImages; ++uiImage)
    {
        VkDescriptorSet* descriptorSet =
            &g_vulkanContextResources.vulkanDescriptorResourcePool.PtrFromHandle(descSetHandle.m_hDesc)->resourceChain[uiImage].descriptorSet;

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
                VulkanMemResource* res = &g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(descSetDataHandles->handles[uiDesc].m_hRes)->resourceChain[uiImage];

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
                        descImageInfo[descriptorCount].sampler = g_vulkanContextResources.linearSampler;

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
            vkUpdateDescriptorSets(g_vulkanContextResources.device, descriptorCount, descSetWrites, 0, nullptr);
        }
    }
}

void InitDescriptorPool()
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

    VkResult result = vkCreateDescriptorPool(g_vulkanContextResources.device, &descPoolCreateInfo, nullptr, &g_vulkanContextResources.descriptorPool);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create descriptor pool!", Core::Utility::LogSeverity::eInfo);
        return;
    }
}

void CreateSamplers()
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

    VkResult result = vkCreateSampler(g_vulkanContextResources.device, &samplerCreateInfo, nullptr, &g_vulkanContextResources.linearSampler);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create sampler!", Core::Utility::LogSeverity::eCritical);
        return;
    }
}

bool VulkanCreateRenderPass( uint32 renderPassID, uint32 numColorRTs, uint32 colorFormat, uint32 startLayout, uint32 endLayout, uint32 depthFormat)
{
    VulkanRenderPass& renderPass = g_vulkanContextResources.renderPasses[renderPassID];
    CreateRenderPass(g_vulkanContextResources.device, numColorRTs, GetVkImageFormat(colorFormat), GetVkImageLayout(startLayout), GetVkImageLayout(endLayout), GetVkImageFormat(depthFormat), &renderPass.renderPassVk);
    renderPass.numColorRTs = numColorRTs;
    renderPass.hasDepth = (depthFormat != ImageFormat::Invalid);
    return true;
}

FramebufferHandle VulkanCreateFramebuffer(
    ResourceHandle* rtColorHandles, uint32 numRTColorHandles, ResourceHandle rtDepthHandle,
    uint32 width, uint32 height, uint32 renderPassID)
{
    TINKER_ASSERT(numRTColorHandles <= VULKAN_MAX_RENDERTARGETS);

    bool HasDepth = rtDepthHandle != DefaultResHandle_Invalid;

    // Alloc handle
    uint32 newFramebufferHandle = g_vulkanContextResources.vulkanFramebufferResourcePool.Alloc();
    TINKER_ASSERT(newFramebufferHandle != TINKER_INVALID_HANDLE);

    VkImageView attachments[VULKAN_MAX_RENDERTARGETS_WITH_DEPTH];
    for (uint32 i = 0; i < ARRAYCOUNT(attachments); ++i)
        attachments[i] = VK_NULL_HANDLE;

    for (uint32 uiImage = 0; uiImage < g_vulkanContextResources.numSwapChainImages; ++uiImage)
    {
        VulkanFramebufferResource* newFramebuffer =
            &g_vulkanContextResources.vulkanFramebufferResourcePool.PtrFromHandle(newFramebufferHandle)->resourceChain[uiImage];

        // Create framebuffer
        for (uint32 uiImageView = 0; uiImageView < numRTColorHandles; ++uiImageView)
        {
            attachments[uiImageView] =
                g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(rtColorHandles[uiImageView].m_hRes)->resourceChain[uiImage].imageView;
        }

        VkImageView depthImageView = VK_NULL_HANDLE;
        if (HasDepth)
        {
            depthImageView = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(rtDepthHandle.m_hRes)->resourceChain[uiImage].imageView;
        }

        CreateFramebuffer(g_vulkanContextResources.device, attachments, numRTColorHandles, depthImageView, width, height, g_vulkanContextResources.renderPasses[renderPassID].renderPassVk, &newFramebuffer->framebuffer);

        for (uint32 uiRT = 0; uiRT < numRTColorHandles; ++uiRT)
        {
            // TODO: pass clear value as parameter
            newFramebuffer->clearValues[uiRT].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        }
        uint32 numClearValues = numRTColorHandles;

        if (HasDepth)
        {
            newFramebuffer->clearValues[numRTColorHandles].depthStencil = { 1.0f, 0 };
            ++numClearValues;
        }

        newFramebuffer->numClearValues = numClearValues;
    }

    return FramebufferHandle(newFramebufferHandle);
}

ResourceHandle VulkanCreateImageResource( uint32 imageFormat, uint32 width, uint32 height, uint32 numArrayEles)
{
    uint32 newResourceHandle = g_vulkanContextResources.vulkanMemResourcePool.Alloc();

    for (uint32 uiImage = 0; uiImage < g_vulkanContextResources.numSwapChainImages; ++uiImage)
    {
        VulkanMemResourceChain* newResourceChain =
            g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(newResourceHandle);
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

        VkResult result = vkCreateImage(g_vulkanContextResources.device,
            &imageCreateInfo,
            nullptr,
            &newResource->image);

        VkMemoryRequirements memRequirements = {};
        vkGetImageMemoryRequirements(g_vulkanContextResources.device, newResource->image, &memRequirements);
        AllocGPUMemory(g_vulkanContextResources.physicalDevice,
            g_vulkanContextResources.device,
            &newResource->deviceMemory,
            memRequirements,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vkBindImageMemory(g_vulkanContextResources.device, newResource->image, newResource->deviceMemory, 0);

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

        CreateImageView(g_vulkanContextResources.device,
            GetVkImageFormat(imageFormat),
            aspectMask,
            newResource->image,
            &newResource->imageView,
            numArrayEles);
    }

    return ResourceHandle(newResourceHandle);
}

ResourceHandle VulkanCreateResource( const ResourceDesc& resDesc)
{
    ResourceHandle newHandle = DefaultResHandle_Invalid;

    switch (resDesc.resourceType)
    {
        case ResourceType::eBuffer1D:
        {
            newHandle = VulkanCreateBuffer(resDesc.dims.x, resDesc.bufferUsage);
            break;
        }

        case ResourceType::eImage2D:
        {
            newHandle = VulkanCreateImageResource(resDesc.imageFormat, resDesc.dims.x, resDesc.dims.y, resDesc.arrayEles);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid resource description type!", Core::Utility::LogSeverity::eCritical);
            return newHandle;
        }
    }

    // Set the internal resdesc to the one provided
    VulkanMemResourceChain* newResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(newHandle.m_hRes);
    newResourceChain->resDesc = resDesc;
    
    return newHandle;
}

void VulkanDestroyImageResource( ResourceHandle handle)
{
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    for (uint32 uiImage = 0; uiImage < g_vulkanContextResources.numSwapChainImages; ++uiImage)
    {
        VulkanMemResource* resource = &g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[uiImage];
        vkDestroyImage(g_vulkanContextResources.device, resource->image, nullptr);
        vkFreeMemory(g_vulkanContextResources.device, resource->deviceMemory, nullptr);
        vkDestroyImageView(g_vulkanContextResources.device, resource->imageView, nullptr);
    }

    g_vulkanContextResources.vulkanMemResourcePool.Dealloc(handle.m_hRes);
}

void VulkanDestroyFramebuffer( FramebufferHandle handle)
{
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    for (uint32 uiImage = 0; uiImage < g_vulkanContextResources.numSwapChainImages; ++uiImage)
    {
        VulkanFramebufferResource* framebuffer = &g_vulkanContextResources.vulkanFramebufferResourcePool.PtrFromHandle(handle.m_hFramebuffer)->resourceChain[uiImage];
        vkDestroyFramebuffer(g_vulkanContextResources.device, framebuffer->framebuffer, nullptr);
    }
    
    g_vulkanContextResources.vulkanFramebufferResourcePool.Dealloc(handle.m_hFramebuffer);
}

void VulkanDestroyBuffer( ResourceHandle handle, uint32 bufferUsage)
{
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    for (uint32 uiImage = 0; uiImage < g_vulkanContextResources.numSwapChainImages; ++uiImage)
    {
        VulkanMemResource* resource = &g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[uiImage];

        // Only destroy/free memory if it's a validly created buffer
        if (resource->buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(g_vulkanContextResources.device, resource->buffer, nullptr);
            vkFreeMemory(g_vulkanContextResources.device, resource->deviceMemory, nullptr);
        }
        else
        {
            // TODO: this is currently happening since vertex buffers are only alloc'd once, but
            // currently the architecture stores a chain of buffer objects.
            //Core::Utility::LogMsg("Platform", "Freeing a null buffer", Core::Utility::LogSeverity::eWarning);
        }
    }

    g_vulkanContextResources.vulkanMemResourcePool.Dealloc(handle.m_hRes);
}

void VulkanDestroyResource( ResourceHandle handle)
{
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    VulkanMemResourceChain* resourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(handle.m_hRes);

    switch (resourceChain->resDesc.resourceType)
    {
        case ResourceType::eBuffer1D:
        {
            VulkanDestroyBuffer(handle, resourceChain->resDesc.bufferUsage);
            break;
        }

        case ResourceType::eImage2D:
        {
            VulkanDestroyImageResource(handle);
            break;
        }
    }
}

void DestroyVulkan()
{
    if (!g_vulkanContextResources.isInitted)
        return;

    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    VulkanDestroySwapChain();

    vkDestroyCommandPool(g_vulkanContextResources.device, g_vulkanContextResources.commandPool, nullptr);
    g_vulkanContextResources.commandBuffers = nullptr;

    VulkanDestroyAllPSOPerms();
    DestroyAllDescLayouts();
    VulkanDestroyAllRenderPasses();

    for (uint32 uiImage = 0; uiImage < VULKAN_MAX_FRAMES_IN_FLIGHT; ++uiImage)
    {
        vkDestroySemaphore(g_vulkanContextResources.device, g_vulkanContextResources.renderCompleteSemaphores[uiImage], nullptr);
        vkDestroySemaphore(g_vulkanContextResources.device, g_vulkanContextResources.swapChainImageAvailableSemaphores[uiImage], nullptr);
        vkDestroyFence(g_vulkanContextResources.device, g_vulkanContextResources.fences[uiImage], nullptr);
    }

    vkDestroySampler(g_vulkanContextResources.device, g_vulkanContextResources.linearSampler, nullptr);

    #if defined(ENABLE_VULKAN_VALIDATION_LAYERS)
    // Debug utils messenger
    PFN_vkDestroyDebugUtilsMessengerEXT dbgDestroyFunc =
        (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(g_vulkanContextResources.instance,
                                                                    "vkDestroyDebugUtilsMessengerEXT");
    if (dbgDestroyFunc)
    {
        dbgDestroyFunc(g_vulkanContextResources.instance, g_vulkanContextResources.debugMessenger, nullptr);
    }
    else
    {
        Core::Utility::LogMsg("Platform", "Failed to get destroy debug utils messenger proc addr!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
    #endif

    vkDestroyDevice(g_vulkanContextResources.device, nullptr);
    vkDestroySurfaceKHR(g_vulkanContextResources.instance, g_vulkanContextResources.surface, nullptr);
    vkDestroyInstance(g_vulkanContextResources.instance, nullptr);

    g_vulkanContextResources.vulkanMemResourcePool.ExplicitFree();
    g_vulkanContextResources.vulkanDescriptorResourcePool.ExplicitFree();
    g_vulkanContextResources.vulkanFramebufferResourcePool.ExplicitFree();
}

}
}
}

#include "Graphics/Vulkan/Vulkan.h"
#include "Graphics/Vulkan/VulkanTypes.h"
#include "Utility/Logging.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan_win32.h>
#endif

namespace Tk
{
namespace Graphics
{

static void CreateDescriptorPool()
{
    VkDescriptorPoolSize descPoolSizes[VULKAN_NUM_SUPPORTED_DESCRIPTOR_TYPES] = {};
    descPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descPoolSizes[0].descriptorCount = VULKAN_DESCRIPTOR_POOL_MAX_UNIFORM_BUFFERS;
    descPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descPoolSizes[1].descriptorCount = VULKAN_DESCRIPTOR_POOL_MAX_SAMPLED_IMAGES;
    descPoolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descPoolSizes[2].descriptorCount = VULKAN_DESCRIPTOR_POOL_MAX_STORAGE_BUFFERS;
    descPoolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descPoolSizes[3].descriptorCount = VULKAN_DESCRIPTOR_POOL_MAX_STORAGE_IMAGES;

    uint32 maxSets = 0;
    for (uint32 i = 0; i < VULKAN_NUM_SUPPORTED_DESCRIPTOR_TYPES; ++i)
    {
        maxSets += descPoolSizes[i].descriptorCount = VULKAN_DESCRIPTOR_POOL_MAX_STORAGE_IMAGES;
    }

    VkDescriptorPoolCreateInfo descPoolCreateInfo = {};
    descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolCreateInfo.poolSizeCount = VULKAN_NUM_SUPPORTED_DESCRIPTOR_TYPES;
    descPoolCreateInfo.pPoolSizes = descPoolSizes;
    descPoolCreateInfo.maxSets = maxSets;

    VkResult result = vkCreateDescriptorPool(g_vulkanContextResources.device, &descPoolCreateInfo, nullptr, &g_vulkanContextResources.descriptorPool);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create descriptor pool!", Core::Utility::LogSeverity::eInfo);
        return;
    }
}

static void CreateImageView(VkDevice device, VkFormat format, VkImageAspectFlags aspectMask, VkImage image, VkImageView* imageView, uint32 arrayEles)
{
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = arrayEles > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = arrayEles;

    VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, imageView);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan image view!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
}

static VkShaderModule CreateShaderModule(const char* shaderCode, uint32 numShaderCodeBytes, VkDevice device)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = numShaderCodeBytes;
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode);

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan shader module!", Core::Utility::LogSeverity::eCritical);
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

void CreateSwapChainAPIObjects(SwapChainData* swapChainData, const Tk::Platform::WindowHandles* windowHandles)
{
    uint32 newSwapChainAPIObjectsHandle = g_vulkanContextResources.vulkanSwapChainDataPool.Alloc();
    swapChainData->swapChainAPIObjectsHandle = newSwapChainAPIObjectsHandle;
    VulkanSwapChainData* vulkanSwapChainData = g_vulkanContextResources.vulkanSwapChainDataPool.PtrFromHandle(newSwapChainAPIObjectsHandle);

    // Surface
    #if defined(_WIN32)
    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {};
    win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32SurfaceCreateInfo.hinstance = (HINSTANCE)windowHandles->hInstance;
    win32SurfaceCreateInfo.hwnd = (HWND)windowHandles->hWindow;

    VkResult result = vkCreateWin32SurfaceKHR(g_vulkanContextResources.instance,
        &win32SurfaceCreateInfo,
        NULL,
        &vulkanSwapChainData->surface);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Win32SurfaceKHR!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
    #else
    // TODO: implement other platform surface types
    TINKER_ASSERT(0);
    #endif

    // Swap chain
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_vulkanContextResources.physicalDevice,
        vulkanSwapChainData->surface,
        &capabilities);

    VkExtent2D optimalExtent = {};
    if (capabilities.currentExtent.width != 0xffffffff)
    {
        optimalExtent = capabilities.currentExtent;
    }
    else
    {
        optimalExtent.width =
            CLAMP(swapChainData->windowWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        optimalExtent.height =
            CLAMP(swapChainData->windowHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    uint32 numSwapChainImages = DESIRED_NUM_SWAP_CHAIN_IMAGES;
    // TODO: find out about minImageCount having driver overhead 
    //TINKER_ASSERT(numSwapChainImages >= capabilities.minImageCount + 1);
    if (capabilities.maxImageCount > 0) // 0 can indicate no maximum
    {
        numSwapChainImages = Min(numSwapChainImages, capabilities.maxImageCount);
    }
    if (numSwapChainImages != DESIRED_NUM_SWAP_CHAIN_IMAGES)
    {
        Core::Utility::LogMsg("Platform", "Swap chain unable to support number of requested images!", Core::Utility::LogSeverity::eCritical);
    }

    uint32 numAvailableSurfaceFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(g_vulkanContextResources.physicalDevice,
        vulkanSwapChainData->surface,
        &numAvailableSurfaceFormats,
        nullptr);

    if (numAvailableSurfaceFormats == 0)
    {
        Core::Utility::LogMsg("Platform", "Zero available Vulkan swap chain surface formats!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkSurfaceFormatKHR* availableSurfaceFormats = (VkSurfaceFormatKHR*)g_vulkanContextResources.DataAllocator.Alloc(sizeof(VkSurfaceFormatKHR) * numAvailableSurfaceFormats, 1);
    vkGetPhysicalDeviceSurfaceFormatsKHR(g_vulkanContextResources.physicalDevice,
        vulkanSwapChainData->surface,
        &numAvailableSurfaceFormats,
        availableSurfaceFormats);

    VkSurfaceFormatKHR chosenFormat = {};
    chosenFormat.format = VK_FORMAT_UNDEFINED;
    for (uint32 uiAvailFormat = 0; uiAvailFormat < numAvailableSurfaceFormats; ++uiAvailFormat)
    {
        if (availableSurfaceFormats[uiAvailFormat].format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableSurfaceFormats[uiAvailFormat].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosenFormat = availableSurfaceFormats[uiAvailFormat];
        }
    }
    TINKER_ASSERT(chosenFormat.format != VK_FORMAT_UNDEFINED);

    uint32 numAvailablePresentModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(g_vulkanContextResources.physicalDevice,
        vulkanSwapChainData->surface,
        &numAvailablePresentModes,
        nullptr);

    if (numAvailablePresentModes == 0)
    {
        Core::Utility::LogMsg("Platform", "Zero available Vulkan swap chain present modes!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkPresentModeKHR* availablePresentModes = (VkPresentModeKHR*)g_vulkanContextResources.DataAllocator.Alloc(sizeof(VkPresentModeKHR) * numAvailablePresentModes, 1);
    vkGetPhysicalDeviceSurfacePresentModesKHR(g_vulkanContextResources.physicalDevice,
        vulkanSwapChainData->surface,
        &numAvailablePresentModes,
        availablePresentModes);

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32 uiAvailPresMode = 0; uiAvailPresMode < numAvailablePresentModes; ++uiAvailPresMode)
    {
        /*if (availablePresentModes[uiAvailPresMode] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            chosenPresentMode = availablePresentModes[uiAvailPresMode];
            break;
        }*/
    }

    vulkanSwapChainData->swapChainExtent = optimalExtent;
    g_vulkanContextResources.swapChainFormat = chosenFormat.format; // not stored per allocated swap chain data, all swap chains should have the same format 

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = vulkanSwapChainData->surface;
    swapChainCreateInfo.minImageCount = numSwapChainImages;
    swapChainCreateInfo.imageFormat = chosenFormat.format;
    swapChainCreateInfo.imageColorSpace = chosenFormat.colorSpace;
    swapChainCreateInfo.imageExtent = optimalExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.preTransform = capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = chosenPresentMode;

    const uint32 numQueueFamilyIndices = 1;
    const uint32 queueFamilyIndices[numQueueFamilyIndices] =
    {
        g_vulkanContextResources.graphicsQueueIndex,
    };

    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.queueFamilyIndexCount = 0;
    swapChainCreateInfo.pQueueFamilyIndices = nullptr;

    result = vkCreateSwapchainKHR(g_vulkanContextResources.device,
        &swapChainCreateInfo,
        nullptr,
        &vulkanSwapChainData->swapChain);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan swap chain!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    vkGetSwapchainImagesKHR(g_vulkanContextResources.device,
        vulkanSwapChainData->swapChain,
        &numSwapChainImages,
        nullptr);
    TINKER_ASSERT(numSwapChainImages != 0);

    vulkanSwapChainData->swapChainImages = (VkImage*)g_vulkanContextResources.DataAllocator.Alloc(sizeof(VkImage) * numSwapChainImages, 1);
    vulkanSwapChainData->swapChainImageViews = (VkImageView*)g_vulkanContextResources.DataAllocator.Alloc(sizeof(VkImageView) * numSwapChainImages, 1);
    swapChainData->numSwapChainImages = numSwapChainImages;

    for (uint32 i = 0; i < numSwapChainImages; ++i)
    {
        vulkanSwapChainData->swapChainImages[i] = VK_NULL_HANDLE;
        vulkanSwapChainData->swapChainImageViews[i] = VK_NULL_HANDLE;
    }
    
    vkGetSwapchainImagesKHR(g_vulkanContextResources.device,
        vulkanSwapChainData->swapChain,
        &numSwapChainImages,
        vulkanSwapChainData->swapChainImages);

    for (uint32 uiImageView = 0; uiImageView < numSwapChainImages; ++uiImageView)
    {
        CreateImageView(g_vulkanContextResources.device,
            g_vulkanContextResources.swapChainFormat,
            VK_IMAGE_ASPECT_COLOR_BIT,
            vulkanSwapChainData->swapChainImages[uiImageView],
            &vulkanSwapChainData->swapChainImageViews[uiImageView],
            1);
    }

    // Register swap chain images in the resource pool manually 
    for (uint32 i = 0; i < DESIRED_NUM_SWAP_CHAIN_IMAGES; ++i)
    {
        uint32 newResourceHandle = g_vulkanContextResources.vulkanMemResourcePool.Alloc();
        TINKER_ASSERT(newResourceHandle != TINKER_INVALID_HANDLE);
        VulkanMemResourceChain* newResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(newResourceHandle);
        *newResourceChain = {};
        
        // Only one image stored, not per frame in flight 
        newResourceChain->resourceChain[0].image = vulkanSwapChainData->swapChainImages[i];
        newResourceChain->resourceChain[0].imageView = vulkanSwapChainData->swapChainImageViews[i];
        newResourceChain->resDesc.debugLabel = "Swap chain image resource";
        newResourceChain->resDesc.dims = v3ui((uint32)vulkanSwapChainData->swapChainExtent.width, (uint32)vulkanSwapChainData->swapChainExtent.height, 1);
        newResourceChain->resDesc.imageFormat = ImageFormat::TheSwapChainFormat;
        newResourceChain->resDesc.arrayEles = 1;
        newResourceChain->resDesc.imageUsageFlags = ImageUsageFlags::RenderTarget;
        swapChainData->swapChainResourceHandles[i] = ResourceHandle(newResourceHandle);
    }

    // Virtual frame synchronization data initialization - 2 semaphores and fence
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32 uiFrame = 0; uiFrame < MAX_FRAMES_IN_FLIGHT; ++uiFrame)
    {
        result = vkCreateSemaphore(g_vulkanContextResources.device,
            &semaphoreCreateInfo,
            nullptr,
            &vulkanSwapChainData->virtualFrameSyncData[uiFrame].GPUWorkCompleteSema);

        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Platform", "Failed to create Vulkan gpu work complete semaphore!", Core::Utility::LogSeverity::eCritical);
        }

        result = vkCreateSemaphore(g_vulkanContextResources.device,
            &semaphoreCreateInfo,
            nullptr,
            &vulkanSwapChainData->virtualFrameSyncData[uiFrame].ImageAvailableSema);

        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Platform", "Failed to create Vulkan present semaphore!", Core::Utility::LogSeverity::eCritical);
        }
    }

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32 uiFrame = 0; uiFrame < MAX_FRAMES_IN_FLIGHT; ++uiFrame)
    {
        result = vkCreateFence(g_vulkanContextResources.device, &fenceCreateInfo, nullptr, &vulkanSwapChainData->virtualFrameSyncData[uiFrame].Fence);
        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Platform", "Failed to create Vulkan fence!", Core::Utility::LogSeverity::eCritical);
        }
    }
}

void DestroySwapChainAPIObjects(SwapChainData* swapChainData)
{
    uint32 swapChainAPIObjectsHandle = swapChainData->swapChainAPIObjectsHandle;
    swapChainData->isSwapChainValid = false;
    VulkanSwapChainData* vulkanSwapChainData = g_vulkanContextResources.vulkanSwapChainDataPool.PtrFromHandle(swapChainAPIObjectsHandle);

    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    for (uint32 i = 0; i < DESIRED_NUM_SWAP_CHAIN_IMAGES; ++i)
    {
        g_vulkanContextResources.vulkanMemResourcePool.Dealloc(swapChainData->swapChainResourceHandles[i].m_hRes);
    }

    // Don't have to destroy swap chain VkImages
    for (uint32 uiImg = 0; uiImg < swapChainData->numSwapChainImages; ++uiImg)
    {
        vkDestroyImageView(g_vulkanContextResources.device, vulkanSwapChainData->swapChainImageViews[uiImg], nullptr);
    }

    vkDestroySwapchainKHR(g_vulkanContextResources.device, vulkanSwapChainData->swapChain, nullptr);
    vkDestroySurfaceKHR(g_vulkanContextResources.instance, vulkanSwapChainData->surface, nullptr);

    for (uint32 uiFrame = 0; uiFrame < MAX_FRAMES_IN_FLIGHT; ++uiFrame)
    {
        vkDestroySemaphore(g_vulkanContextResources.device, vulkanSwapChainData->virtualFrameSyncData[uiFrame].GPUWorkCompleteSema, nullptr);
        vulkanSwapChainData->virtualFrameSyncData[uiFrame].GPUWorkCompleteSema = VK_NULL_HANDLE;
        vkDestroySemaphore(g_vulkanContextResources.device, vulkanSwapChainData->virtualFrameSyncData[uiFrame].ImageAvailableSema, nullptr);
        vulkanSwapChainData->virtualFrameSyncData[uiFrame].ImageAvailableSema = VK_NULL_HANDLE;
        vkDestroyFence(g_vulkanContextResources.device, vulkanSwapChainData->virtualFrameSyncData[uiFrame].Fence, nullptr);
        vulkanSwapChainData->virtualFrameSyncData[uiFrame].Fence = VK_NULL_HANDLE;
    }

    g_vulkanContextResources.vulkanSwapChainDataPool.Dealloc(swapChainAPIObjectsHandle);
    swapChainData->swapChainAPIObjectsHandle = TINKER_INVALID_HANDLE;
}

CREATE_GRAPHICS_PIPELINE(CreateGraphicsPipeline)
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

    // Viewport and scissor must be set dynamically 
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;

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
        VK_DYNAMIC_STATE_SCISSOR,
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
    pushConstantRange.size = MIN_PUSH_CONSTANTS_SIZE;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = numDescriptorLayoutHandles;
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
    pipelineLayoutCreateInfo.pushConstantRangeCount = numPushConstants;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    VkPipelineLayout& pipelineLayout = g_vulkanContextResources.psoPermutations.pipelineLayout[shaderID];
    VkResult result = vkCreatePipelineLayout(g_vulkanContextResources.device,
        &pipelineLayoutCreateInfo,
        nullptr,
        &pipelineLayout);

    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan graphics pipeline layout!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
    else
    {
        for (uint32 blendState = 0; blendState < VulkanContextResources::eMaxBlendStates; ++blendState)
        {
            for (uint32 depthState = 0; depthState < VulkanContextResources::eMaxDepthStates; ++depthState)
            {
                VkPipeline& graphicsPipeline = g_vulkanContextResources.psoPermutations.graphicsPipeline[shaderID][blendState][depthState];

                DepthCullState depthCullState = GetVkDepthCullState(depthState);
                VkPipelineColorBlendAttachmentState colorBlendAttachment = GetVkBlendState(blendState);

                if (numColorRTs == 0)
                {
                    colorBlending.attachmentCount = 0;
                    colorBlending.pAttachments = nullptr;
                }
                else
                {
                    colorBlending.attachmentCount = numColorRTs;
                    colorBlending.pAttachments = &colorBlendAttachment;
                }

                VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
                pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
                pipelineRenderingCreateInfo.colorAttachmentCount = numColorRTs;
                VkFormat formats[MAX_MULTIPLE_RENDERTARGETS] = {};
                for (uint32 uiFmt = 0; uiFmt < Min(numColorRTs, (uint32)ARRAYCOUNT(formats)); ++uiFmt)
                {
                    formats[uiFmt] = GetVkImageFormat(colorRTFormats[uiFmt]);
                }
                pipelineRenderingCreateInfo.pColorAttachmentFormats = formats;

                pipelineRenderingCreateInfo.depthAttachmentFormat = GetVkImageFormat(depthFormat);
                pipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

                VkPipelineRasterizationStateCreateInfo rasterizer = {};
                rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizer.depthClampEnable = VK_FALSE;
                rasterizer.rasterizerDiscardEnable = VK_FALSE;
                rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
                rasterizer.lineWidth = 1.0f;
                rasterizer.cullMode = depthCullState.cullMode;
                rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                rasterizer.depthBiasEnable = VK_FALSE;
                rasterizer.depthBiasConstantFactor = 0.0f;
                rasterizer.depthBiasClamp = 0.0f;
                rasterizer.depthBiasSlopeFactor = 0.0f;

                VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
                pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo; // for dynamic rendering
                pipelineCreateInfo.stageCount = numStages;
                pipelineCreateInfo.pStages = shaderStages;
                pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
                pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
                pipelineCreateInfo.pViewportState = &viewportState;
                pipelineCreateInfo.pRasterizationState = &rasterizer;
                pipelineCreateInfo.pMultisampleState = &multisampling;
                pipelineCreateInfo.pDepthStencilState = &depthCullState.depthState;
                pipelineCreateInfo.pColorBlendState = &colorBlending;
                pipelineCreateInfo.pDynamicState = &dynamicState;
                pipelineCreateInfo.layout = pipelineLayout;
                pipelineCreateInfo.renderPass = VK_NULL_HANDLE; // for dynamic rendering
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
    }

    vkDestroyShaderModule(g_vulkanContextResources.device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(g_vulkanContextResources.device, fragmentShaderModule, nullptr);

    return result == VK_SUCCESS;
}

DESTROY_GRAPHICS_PIPELINE(DestroyGraphicsPipeline)
{
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    VkPipelineLayout& pipelineLayout = g_vulkanContextResources.psoPermutations.pipelineLayout[shaderID];
    if (pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(g_vulkanContextResources.device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }

    for (uint32 uiblendState = 0; uiblendState < VulkanContextResources::eMaxBlendStates; ++uiblendState)
    {
        for (uint32 uiDepthState = 0; uiDepthState < VulkanContextResources::eMaxDepthStates; ++uiDepthState)
        {
            VkPipeline& graphicsPipeline = g_vulkanContextResources.psoPermutations.graphicsPipeline[shaderID][uiblendState][uiDepthState];
            if (graphicsPipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(g_vulkanContextResources.device, graphicsPipeline, nullptr);
                graphicsPipeline = VK_NULL_HANDLE;
            }
        }
    }
}

CREATE_COMPUTE_PIPELINE(CreateComputePipeline)
{
    VkShaderModule computeShaderModule = VK_NULL_HANDLE;

    const uint32 numStages = 1;
    if (numComputeShaderBytes > 0)
    {
        computeShaderModule = CreateShaderModule((const char*)computeShaderCode, numComputeShaderBytes, g_vulkanContextResources.device);
    }

    // Programmable shader stages
    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module = computeShaderModule;
    shaderStage.pName = "main";

    // Descriptor layouts
    TINKER_ASSERT(numDescriptorLayoutHandles <= MAX_DESCRIPTOR_SETS_PER_SHADER);

    VkDescriptorSetLayout descriptorSetLayouts[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    for (uint32 uiDesc = 0; uiDesc < numDescriptorLayoutHandles; ++uiDesc)
    {
        uint32 descLayoutID = descriptorLayoutHandles[uiDesc];
        if (descLayoutID != DESCLAYOUT_ID_MAX)
            descriptorSetLayouts[uiDesc] = g_vulkanContextResources.descLayouts[descLayoutID].layout;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = numDescriptorLayoutHandles;
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;

    VkPipelineLayout& pipelineLayout = g_vulkanContextResources.psoPermutations.computePipelineLayout[shaderID];
    VkResult result = vkCreatePipelineLayout(g_vulkanContextResources.device,
        &pipelineLayoutCreateInfo,
        nullptr,
        &pipelineLayout);

    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create Vulkan compute pipeline layout!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
    else
    {
        VkComputePipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.layout = pipelineLayout;
        pipelineCreateInfo.stage = shaderStage;

        VkPipeline& computePipeline = g_vulkanContextResources.psoPermutations.computePipeline[shaderID];

        result = vkCreateComputePipelines(g_vulkanContextResources.device,
            VK_NULL_HANDLE,
            1,
            &pipelineCreateInfo,
            nullptr,
            &computePipeline);

        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Platform", "Failed to create Vulkan compute pipeline!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
        }
    }

    vkDestroyShaderModule(g_vulkanContextResources.device, computeShaderModule, nullptr);

    return result == VK_SUCCESS;
}

DESTROY_COMPUTE_PIPELINE(DestroyComputePipeline)
{
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    VkPipelineLayout& pipelineLayout = g_vulkanContextResources.psoPermutations.computePipelineLayout[shaderID];
    if (pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(g_vulkanContextResources.device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }

    VkPipeline& computePipeline = g_vulkanContextResources.psoPermutations.computePipeline[shaderID];
    if (computePipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(g_vulkanContextResources.device, computePipeline, nullptr);
        computePipeline = VK_NULL_HANDLE;
    }
}

void DestroyAllPSOPerms()
{
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    for (uint32 shaderID = 0; shaderID < VulkanContextResources::eMaxShaders; ++shaderID)
    {
        DestroyGraphicsPipeline(shaderID);
    }

    for (uint32 shaderID = 0; shaderID < VulkanContextResources::eMaxShadersCompute; ++shaderID)
    {
        DestroyComputePipeline(shaderID);
    }
}

static ResourceHandle CreateBufferResource(uint32 sizeInBytes, uint32 bufferUsage, const char* debugLabel)
{
    uint32 newResourceHandle =
        g_vulkanContextResources.vulkanMemResourcePool.Alloc();
    TINKER_ASSERT(newResourceHandle != TINKER_INVALID_HANDLE);
    VulkanMemResourceChain* newResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(newResourceHandle);
    *newResourceChain = {};

    uint32 isMultiBufferedResource = IsBufferUsageMultiBuffered(bufferUsage);
    const uint32 NumCopies = isMultiBufferedResource ? MAX_FRAMES_IN_FLIGHT : 1u;

    const VkBufferUsageFlags usageFlags = GetVkBufferUsageFlags(bufferUsage);
    const VkMemoryPropertyFlags propertyFlags = GetVkMemoryPropertyFlags(bufferUsage);

    // Pick the correct gpu memory allocator
    // TODO: this will change once the user can create allocators via the graphics layer
    uint32 AllocatorIndex = 0xFFFFFFFF;
    if (propertyFlags == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        AllocatorIndex = g_vulkanContextResources.eVulkanMemoryAllocatorDeviceLocalBuffers;
    else
        AllocatorIndex = g_vulkanContextResources.eVulkanMemoryAllocatorHostVisibleBuffers;

    for (uint32 uiBuf = 0; uiBuf < NumCopies; ++uiBuf)
    {
        VulkanMemResource* newResource = &newResourceChain->resourceChain[uiBuf];

        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = sizeInBytes;
        bufferCreateInfo.usage = usageFlags;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(g_vulkanContextResources.device, &bufferCreateInfo, nullptr, &newResource->buffer);
        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Platform", "Failed to create buffer!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
        }

        VkMemoryRequirements memRequirements = {};
        vkGetBufferMemoryRequirements(g_vulkanContextResources.device, newResource->buffer, &memRequirements);

        VulkanMemAlloc newAlloc = g_vulkanContextResources.GPUMemAllocators[AllocatorIndex].Alloc(memRequirements);
        result = vkBindBufferMemory(g_vulkanContextResources.device, newResource->buffer, newAlloc.allocMem, newAlloc.allocOffset);
        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Platform", "Failed to bind buffer memory!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
        }
        newResource->GpuMemAlloc = newAlloc;

        DbgSetBufferObjectName((uint64)newResource->buffer, debugLabel);
    }

    return ResourceHandle(newResourceHandle);
}

static ResourceHandle CreateImageResource(uint32 imageFormat, uint32 imageUsageFlags, uint32 width, uint32 height, uint32 numArrayEles, const char* debugLabel)
{
    uint32 newResourceHandle = g_vulkanContextResources.vulkanMemResourcePool.Alloc();
    TINKER_ASSERT(newResourceHandle != TINKER_INVALID_HANDLE);
    TINKER_ASSERT(imageUsageFlags);
    VulkanMemResourceChain* newResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(newResourceHandle);
    *newResourceChain = {};

    // Images not duplicated per frame in flight
    VulkanMemResource* newResource = &newResourceChain->resourceChain[0];

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
    
    if (imageUsageFlags & ImageUsageFlags::RenderTarget)
    {
        imageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    if (imageUsageFlags & ImageUsageFlags::DepthStencil)
    {
        imageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }

    if (imageUsageFlags & ImageUsageFlags::UAV)
    {
        imageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    if (imageUsageFlags & ImageUsageFlags::TransferDst)
    {
        imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    if (imageUsageFlags & ImageUsageFlags::Sampled)
    {
        imageCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    VkResult result = vkCreateImage(g_vulkanContextResources.device, &imageCreateInfo, nullptr, &newResource->image);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to create image!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    // Pick the correct gpu memory allocator
    // TODO: this will change once the user can create allocators via the graphics layer
    const uint32 AllocatorIndex = g_vulkanContextResources.eVulkanMemoryAllocatorDeviceLocalImages;
    VkMemoryRequirements memRequirements = {};
    vkGetImageMemoryRequirements(g_vulkanContextResources.device, newResource->image, &memRequirements);

    VulkanMemAlloc newAlloc = g_vulkanContextResources.GPUMemAllocators[AllocatorIndex].Alloc(memRequirements);
    result = vkBindImageMemory(g_vulkanContextResources.device, newResource->image, newAlloc.allocMem, newAlloc.allocOffset);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to bind image memory!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    DbgSetImageObjectName((uint64)newResource->image, debugLabel);

    // Create image view
    VkImageAspectFlags aspectMask = {};
    // TODO: collapse this switch into an array of data
    switch (imageFormat)
    {
        case ImageFormat::RGBA16_Float:
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

    return ResourceHandle(newResourceHandle);
}

ResourceHandle CreateResource(const ResourceDesc& resDesc)
{
    ResourceHandle newHandle = DefaultResHandle_Invalid;

    switch (resDesc.resourceType)
    {
        case ResourceType::eBuffer1D:
        {
            newHandle = CreateBufferResource(resDesc.dims.x, resDesc.bufferUsage, resDesc.debugLabel);
            break;
        }

        case ResourceType::eImage2D:
        {
            newHandle = CreateImageResource(resDesc.imageFormat, resDesc.imageUsageFlags, resDesc.dims.x, resDesc.dims.y, resDesc.arrayEles, resDesc.debugLabel);
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

void DestroyResource(ResourceHandle handle)
{
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?

    VulkanMemResourceChain* resourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(handle.m_hRes);

    for (uint32 uiFrame = 0; uiFrame < MAX_FRAMES_IN_FLIGHT; ++uiFrame)
    {
        VulkanMemResource* resource = &g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(handle.m_hRes)->resourceChain[uiFrame];

        switch (resourceChain->resDesc.resourceType)
        {
            case ResourceType::eBuffer1D:
            {
                if (resource->buffer != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(g_vulkanContextResources.device, resource->buffer, nullptr);
                }
                break;
            }

            case ResourceType::eImage2D:
            {
                if (resource->image != VK_NULL_HANDLE)
                {
                    vkDestroyImage(g_vulkanContextResources.device, resource->image, nullptr);
                    vkDestroyImageView(g_vulkanContextResources.device, resource->imageView, nullptr);
                }
                break;
            }
        }
    }
    g_vulkanContextResources.vulkanMemResourcePool.Dealloc(handle.m_hRes);
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

DescriptorHandle CreateDescriptor(uint32 descriptorLayoutID)
{
    if (g_vulkanContextResources.descriptorPool == VK_NULL_HANDLE)
    {
        // Allocate the descriptor pool
        CreateDescriptorPool();
    }

    VulkanDescriptorLayout& vulkanDescriptorLayout = g_vulkanContextResources.descLayouts[descriptorLayoutID];
    const VkDescriptorSetLayout& descriptorSetLayout = vulkanDescriptorLayout.layout;
    TINKER_ASSERT(descriptorSetLayout != VK_NULL_HANDLE);

    uint32 newDescriptorHandle = g_vulkanContextResources.vulkanDescriptorResourcePool.Alloc();

    for (uint32 uiImage = 0; uiImage < MAX_FRAMES_IN_FLIGHT; ++uiImage)
    {
        if (descriptorSetLayout != VK_NULL_HANDLE)
        {
            VkDescriptorSetAllocateInfo descSetAllocInfo = {};
            descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descSetAllocInfo.descriptorPool = g_vulkanContextResources.descriptorPool;
            descSetAllocInfo.descriptorSetCount = 1;
            descSetAllocInfo.pSetLayouts = &descriptorSetLayout;

            VulkanDescriptor& vulkanDescriptor = g_vulkanContextResources.vulkanDescriptorResourcePool.PtrFromHandle(newDescriptorHandle)->resourceChain[uiImage];

            VkResult result = vkAllocateDescriptorSets(g_vulkanContextResources.device, &descSetAllocInfo, &vulkanDescriptor.descriptorSet);
            if (result != VK_SUCCESS)
            {
                Core::Utility::LogMsg("Platform", "Failed to create Vulkan descriptor set!", Core::Utility::LogSeverity::eCritical);
                TINKER_ASSERT(0);
            }
        }
    }

    return DescriptorHandle(newDescriptorHandle, descriptorLayoutID);
}

bool CreateDescriptorLayout(uint32 descriptorLayoutID, const DescriptorLayout* descriptorLayout)
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
            descLayoutBinding[numBindings].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
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

void DestroyDescriptor(DescriptorHandle handle)
{
    vkDeviceWaitIdle(g_vulkanContextResources.device); // TODO: move this?
    for (uint32 uiImage = 0; uiImage < MAX_FRAMES_IN_FLIGHT; ++uiImage)
    {
        // TODO: destroy something?
    }
    g_vulkanContextResources.vulkanDescriptorResourcePool.Dealloc(handle.m_hDesc);
}

void DestroyAllDescLayouts()
{
    for (uint32 desc = 0; desc < VulkanContextResources::eMaxDescLayouts; ++desc)
    {
        VulkanDescriptorLayout& vulkanDescriptorLayout = g_vulkanContextResources.descLayouts[desc];
        VkDescriptorSetLayout& descLayout = vulkanDescriptorLayout.layout;
        if (descLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(g_vulkanContextResources.device, descLayout, nullptr);
            descLayout = VK_NULL_HANDLE;
        }
    }
}

void DestroyAllDescriptors()
{
    vkDestroyDescriptorPool(g_vulkanContextResources.device, g_vulkanContextResources.descriptorPool, nullptr);
    g_vulkanContextResources.descriptorPool = VK_NULL_HANDLE;
}

}
}

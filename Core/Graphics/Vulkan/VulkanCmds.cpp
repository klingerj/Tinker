#include "Graphics/Common/GraphicsCommon.h"
#include "Graphics/Vulkan/Vulkan.h"
#include "Graphics/Vulkan/VulkanTypes.h"
#include "Utility/Logging.h"

namespace Tk
{
namespace Core
{
namespace Graphics
{

bool VulkanAcquireFrame()
{
    const VulkanVirtualFrameSyncData& virtualFrameSyncData = g_vulkanContextResources.virtualFrameSyncData[g_vulkanContextResources.currentVirtualFrame];

    VkResult result = vkWaitForFences(g_vulkanContextResources.device, 1, &virtualFrameSyncData.Fence, VK_FALSE, (uint64)-1);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Waiting for virtual frame fence took too long!", Core::Utility::LogSeverity::eInfo);
        return false;
    }
    vkResetFences(g_vulkanContextResources.device, 1, &virtualFrameSyncData.Fence);

    uint32 currentSwapChainImageIndex = TINKER_INVALID_HANDLE;

    result = vkAcquireNextImageKHR(g_vulkanContextResources.device,
        g_vulkanContextResources.swapChain,
        (uint64)-1,
        virtualFrameSyncData.PresentCompleteSema,
        VK_NULL_HANDLE,
        &currentSwapChainImageIndex);

    // Error checking
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        Core::Utility::LogMsg("Platform", "Failed to acquire next swap chain image!", Core::Utility::LogSeverity::eInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            TINKER_ASSERT(0); // untested code path
            Core::Utility::LogMsg("Platform", "Recreating swap chain!", Core::Utility::LogSeverity::eInfo);
            VulkanDestroySwapChain();
            VulkanCreateSwapChain();
            return false; // Don't present on this frame
        }
        else
        {
            Core::Utility::LogMsg("Platform", "Not recreating swap chain!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
        }

        return false;
    }

    g_vulkanContextResources.currentSwapChainImage = currentSwapChainImageIndex;
    return true;
}

void VulkanSubmitFrame()
{
    const VulkanVirtualFrameSyncData& virtualFrameSyncData = g_vulkanContextResources.virtualFrameSyncData[g_vulkanContextResources.currentVirtualFrame];

    // Submit
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[1] = { virtualFrameSyncData.PresentCompleteSema };
    VkPipelineStageFlags waitStages[1] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &g_vulkanContextResources.commandBuffers[g_vulkanContextResources.currentVirtualFrame];

    VkSemaphore signalSemaphores[1] = { virtualFrameSyncData.GPUWorkCompleteSema };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkResult result = vkQueueSubmit(g_vulkanContextResources.graphicsQueue, 1, &submitInfo, virtualFrameSyncData.Fence);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to submit command buffer to queue!", Core::Utility::LogSeverity::eCritical);
    }

    // Present
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &g_vulkanContextResources.swapChain;
    presentInfo.pImageIndices = &g_vulkanContextResources.currentSwapChainImage;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(g_vulkanContextResources.presentationQueue, &presentInfo);

    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to present swap chain!", Core::Utility::LogSeverity::eInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            TINKER_ASSERT(0); // untested code path
            Core::Utility::LogMsg("Platform", "Recreating swap chain!", Core::Utility::LogSeverity::eInfo);
            VulkanDestroySwapChain();
            VulkanCreateSwapChain();
            return;
        }
        else
        {
            Core::Utility::LogMsg("Platform", "Not recreating swap chain!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
        }
    }

    g_vulkanContextResources.currentVirtualFrame = (g_vulkanContextResources.currentVirtualFrame + 1) % VULKAN_MAX_FRAMES_IN_FLIGHT;
}

void BeginVulkanCommandRecording()
{
    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = 0;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    VkResult result = vkBeginCommandBuffer(g_vulkanContextResources.commandBuffers[g_vulkanContextResources.currentVirtualFrame], &commandBufferBeginInfo);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to begin Vulkan command buffer!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
}

void EndVulkanCommandRecording()
{
    if (g_vulkanContextResources.currentSwapChainImage == TINKER_INVALID_HANDLE || g_vulkanContextResources.currentVirtualFrame == TINKER_INVALID_HANDLE)
    {
        TINKER_ASSERT(0);
    }

    VkResult result = vkEndCommandBuffer(g_vulkanContextResources.commandBuffers[g_vulkanContextResources.currentVirtualFrame]);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to end Vulkan command buffer!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
}

void BeginVulkanCommandRecordingImmediate()
{
    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult result = vkBeginCommandBuffer(g_vulkanContextResources.commandBuffer_Immediate, &commandBufferBeginInfo);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to begin Vulkan command buffer (immediate)!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
}

void EndVulkanCommandRecordingImmediate()
{
    VkResult result = vkEndCommandBuffer(g_vulkanContextResources.commandBuffer_Immediate);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to end Vulkan command buffer (immediate)!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &g_vulkanContextResources.commandBuffer_Immediate;

    result = vkQueueSubmit(g_vulkanContextResources.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to submit command buffer to queue!", Core::Utility::LogSeverity::eCritical);
    }
    vkQueueWaitIdle(g_vulkanContextResources.graphicsQueue);
}

// TODO: remove mystery bool param
VkCommandBuffer ChooseAppropriateCommandBuffer(bool immediateSubmit)
{
    VkCommandBuffer commandBuffer = g_vulkanContextResources.commandBuffer_Immediate;

    if (!immediateSubmit)
    {
        TINKER_ASSERT(g_vulkanContextResources.currentSwapChainImage != TINKER_INVALID_HANDLE && g_vulkanContextResources.currentVirtualFrame != TINKER_INVALID_HANDLE);
        commandBuffer = g_vulkanContextResources.commandBuffers[g_vulkanContextResources.currentVirtualFrame];
    }

    return commandBuffer;
}

void VulkanRecordCommandPushConstant(uint8* data, uint32 sizeInBytes, uint32 shaderID, uint32 blendState, uint32 depthState)
{
    TINKER_ASSERT(data && sizeInBytes);

    VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(false);
    const VkPipelineLayout& pipelineLayout = g_vulkanContextResources.psoPermutations.pipelineLayout[shaderID];
    TINKER_ASSERT(pipelineLayout != VK_NULL_HANDLE);

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeInBytes, data);
}

void VulkanRecordCommandDrawCall(ResourceHandle indexBufferHandle, uint32 numIndices,
    uint32 numInstances, const char* debugLabel, bool immediateSubmit)
{
    TINKER_ASSERT(indexBufferHandle != DefaultResHandle_Invalid);

    VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(immediateSubmit);

    // Index buffer
    VulkanMemResourceChain* indexBufferResource = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(indexBufferHandle.m_hRes);
    //uint32 indexBufferSize = indexBufferResource->resDesc.dims.x;
    //TODO: bad indexing
    VkBuffer& indexBuffer = indexBufferResource->resourceChain[g_vulkanContextResources.currentVirtualFrame].buffer;
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

#if defined(ENABLE_VULKAN_DEBUG_LABELS)
    VkDebugUtilsLabelEXT label =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        NULL,
        debugLabel,
        { 0.0f, 0.0f, 0.0f, 0.0f },
    };
    g_vulkanContextResources.pfnCmdInsertDebugUtilsLabelEXT(commandBuffer, &label);
#endif
    vkCmdDrawIndexed(commandBuffer, numIndices, numInstances, 0, 0, 0);
}

void VulkanRecordCommandBindShader(uint32 shaderID, uint32 blendState, uint32 depthState,
    const DescriptorHandle* descSetHandles, bool immediateSubmit)
{
    const VkPipeline& pipeline = g_vulkanContextResources.psoPermutations.graphicsPipeline[shaderID][blendState][depthState];
    TINKER_ASSERT(pipeline != VK_NULL_HANDLE);
    const VkPipelineLayout& pipelineLayout = g_vulkanContextResources.psoPermutations.pipelineLayout[shaderID];
    TINKER_ASSERT(pipelineLayout != VK_NULL_HANDLE);

    VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(immediateSubmit);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    for (uint32 uiDesc = 0; uiDesc < MAX_DESCRIPTOR_SETS_PER_SHADER; ++uiDesc)
    {
        DescriptorHandle descHandle = descSetHandles[uiDesc];
        if (descHandle != DefaultDescHandle_Invalid)
        {
            VkDescriptorSet* descSet =
                &g_vulkanContextResources.vulkanDescriptorResourcePool.PtrFromHandle(descHandle.m_hDesc)->resourceChain[g_vulkanContextResources.currentVirtualFrame].descriptorSet;

            vkCmdBindDescriptorSets(commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout, uiDesc, 1, descSet, 0, nullptr);
        }
    }
}

void VulkanRecordCommandMemoryTransfer(uint32 sizeInBytes, ResourceHandle srcBufferHandle, ResourceHandle dstBufferHandle,
    const char* debugLabel, bool immediateSubmit)
{
    VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(immediateSubmit);

#if defined(ENABLE_VULKAN_DEBUG_LABELS)
    VkDebugUtilsLabelEXT label =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        NULL,
        debugLabel,
        { 0.0f, 0.0f, 0.0f, 0.0f },
    };
    g_vulkanContextResources.pfnCmdInsertDebugUtilsLabelEXT(commandBuffer, &label);
#endif

    VulkanMemResourceChain* dstResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(dstBufferHandle.m_hRes);
    VulkanMemResource* dstResource = &dstResourceChain->resourceChain[g_vulkanContextResources.currentVirtualFrame];

    switch (dstResourceChain->resDesc.resourceType)
    {
        case ResourceType::eBuffer1D:
        {
            VkBufferCopy bufferCopy = {};

            for (uint32 uiBuf = 0; uiBuf < VULKAN_MAX_FRAMES_IN_FLIGHT; ++uiBuf)
            {
                bufferCopy.srcOffset = 0; // TODO: make this a function param?
                bufferCopy.size = sizeInBytes;
                bufferCopy.dstOffset = 0;

                // TODO:bad indexing, assumes staging buffers are only one copy
                VkBuffer srcBuffer = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(srcBufferHandle.m_hRes)->resourceChain[0].buffer;
                VkBuffer dstBuffer = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(dstBufferHandle.m_hRes)->resourceChain[uiBuf].buffer;
                vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);
            }

            break;
        }

        case ResourceType::eImage2D:
        {
            uint32 bytesPerPixel = 0;
            switch (dstResourceChain->resDesc.imageFormat)
            {
                case ImageFormat::BGRA8_SRGB:
                case ImageFormat::RGBA8_SRGB:
                {
                    bytesPerPixel = 32 / 8;
                    break;
                }

                default:
                {
                    Core::Utility::LogMsg("Platform", "Unsupported image copy dst format!", Core::Utility::LogSeverity::eCritical);
                    TINKER_ASSERT(0);
                    return;
                }
            }

            for (uint32 uiImg = 0; uiImg < VULKAN_MAX_FRAMES_IN_FLIGHT; ++uiImg)
            {
                VkBuffer& srcBuffer = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(srcBufferHandle.m_hRes)->resourceChain[0].buffer;
                VkImage& dstImage = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(dstBufferHandle.m_hRes)->resourceChain[uiImg].image;

                // TODO: make some of these function params
                VkBufferImageCopy region = {};
                region.bufferOffset = 0;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = 0;
                region.imageSubresource.baseArrayLayer = 0;
                region.imageSubresource.layerCount = dstResourceChain->resDesc.arrayEles;
                region.imageOffset = { 0, 0, 0 };
                region.imageExtent = { dstResourceChain->resDesc.dims.x, dstResourceChain->resDesc.dims.y, dstResourceChain->resDesc.dims.z };

                vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            }
            break;
        }
    }
}

void VulkanRecordCommandRenderPassBegin(uint32 numColorRTs, const ResourceHandle* colorRTs, ResourceHandle depthRT, uint32 renderWidth, uint32 renderHeight,
    const char* debugLabel, bool immediateSubmit)
{
    const bool HasDepth = depthRT.m_hRes != TINKER_INVALID_HANDLE;
    const uint32 numAttachments = numColorRTs + (HasDepth ? 1u : 0u);

    if (HasDepth)
        TINKER_ASSERT(numAttachments <= VULKAN_MAX_RENDERTARGETS_WITH_DEPTH);
    else
        TINKER_ASSERT(numAttachments <= VULKAN_MAX_RENDERTARGETS);

    VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(immediateSubmit);
    
    VkRenderingAttachmentInfo colorAttachments[VULKAN_MAX_RENDERTARGETS] = {};
    for (uint32 i = 0; i < min(numColorRTs, VULKAN_MAX_RENDERTARGETS); ++i)
    {
        VkRenderingAttachmentInfo& colorAttachment = colorAttachments[i];
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };

        if (colorRTs[i] == IMAGE_HANDLE_SWAP_CHAIN)
        {
            colorAttachment.imageView = g_vulkanContextResources.swapChainImageViews[g_vulkanContextResources.currentSwapChainImage];
        }
        else
        {
            VulkanMemResource* resource =
                &g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(colorRTs[i].m_hRes)->resourceChain[g_vulkanContextResources.currentVirtualFrame];
            colorAttachment.imageView = resource->imageView;
        }
    }

    VkRenderingAttachmentInfo depthAttachment = {};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.color = { DEPTH_MAX, 0 };
    if (HasDepth)
    {
        VulkanMemResource* resource =
            &g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(depthRT.m_hRes)->resourceChain[g_vulkanContextResources.currentVirtualFrame];
        depthAttachment.imageView = resource->imageView;
    }

    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = { 0, 0, renderWidth, renderHeight };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = numColorRTs;
    renderingInfo.pColorAttachments = numColorRTs ? colorAttachments : nullptr;
    renderingInfo.pDepthAttachment = HasDepth ? &depthAttachment : nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport = { 0, 0, (float)renderWidth, (float)renderHeight, DEPTH_MIN, DEPTH_MAX };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = { 0, 0, renderWidth, renderHeight };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    /*
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderArea.extent = VkExtent2D({ renderWidth, renderHeight });

    FramebufferHandle framebuffer = framebufferHandle;
    if (framebuffer == DefaultFramebufferHandle_Invalid)
    {
        renderPassBeginInfo.framebuffer = g_vulkanContextResources.swapChainFramebuffers[g_vulkanContextResources.currentSwapChainImage];
        renderPassBeginInfo.renderArea.extent = g_vulkanContextResources.swapChainExtent;

        VkClearValue swapChainClearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
        renderPassBeginInfo.pClearValues = &swapChainClearValue;
        renderPassBeginInfo.clearValueCount = 1;
    }
    else
    {
        VulkanFramebufferResource* framebufferPtr =
            &g_vulkanContextResources.vulkanFramebufferResourcePool.PtrFromHandle(framebuffer.m_hFramebuffer)->resourceChain[g_vulkanContextResources.currentVirtualFrame];
        renderPassBeginInfo.framebuffer = framebufferPtr->framebuffer;
        renderPassBeginInfo.clearValueCount = framebufferPtr->numClearValues;
        renderPassBeginInfo.pClearValues = framebufferPtr->clearValues;
    }
    renderPassBeginInfo.renderPass = g_vulkanContextResources.renderPasses[renderPassID].renderPassVk;*/

#if defined(ENABLE_VULKAN_DEBUG_LABELS)
    VkDebugUtilsLabelEXT label =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        NULL,
        debugLabel,
        { 0.0f, 0.0f, 0.0f, 0.0f },
    };
    g_vulkanContextResources.pfnCmdBeginDebugUtilsLabelEXT(commandBuffer, &label);
#endif
}

void VulkanRecordCommandRenderPassEnd(bool immediateSubmit)
{
    VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(immediateSubmit);

    vkCmdEndRendering(commandBuffer);

/*
#if defined(ENABLE_VULKAN_DEBUG_LABELS)
    g_vulkanContextResources.pfnCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
*/
}

void VulkanRecordCommandTransitionLayout(ResourceHandle imageHandle,
    uint32 startLayout, uint32 endLayout, const char* debugLabel, bool immediateSubmit)
{
    if (startLayout == endLayout)
    {
        // Useless transition, don't record it
        TINKER_ASSERT(0);
        return;
    }

    VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(immediateSubmit);

#if defined(ENABLE_VULKAN_DEBUG_LABELS)
    VkDebugUtilsLabelEXT label =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        NULL,
        debugLabel,
        { 0.0f, 0.0f, 0.0f, 0.0f },
    };
    g_vulkanContextResources.pfnCmdInsertDebugUtilsLabelEXT(commandBuffer, &label);
#endif

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // TODO: probably change this
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.oldLayout = GetVkImageLayout(startLayout);
    barrier.newLayout = GetVkImageLayout(endLayout);

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    switch (startLayout)
    {
        case ImageLayout::eUndefined:
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.srcAccessMask = 0;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        }

        case ImageLayout::eShaderRead:
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        }

        case ImageLayout::eTransferDst:
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        }

        case ImageLayout::eDepthOptimal:
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        }

        case ImageLayout::eRenderOptimal:
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid dst image resource layout specified for layout transition!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
            return;
        }
    }

    switch (endLayout)
    {
        case ImageLayout::eUndefined:
        {
            TINKER_ASSERT(0);
            // Can't transition to undefined according to Vulkan spec
            return;
        }

        case ImageLayout::eShaderRead:
        {
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        }

        case ImageLayout::eTransferDst:
        {
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        }

        case ImageLayout::eDepthOptimal:
        {
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dstStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        }

        case ImageLayout::eRenderOptimal:
        {
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        }

        case ImageLayout::ePresent:
        {
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid src image resource layout specified for layout transition!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
            return;
        }
    }

    VkImage image = VK_NULL_HANDLE;
    uint32 numArrayEles = 0;
    if (imageHandle == IMAGE_HANDLE_SWAP_CHAIN)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image = g_vulkanContextResources.swapChainImages[g_vulkanContextResources.currentSwapChainImage];
        numArrayEles = 1;
    }
    else
    {
        VulkanMemResourceChain* memResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(imageHandle.m_hRes);
        VulkanMemResource* memResource = &memResourceChain->resourceChain[g_vulkanContextResources.currentVirtualFrame];
        image = memResource->image;
        numArrayEles = memResourceChain->resDesc.arrayEles;

        switch (memResourceChain->resDesc.imageFormat)
        {
            case ImageFormat::BGRA8_SRGB:
            case ImageFormat::RGBA8_SRGB:
            {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                break;
            }

            case ImageFormat::Depth_32F:
            {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                break;
            }

            default:
            {
                Core::Utility::LogMsg("Platform", "Invalid image format for layout transition command!", Core::Utility::LogSeverity::eCritical);
                TINKER_ASSERT(0);
                return;
            }
        }
    }

    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = numArrayEles;

    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanRecordCommandClearImage(ResourceHandle imageHandle,
    const v4f& clearValue, const char* debugLabel, bool immediateSubmit)
{
    VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(immediateSubmit);

#if defined(ENABLE_VULKAN_DEBUG_LABELS)
    VkDebugUtilsLabelEXT label =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        NULL,
        debugLabel,
        { 0.0f, 0.0f, 0.0f, 0.0f },
    };
    g_vulkanContextResources.pfnCmdInsertDebugUtilsLabelEXT(commandBuffer, &label);
#endif

    VulkanMemResourceChain* memResourceChain = nullptr;
    VulkanMemResource* memResource = nullptr;
    uint32 imageFormat = ImageFormat::Invalid;

    if (imageHandle == IMAGE_HANDLE_SWAP_CHAIN)
    {
        //imageFormat = ImageFormat::TheSwapChainFormat;
        TINKER_ASSERT(0); // don't clear the swap chain for now
    }
    else
    {
        memResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(imageHandle.m_hRes);
        memResource = &memResourceChain->resourceChain[g_vulkanContextResources.currentVirtualFrame];
        imageFormat = memResourceChain->resDesc.imageFormat;
    }

    VkClearColorValue clearColor = {};
    VkClearDepthStencilValue clearDepth = {};

    VkImageSubresourceRange range = {};
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    switch (imageFormat)
    {
        case ImageFormat::BGRA8_SRGB:
        case ImageFormat::RGBA8_SRGB:
        {
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            for (uint32 i = 0; i < 4; ++i)
            {
                clearColor.uint32[i] = *(uint32*)&clearValue[i];
            }

            vkCmdClearColorImage(commandBuffer,
                memResource->image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

            break;
        }

        case ImageFormat::Depth_32F:
        {
            range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            clearDepth.depth = clearValue.x;
            clearDepth.stencil = *(uint32*)&clearValue.y;

            vkCmdClearDepthStencilImage(commandBuffer,
                memResource->image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepth, 1, &range);

            break;
        }

        case ImageFormat::TheSwapChainFormat:
        {
            // TODO: similar to regular color, but need to grab swap chain image handle from vulkan resources rather than following mem resource ptr
        }
        
        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid image format for clear command!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
            return;
        }
    }
}

}
}
}

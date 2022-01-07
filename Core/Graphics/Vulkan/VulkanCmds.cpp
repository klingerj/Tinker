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
    vkWaitForFences(g_vulkanContextResources.device, 1, &g_vulkanContextResources.fences[g_vulkanContextResources.currentFrame], VK_TRUE, (uint64)-1);

    uint32 currentSwapChainImageIndex = TINKER_INVALID_HANDLE;

    VkResult result = vkAcquireNextImageKHR(g_vulkanContextResources.device,
        g_vulkanContextResources.swapChain,
        (uint64)-1,
        g_vulkanContextResources.swapChainImageAvailableSemaphores[g_vulkanContextResources.currentFrame],
        VK_NULL_HANDLE,
        &currentSwapChainImageIndex);

    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to acquire next swap chain image!", Core::Utility::LogSeverity::eInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
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

    if (g_vulkanContextResources.imageInFlightFences[currentSwapChainImageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(g_vulkanContextResources.device, 1, &g_vulkanContextResources.imageInFlightFences[currentSwapChainImageIndex], VK_TRUE, (uint64)-1);

    g_vulkanContextResources.imageInFlightFences[currentSwapChainImageIndex] = g_vulkanContextResources.fences[g_vulkanContextResources.currentFrame];
    g_vulkanContextResources.currentSwapChainImage = currentSwapChainImageIndex;
    return true;
}

void VulkanSubmitFrame()
{
    // Submit
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[1] = { g_vulkanContextResources.swapChainImageAvailableSemaphores[g_vulkanContextResources.currentFrame] };
    VkPipelineStageFlags waitStages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &g_vulkanContextResources.commandBuffers[g_vulkanContextResources.currentSwapChainImage];

    VkSemaphore signalSemaphores[1] = { g_vulkanContextResources.renderCompleteSemaphores[g_vulkanContextResources.currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(g_vulkanContextResources.device, 1, &g_vulkanContextResources.fences[g_vulkanContextResources.currentFrame]);
    VkResult result = vkQueueSubmit(g_vulkanContextResources.graphicsQueue, 1, &submitInfo, g_vulkanContextResources.fences[g_vulkanContextResources.currentFrame]);
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
            TINKER_ASSERT(0);
            /*Core::Utility::LogMsg("Platform", "Recreating swap chain!", Core::Utility::LogSeverity::eInfo);
            VulkanDestroySwapChain(g_vulkanContextResources);
            VulkanCreateSwapChain(g_vulkanContextResources);
            return; // Don't present on this frame*/
        }
        else
        {
            Core::Utility::LogMsg("Platform", "Not recreating swap chain!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
        }
    }

    g_vulkanContextResources.currentFrame = (g_vulkanContextResources.currentFrame + 1) % VULKAN_MAX_FRAMES_IN_FLIGHT;

    //vkQueueWaitIdle(g_vulkanContextResources.presentationQueue);
}

void BeginVulkanCommandRecording()
{
    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = 0;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    VkResult result = vkBeginCommandBuffer(g_vulkanContextResources.commandBuffers[g_vulkanContextResources.currentSwapChainImage], &commandBufferBeginInfo);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to begin Vulkan command buffer!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
}

void EndVulkanCommandRecording()
{
    if (g_vulkanContextResources.currentSwapChainImage == TINKER_INVALID_HANDLE)
    {
        TINKER_ASSERT(0);
    }

    VkResult result = vkEndCommandBuffer(g_vulkanContextResources.commandBuffers[g_vulkanContextResources.currentSwapChainImage]);
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
        TINKER_ASSERT(g_vulkanContextResources.currentSwapChainImage != TINKER_INVALID_HANDLE);
        commandBuffer = g_vulkanContextResources.commandBuffers[g_vulkanContextResources.currentSwapChainImage];
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

    uint32 currentSwapChainImage = g_vulkanContextResources.currentSwapChainImage;

    // Index buffer
    VulkanMemResourceChain* indexBufferResource = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(indexBufferHandle.m_hRes);
    //uint32 indexBufferSize = indexBufferResource->resDesc.dims.x;
    //TODO: bad indexing
    VkBuffer& indexBuffer = indexBufferResource->resourceChain[g_vulkanContextResources.currentSwapChainImage].buffer;
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
                &g_vulkanContextResources.vulkanDescriptorResourcePool.PtrFromHandle(descHandle.m_hDesc)->resourceChain[g_vulkanContextResources.currentSwapChainImage].descriptorSet;

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
    VulkanMemResource* dstResource = &dstResourceChain->resourceChain[g_vulkanContextResources.currentSwapChainImage];

    switch (dstResourceChain->resDesc.resourceType)
    {
        case ResourceType::eBuffer1D:
        {
            VkBufferCopy bufferCopy = {};

            for (uint32 uiBuf = 0; uiBuf < g_vulkanContextResources.numSwapChainImages; ++uiBuf)
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

            for (uint32 uiImg = 0; uiImg < g_vulkanContextResources.numSwapChainImages; ++uiImg)
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

void VulkanRecordCommandRenderPassBegin(FramebufferHandle framebufferHandle, uint32 renderPassID, uint32 renderWidth, uint32 renderHeight,
    const char* debugLabel, bool immediateSubmit)
{
    VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(immediateSubmit);

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderArea.extent = VkExtent2D({ renderWidth, renderHeight });

    FramebufferHandle framebuffer = framebufferHandle;
    if (framebuffer == DefaultFramebufferHandle_Invalid)
    {
        framebuffer = g_vulkanContextResources.swapChainFramebufferHandle;
        renderPassBeginInfo.renderArea.extent = g_vulkanContextResources.swapChainExtent;
    }
    VulkanFramebufferResource* framebufferPtr =
        &g_vulkanContextResources.vulkanFramebufferResourcePool.PtrFromHandle(framebuffer.m_hFramebuffer)->resourceChain[g_vulkanContextResources.currentSwapChainImage];
    renderPassBeginInfo.framebuffer = framebufferPtr->framebuffer;
    renderPassBeginInfo.renderPass = g_vulkanContextResources.renderPasses[renderPassID].renderPassVk;

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
    g_vulkanContextResources.pfnCmdBeginDebugUtilsLabelEXT(commandBuffer, &label);
#endif

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRecordCommandRenderPassEnd(bool immediateSubmit)
{
    VkCommandBuffer commandBuffer = ChooseAppropriateCommandBuffer(immediateSubmit);

    vkCmdEndRenderPass(commandBuffer);

#if defined(ENABLE_VULKAN_DEBUG_LABELS)
    g_vulkanContextResources.pfnCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
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

    VulkanMemResourceChain* memResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(imageHandle.m_hRes);

    for (uint32 uiImg = 0; uiImg < g_vulkanContextResources.numSwapChainImages; ++uiImg)
    {
        VulkanMemResource* memResource = &memResourceChain->resourceChain[uiImg];

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

            default:
            {
                Core::Utility::LogMsg("Platform", "Invalid src image resource layout specified for layout transition!", Core::Utility::LogSeverity::eCritical);
                TINKER_ASSERT(0);
                return;
            }
        }

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

        barrier.image = memResource->image;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = memResourceChain->resDesc.arrayEles;

        vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
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

    VulkanMemResourceChain* memResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(imageHandle.m_hRes);
    VulkanMemResource* memResource = &memResourceChain->resourceChain[g_vulkanContextResources.currentSwapChainImage];

    VkClearColorValue clearColor = {};
    VkClearDepthStencilValue clearDepth = {};

    VkImageSubresourceRange range = {};
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    switch (memResourceChain->resDesc.imageFormat)
    {
        case ImageFormat::BGRA8_SRGB:
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

        case ImageFormat::RGBA8_SRGB:
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

        case ImageFormat::Depth_32F:
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
            Core::Utility::LogMsg("Platform", "Invalid image format for clear command!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
            return;
        }
    }
}

}
}
}

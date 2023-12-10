#include "Graphics/Common/GraphicsCommon.h"
#include "Graphics/Vulkan/Vulkan.h"
#include "Graphics/Vulkan/VulkanTypes.h"
#include "Utility/Logging.h"
#include "DataStructures/Vector.h"

namespace Tk
{
namespace Graphics
{

static VkCommandBuffer ExtractVkCommandBuffer(CommandBuffer commandBuffer)
{
    return (VkCommandBuffer)commandBuffer.apiObjectHandles[g_vulkanContextResources.currentVirtualFrame];
}

bool VulkanAcquireFrame(SwapChainData* swapChainData)
{
    VulkanSwapChainData* vulkanSwapChainData = g_vulkanContextResources.vulkanSwapChainDataPool.PtrFromHandle(swapChainData->swapChainAPIObjectsHandle);
    const VulkanVirtualFrameSyncData& virtualFrameSyncData = vulkanSwapChainData->virtualFrameSyncData[g_vulkanContextResources.currentVirtualFrame];

    VkResult result = vkWaitForFences(g_vulkanContextResources.device, 1, &virtualFrameSyncData.Fence, VK_FALSE, (uint64)-1);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Waiting for virtual frame fence took too long!", Core::Utility::LogSeverity::eInfo);
        return false;
    }
    vkResetFences(g_vulkanContextResources.device, 1, &virtualFrameSyncData.Fence);

    uint32 currentSwapChainImageIndex = TINKER_INVALID_HANDLE;

    result = vkAcquireNextImageKHR(g_vulkanContextResources.device,
        vulkanSwapChainData->swapChain,
        (uint64)-1,
        virtualFrameSyncData.ImageAvailableSema,
        VK_NULL_HANDLE,
        &currentSwapChainImageIndex);

    // Error checking
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        Core::Utility::LogMsg("Platform", "Failed to acquire next swap chain image!", Core::Utility::LogSeverity::eInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            Core::Utility::LogMsg("Platform", "out of date swap chain!", Core::Utility::LogSeverity::eInfo);
        }
        else
        {
            Core::Utility::LogMsg("Platform", "Not recreating swap chain!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0); // we will probably crash
        }
        return false; // Don't present on this frame
    }

    swapChainData->currentSwapChainImage = currentSwapChainImageIndex;
    return true;
}

void VulkanSubmitFrame(SwapChainData* swapChainData, CommandBuffer commandBuffer)
{
    VulkanSwapChainData* vulkanSwapChainData = g_vulkanContextResources.vulkanSwapChainDataPool.PtrFromHandle(swapChainData->swapChainAPIObjectsHandle);
    const VulkanVirtualFrameSyncData& virtualFrameSyncData = vulkanSwapChainData->virtualFrameSyncData[g_vulkanContextResources.currentVirtualFrame];

    // Submit
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkCommandBuffer vkCommandBuffer = ExtractVkCommandBuffer(commandBuffer);

    VkSemaphore waitSemaphores[1] = { virtualFrameSyncData.ImageAvailableSema };
    VkPipelineStageFlags waitStages[1] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCommandBuffer;

    VkSemaphore signalSemaphores[1] = { virtualFrameSyncData.GPUWorkCompleteSema };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkResult result = vkQueueSubmit(g_vulkanContextResources.graphicsQueue, 1, &submitInfo, virtualFrameSyncData.Fence);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to submit command buffer to queue!", Core::Utility::LogSeverity::eCritical);
    }
}

void VulkanPresentToSwapChain(SwapChainData* swapChainData)
{
    VulkanSwapChainData* vulkanSwapChainData = g_vulkanContextResources.vulkanSwapChainDataPool.PtrFromHandle(swapChainData->swapChainAPIObjectsHandle);
    const VulkanVirtualFrameSyncData& virtualFrameSyncData = vulkanSwapChainData->virtualFrameSyncData[g_vulkanContextResources.currentVirtualFrame];

    VkSemaphore signalSemaphores[1] = { virtualFrameSyncData.GPUWorkCompleteSema };

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vulkanSwapChainData->swapChain;
    presentInfo.pImageIndices = &swapChainData->currentSwapChainImage;

    VkResult result = vkQueuePresentKHR(g_vulkanContextResources.graphicsQueue, &presentInfo);

    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to present swap chain!", Core::Utility::LogSeverity::eInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            Core::Utility::LogMsg("Platform", "Out of date / suboptimal swap chain (probably minimized window)!", Core::Utility::LogSeverity::eInfo);

        }
        else
        {
            Core::Utility::LogMsg("Platform", "Not recreating swap chain!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0); // we will probably crash
        }
        return; // don't present on this frame
    }
}

void EndFrame()
{
    g_vulkanContextResources.currentVirtualFrame = (g_vulkanContextResources.currentVirtualFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    ++g_vulkanContextResources.frameCounter;
}

void* MapResource(ResourceHandle handle)
{
    VulkanMemResourceChain* resourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(handle.m_hRes);
    const ResourceDesc& desc = resourceChain->resDesc;
    VulkanMemResource* resource = &resourceChain->resourceChain[IsBufferUsageMultiBuffered(desc.bufferUsage) ?  g_vulkanContextResources.currentVirtualFrame : 0];

    // Note: Right now, all host visible memory is allocated into the same single device memory block, which I just leave persistently mapped. So this just has to return the mapped ptr + offset.
    if (1)
    {
        return (void*)((uint8*)resource->GpuMemAlloc.mappedMemPtr + resource->GpuMemAlloc.allocOffset);
    }
    else
    {
        void* newMappedMem;
        VkResult result = vkMapMemory(g_vulkanContextResources.device, resource->GpuMemAlloc.allocMem, 0, VK_WHOLE_SIZE, 0, &newMappedMem);

        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Platform", "Failed to map gpu memory!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
            return nullptr;
        }

        return newMappedMem;
    }
}

void UnmapResource(ResourceHandle handle)
{
    VulkanMemResourceChain* resourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(handle.m_hRes);
    const ResourceDesc& desc = resourceChain->resDesc;
    VulkanMemResource* resource = &resourceChain->resourceChain[IsBufferUsageMultiBuffered(desc.bufferUsage) ? g_vulkanContextResources.currentVirtualFrame : 0];

    // Flush right before unmapping
    VkMappedMemoryRange memoryRange = {};
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.memory = resource->GpuMemAlloc.allocMem;
    memoryRange.offset = resource->GpuMemAlloc.allocOffset;
    memoryRange.size = resource->GpuMemAlloc.allocSize;

    VkResult result = vkFlushMappedMemoryRanges(g_vulkanContextResources.device, 1, &memoryRange);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to flush mapped gpu memory!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    if (0)
    {
        vkUnmapMemory(g_vulkanContextResources.device, resource->GpuMemAlloc.allocMem);
    }
}

WRITE_DESCRIPTOR_ARRAY(WriteDescriptorArray)
{
    DescriptorLayout* descLayout = &g_vulkanContextResources.descLayouts[descSetHandle.m_layoutID].bindings;

    uint32 uiFrameCounter = 0;
    uint32 uiFrameCounterMax = MAX_FRAMES_IN_FLIGHT;
    if (updateFlags & DescUpdateConfigFlags::Transient)
    {
        uiFrameCounter = g_vulkanContextResources.currentVirtualFrame;
        uiFrameCounterMax = uiFrameCounter + 1;
    }

    for (; uiFrameCounter < uiFrameCounterMax; ++uiFrameCounter)
    {
        VkDescriptorSet* descriptorSet =
            &g_vulkanContextResources.vulkanDescriptorResourcePool.PtrFromHandle(descSetHandle.m_hDesc)->resourceChain[uiFrameCounter].descriptorSet;

        // TODO: clean up the memory allocation here

        // Descriptor layout
        static Tk::Core::Vector<VkWriteDescriptorSet> descSetWrites;
        descSetWrites.Reserve(DESCRIPTOR_BINDLESS_ARRAY_LIMIT);
        descSetWrites.Clear();

        // Desc set info data
        static Tk::Core::Vector<VkDescriptorBufferInfo> descBufferInfo[MAX_BINDINGS_PER_SET];
        static Tk::Core::Vector<VkDescriptorImageInfo> descImageInfo[MAX_BINDINGS_PER_SET];

        for (uint32 uiDesc = 0; uiDesc < MAX_BINDINGS_PER_SET; ++uiDesc)
        {
            descBufferInfo[uiDesc].Clear();
            descBufferInfo[uiDesc].Resize(DESCRIPTOR_BINDLESS_ARRAY_LIMIT);
            descImageInfo[uiDesc].Clear();
            descImageInfo[uiDesc].Resize(DESCRIPTOR_BINDLESS_ARRAY_LIMIT);
        }

        for (uint32 uiDesc = 0; uiDesc < MAX_BINDINGS_PER_SET; ++uiDesc)
        {
            VkWriteDescriptorSet descSetWrite;
            descSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descSetWrite.pNext = nullptr;

            for (uint32 uiEntry = 0; uiEntry < numEntries; ++uiEntry)
            {
                uint32 type = descLayout->params[uiDesc].type;
                if (type != DescriptorType::eMax)
                {
                    VulkanMemResourceChain* resChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(entries[uiEntry].m_hRes);
                    uint32 resIndex = 0;

                    switch (type)
                    {
                        // TODO: buffers

                        case DescriptorType::eArrayOfTextures:
                        {
                            VkImageView* imageView = &resChain->resourceChain[resIndex].imageView; // Should be index 0 for images

                            descImageInfo[uiDesc][uiEntry].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            descImageInfo[uiDesc][uiEntry].imageView = *imageView;
                            descImageInfo[uiDesc][uiEntry].sampler = g_vulkanContextResources.linearSampler;

                            descSetWrite.dstSet = *descriptorSet;
                            descSetWrite.dstBinding = uiDesc;
                            descSetWrite.dstArrayElement = uiEntry;
                            descSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                            descSetWrite.descriptorCount = 1;
                            descSetWrite.pImageInfo = &descImageInfo[uiDesc][uiEntry];
                            break;
                        }

                        default:
                        {
                            // Don't call this function on "normal" non-array descriptors, it's presumably for bindless. Use WriteDescriptorSimple()
                            TINKER_ASSERT(0);
                            break;
                        }
                    }
                    descSetWrites.PushBackRaw(descSetWrite);
                }
            }
        }

        if (numEntries > 0)
        {
            vkUpdateDescriptorSets(g_vulkanContextResources.device, descSetWrites.Size(), &descSetWrites[0], 0, nullptr);
        }
    }
}

WRITE_DESCRIPTOR_SIMPLE(WriteDescriptorSimple)
{
    DescriptorLayout* descLayout = &g_vulkanContextResources.descLayouts[descSetHandle.m_layoutID].bindings;

    for (uint32 uiFrame = 0; uiFrame < MAX_FRAMES_IN_FLIGHT; ++uiFrame)
    {
        VkDescriptorSet* descriptorSet =
            &g_vulkanContextResources.vulkanDescriptorResourcePool.PtrFromHandle(descSetHandle.m_hDesc)->resourceChain[uiFrame].descriptorSet;

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
                VulkanMemResourceChain* resChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(descSetDataHandles->handles[uiDesc].m_hRes);
                uint32 resIndex = 0;

                switch (type)
                {
                    case DescriptorType::eBuffer:
                    case DescriptorType::eSSBO:
                    {
                        resIndex = IsBufferUsageMultiBuffered(resChain->resDesc.bufferUsage) ? uiFrame : 0u;

                        VkBuffer* buffer = &resChain->resourceChain[resIndex].buffer;

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

                    case DescriptorType::eDynamicBuffer:
                    {
                        break;
                    }

                    case DescriptorType::eSampledImage:
                    {
                        VkImageView* imageView = &resChain->resourceChain[resIndex].imageView; // Should be index 0 for images

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

                    case DescriptorType::eStorageImage:
                    {
                        VkImageView* imageView = &resChain->resourceChain[resIndex].imageView; // Should be index 0 for images

                        descImageInfo[descriptorCount].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                        descImageInfo[descriptorCount].imageView = *imageView;
                        descImageInfo[descriptorCount].sampler = VK_NULL_HANDLE;

                        descSetWrites[descriptorCount].dstSet = *descriptorSet;
                        descSetWrites[descriptorCount].dstBinding = descriptorCount;
                        descSetWrites[descriptorCount].dstArrayElement = 0;
                        descSetWrites[descriptorCount].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                        descSetWrites[descriptorCount].descriptorCount = 1;
                        descSetWrites[descriptorCount].pImageInfo = &descImageInfo[descriptorCount];
                        break;
                    }

                    case DescriptorType::eArrayOfTextures:
                    {
                        // Don't call this function on array descriptors, it's presumably for bindless. Use WriteArrayDescriptor()
                        TINKER_ASSERT(0);
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

void BeginCommandRecording(CommandBuffer commandBuffer)
{
    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = 0;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    VkResult result = vkBeginCommandBuffer(ExtractVkCommandBuffer(commandBuffer), &commandBufferBeginInfo);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to begin Vulkan command buffer!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
}

void EndCommandRecording(CommandBuffer commandBuffer)
{
    VkResult result = vkEndCommandBuffer(ExtractVkCommandBuffer(commandBuffer));
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to end Vulkan command buffer!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }
}

void VulkanSubmitCommandBufferAndWaitImmediate(CommandBuffer commandBuffer)
{
    VkCommandBuffer vkCommandBuffer = ExtractVkCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCommandBuffer;

    VkResult result = vkQueueSubmit(g_vulkanContextResources.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to submit command buffer to queue!", Core::Utility::LogSeverity::eCritical);
    }
    vkQueueWaitIdle(g_vulkanContextResources.graphicsQueue);
}

void RecordCommandPushConstant(CommandBuffer commandBuffer, const uint8* data, uint32 sizeInBytes, uint32 shaderID)
{
    TINKER_ASSERT(data && sizeInBytes);

    const VkPipelineLayout& pipelineLayout = g_vulkanContextResources.psoPermutations.pipelineLayout[shaderID];
    TINKER_ASSERT(pipelineLayout != VK_NULL_HANDLE);

    vkCmdPushConstants(ExtractVkCommandBuffer(commandBuffer), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeInBytes, data);
}

void RecordCommandSetViewport(CommandBuffer commandBuffer, float x, float y, float width, float height, float minDepth, float maxDepth)
{
    // The way this is written does not just pass the parameters thru, it automatically implements flipping of the y-coordinate 
    VkViewport viewport = {};
    viewport.x = x;
    viewport.y = height - y;
    viewport.width = width;
    viewport.height = -height;
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
    vkCmdSetViewport(ExtractVkCommandBuffer(commandBuffer), 0, 1, &viewport);
}

void RecordCommandSetScissor(CommandBuffer commandBuffer, int32 offsetX, int32 offsetY, uint32 width, uint32 height)
{
    VkRect2D scissor = {};
    scissor.offset = { offsetX, offsetY };
    scissor.extent = { width, height };
    vkCmdSetScissor(ExtractVkCommandBuffer(commandBuffer), 0, 1, &scissor);
}

void RecordCommandDrawCall(CommandBuffer commandBuffer, ResourceHandle indexBufferHandle, uint32 numIndices, uint32 numInstances,
    uint32 vertOffset, uint32 indexOffset, const char* debugLabel)
{
    TINKER_ASSERT(indexBufferHandle != DefaultResHandle_Invalid);

    // Index buffer
    VulkanMemResourceChain* indexBufferResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(indexBufferHandle.m_hRes);
    const ResourceDesc& desc = indexBufferResourceChain->resDesc;
    VkBuffer& indexBuffer = indexBufferResourceChain->resourceChain[IsBufferUsageMultiBuffered(desc.bufferUsage) ? g_vulkanContextResources.currentVirtualFrame : 0].buffer;

    VkCommandBuffer vkCommandBuffer = ExtractVkCommandBuffer(commandBuffer);
    vkCmdBindIndexBuffer(vkCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(vkCommandBuffer, numIndices, numInstances, indexOffset, vertOffset, 0);
}

void RecordCommandDispatch(CommandBuffer commandBuffer, uint32 threadGroupX, uint32 threadGroupY, uint32 threadGroupZ, const char* debugLabel)
{
    TINKER_ASSERT(threadGroupX > 0);
    TINKER_ASSERT(threadGroupY > 0);
    TINKER_ASSERT(threadGroupZ > 0);
    vkCmdDispatch(ExtractVkCommandBuffer(commandBuffer), threadGroupX, threadGroupY, threadGroupZ);
}

void RecordCommandBindShader(CommandBuffer commandBuffer, uint32 shaderID, uint32 blendState, uint32 depthState)
{
    TINKER_ASSERT(shaderID < SHADER_ID_MAX);

    const VkPipeline& pipeline = g_vulkanContextResources.psoPermutations.graphicsPipeline[shaderID][blendState][depthState];
    TINKER_ASSERT(pipeline != VK_NULL_HANDLE);
    vkCmdBindPipeline(ExtractVkCommandBuffer(commandBuffer), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void RecordCommandBindComputeShader(CommandBuffer commandBuffer, uint32 shaderID)
{
    TINKER_ASSERT(shaderID < SHADER_ID_COMPUTE_MAX);

    const VkPipeline& pipeline = g_vulkanContextResources.psoPermutations.computePipeline[shaderID];
    TINKER_ASSERT(pipeline != VK_NULL_HANDLE);

    vkCmdBindPipeline(ExtractVkCommandBuffer(commandBuffer), VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
}

void RecordCommandBindDescriptor(CommandBuffer commandBuffer, uint32 shaderID, uint32 bindPoint, const DescriptorHandle descSetHandle, uint32 descSetIndex)
{
    TINKER_ASSERT((shaderID < SHADER_ID_MAX && bindPoint == BindPoint::eGraphics) || (shaderID < SHADER_ID_COMPUTE_MAX && bindPoint == BindPoint::eCompute));
    TINKER_ASSERT(bindPoint < BindPoint::eMax);
    TINKER_ASSERT(descSetHandle != DefaultDescHandle_Invalid);
    const VkPipelineLayout pipelineLayout = bindPoint == BindPoint::eGraphics ? g_vulkanContextResources.psoPermutations.pipelineLayout[shaderID] :
        bindPoint == BindPoint::eCompute ? g_vulkanContextResources.psoPermutations.computePipelineLayout[shaderID] : VK_NULL_HANDLE;
    TINKER_ASSERT(pipelineLayout != VK_NULL_HANDLE);

    VkDescriptorSet* descSet =
        &g_vulkanContextResources.vulkanDescriptorResourcePool.PtrFromHandle(descSetHandle.m_hDesc)->resourceChain[g_vulkanContextResources.currentVirtualFrame].descriptorSet;

    vkCmdBindDescriptorSets(ExtractVkCommandBuffer(commandBuffer), GetVkBindPoint(bindPoint), pipelineLayout, descSetIndex, 1, descSet, 0, nullptr);
}

void RecordCommandMemoryTransfer(CommandBuffer commandBuffer, uint32 sizeInBytes, ResourceHandle srcBufferHandle,
    ResourceHandle dstBufferHandle, const char* debugLabel)
{
    VulkanMemResourceChain* dstResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(dstBufferHandle.m_hRes);
    VulkanMemResourceChain* srcResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(srcBufferHandle.m_hRes);

    VkCommandBuffer vkCommandBuffer = ExtractVkCommandBuffer(commandBuffer);

    switch (dstResourceChain->resDesc.resourceType)
    {
        case ResourceType::eBuffer1D:
        {
            VkBufferCopy bufferCopy = {};
            bufferCopy.srcOffset = 0; // TODO: make this stuff a function param?
            bufferCopy.size = sizeInBytes;
            bufferCopy.dstOffset = 0;

            uint32 srcIndex = IsBufferUsageMultiBuffered(srcResourceChain->resDesc.bufferUsage) ? g_vulkanContextResources.currentVirtualFrame : 0u;
            uint32 dstIndex = IsBufferUsageMultiBuffered(dstResourceChain->resDesc.bufferUsage) ? g_vulkanContextResources.currentVirtualFrame : 0u;
            VkBuffer srcBuffer = srcResourceChain->resourceChain[srcIndex].buffer;
            VkBuffer dstBuffer = dstResourceChain->resourceChain[dstIndex].buffer;
            vkCmdCopyBuffer(vkCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);

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

            {
                // TODO: currently no images are duplicated per frame in flight
                VkBuffer& srcBuffer = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(srcBufferHandle.m_hRes)->resourceChain[0].buffer;
                VkImage& dstImage = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(dstBufferHandle.m_hRes)->resourceChain[0].image;

                // TODO: make some of these into function params
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

                vkCmdCopyBufferToImage(vkCommandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            }
            break;
        }
    }
}

void RecordCommandRenderPassBegin(CommandBuffer commandBuffer, uint32 numColorRTs, const ResourceHandle* colorRTs, ResourceHandle depthRT,
    uint32 renderWidth, uint32 renderHeight, const char* debugLabel)
{
    const bool HasDepth = depthRT.m_hRes != TINKER_INVALID_HANDLE;
    const uint32 numAttachments = numColorRTs + (HasDepth ? 1u : 0u);

    if (HasDepth)
    {
        TINKER_ASSERT(numAttachments <= VULKAN_MAX_RENDERTARGETS_WITH_DEPTH);
    }
    else
    {
        TINKER_ASSERT(numAttachments <= VULKAN_MAX_RENDERTARGETS);
    }
    
    VkRenderingAttachmentInfo colorAttachments[VULKAN_MAX_RENDERTARGETS] = {};
    for (uint32 i = 0; i < Min(numColorRTs, VULKAN_MAX_RENDERTARGETS); ++i)
    {
        VkRenderingAttachmentInfo& colorAttachment = colorAttachments[i];
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };

        TINKER_ASSERT(colorRTs[i].m_hRes != TINKER_INVALID_HANDLE);
        VulkanMemResource* resource = &g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(colorRTs[i].m_hRes)->resourceChain[0]; // Currently, all image resources are not duplicated per frame in flight
        colorAttachment.imageView = resource->imageView;
    }

    VkRenderingAttachmentInfo depthAttachment = {};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.color = { DEPTH_MAX, 0 };
    if (HasDepth)
    {
        VulkanMemResource* resource = &g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(depthRT.m_hRes)->resourceChain[0]; // Currently, all image resources are not duplicated per frame in flight
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

    vkCmdBeginRendering(ExtractVkCommandBuffer(commandBuffer), &renderingInfo);
}

void RecordCommandRenderPassEnd(CommandBuffer commandBuffer)
{
    vkCmdEndRendering(ExtractVkCommandBuffer(commandBuffer));
}

void RecordCommandTransitionLayout(CommandBuffer commandBuffer, ResourceHandle imageHandle,
    uint32 startLayout, uint32 endLayout, const char* debugLabel)
{
    if (startLayout == endLayout)
    {
        // Useless transition / error transition, don't record it
        TINKER_ASSERT(0);
        return;
    }

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

        case ImageLayout::eRenderOptimal:
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        }

        case ImageLayout::eGeneral:
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
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
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
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
            dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            break;
        }

        case ImageLayout::eRenderOptimal:
        {
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        }

        case ImageLayout::eGeneral:
        {
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            break;
        }

        case ImageLayout::ePresent:
        {
            barrier.dstAccessMask = 0;
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
    VulkanMemResourceChain* memResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(imageHandle.m_hRes);
    VulkanMemResource* memResource = &memResourceChain->resourceChain[0]; // Currently, all image resources are not duplicated per frame in flight
    image = memResource->image;
    numArrayEles = memResourceChain->resDesc.arrayEles;

    VkFormat imageFormat = GetVkImageFormat(memResourceChain->resDesc.imageFormat);
    switch (imageFormat)
    {
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            break;
        }

        case VK_FORMAT_D32_SFLOAT:
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

    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = numArrayEles;
    vkCmdPipelineBarrier(ExtractVkCommandBuffer(commandBuffer), srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void RecordCommandClearImage(CommandBuffer commandBuffer, ResourceHandle imageHandle,
    const v4f& clearValue, const char* debugLabel)
{
    VulkanMemResourceChain* memResourceChain = nullptr;
    VulkanMemResource* memResource = nullptr;
    uint32 imageFormat = ImageFormat::Invalid;

    memResourceChain = g_vulkanContextResources.vulkanMemResourcePool.PtrFromHandle(imageHandle.m_hRes);
    memResource = &memResourceChain->resourceChain[0]; // Currently, all image resources are not duplicated per frame in flight
    imageFormat = memResourceChain->resDesc.imageFormat;

    VkClearColorValue clearColor = {};
    VkClearDepthStencilValue clearDepth = {};

    VkImageSubresourceRange range = {};
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    switch (imageFormat)
    {
        case ImageFormat::RGBA16_Float:
        case ImageFormat::BGRA8_SRGB:
        case ImageFormat::RGBA8_SRGB:
        {
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            for (uint32 i = 0; i < 4; ++i)
            {
                clearColor.uint32[i] = *(uint32*)&clearValue[i];
            }

            vkCmdClearColorImage(ExtractVkCommandBuffer(commandBuffer),
                memResource->image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

            break;
        }

        case ImageFormat::Depth_32F:
        {
            range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            clearDepth.depth = clearValue.x;
            clearDepth.stencil = *(uint32*)&clearValue.y;

            vkCmdClearDepthStencilImage(ExtractVkCommandBuffer(commandBuffer),
                memResource->image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepth, 1, &range);

            break;
        }

        case ImageFormat::TheSwapChainFormat:
        {
            // Swap chain probably can't be used for transfer cmds. Use a quad blit instead to draw 0,0,0 to the swap chain
            TINKER_ASSERT(0);
        }
        
        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid image format for clear command!", Core::Utility::LogSeverity::eCritical);
            TINKER_ASSERT(0);
            return;
        }
    }
}

void RecordCommandGPUTimestamp(CommandBuffer commandBuffer, uint32 gpuTimestampID)
{
    uint32 currQueryOffset = g_vulkanContextResources.currentVirtualFrame * GPU_TIMESTAMP_NUM_MAX + gpuTimestampID;
    vkCmdWriteTimestamp(ExtractVkCommandBuffer(commandBuffer), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, g_vulkanContextResources.queryPoolTimestamp, currQueryOffset);
}

void ResolveMostRecentAvailableTimestamps(CommandBuffer commandBuffer, void* gpuTimestampCPUSideBuffer, uint32 numTimestampsInQuery)
{
    uint32 currQueryOffset = g_vulkanContextResources.currentVirtualFrame * GPU_TIMESTAMP_NUM_MAX;

    // Need every virtual frame to happen once before reading real values from older frames
    if (g_vulkanContextResources.frameCounter > MAX_FRAMES_IN_FLIGHT - 1)
    {
        VkResult result = vkGetQueryPoolResults(g_vulkanContextResources.device, g_vulkanContextResources.queryPoolTimestamp, currQueryOffset, numTimestampsInQuery, numTimestampsInQuery * sizeof(uint64), gpuTimestampCPUSideBuffer, sizeof(uint64), VK_QUERY_RESULT_64_BIT);
        if (result != VK_SUCCESS)
        {
            Core::Utility::LogMsg("Graphics", "Failed to get query pool results!", Core::Utility::LogSeverity::eCritical);
        }
    }

    vkCmdResetQueryPool(ExtractVkCommandBuffer(commandBuffer), g_vulkanContextResources.queryPoolTimestamp, currQueryOffset, GPU_TIMESTAMP_NUM_MAX);
}

void RecordCommandDebugMarkerStart(CommandBuffer commandBuffer, const char* debugLabel)
{
    DbgStartMarker(ExtractVkCommandBuffer(commandBuffer), debugLabel);
}

void RecordCommandDebugMarkerEnd(CommandBuffer commandBuffer)
{
    DbgEndMarker(ExtractVkCommandBuffer(commandBuffer));
}

}
}

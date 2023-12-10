#pragma once

#include "CoreDefines.h"
#include "Math/VectorTypes.h"
#include "Platform/PlatformGameAPI.h"
#include "Graphics/Common/GraphicsCommon.h"

namespace Tk
{
namespace Graphics
{

// Init/destroy - called one time
int InitVulkan(const Tk::Platform::WindowHandles* platformWindowHandles);
void DestroyVulkan();

// Frame command recording
bool VulkanAcquireFrame(SwapChainData* swapChainData);
void VulkanSubmitFrame(SwapChainData* swapChainData, CommandBuffer commandBuffer);
void VulkanSubmitCommandBufferAndWaitImmediate(CommandBuffer commandBuffer);
void VulkanPresentToSwapChain(SwapChainData* swapChainData);

void BeginVulkanCommandRecording();
void EndVulkanCommandRecording();

void DestroyAllDescLayouts();


}
}

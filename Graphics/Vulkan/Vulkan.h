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
int InitVulkan(const Tk::Platform::WindowHandles* platformWindowHandles, uint32 width, uint32 height);

// Frame command recording
bool VulkanAcquireFrame();
void VulkanSubmitFrame();

void BeginVulkanCommandRecording();
void EndVulkanCommandRecording();
void BeginVulkanCommandRecordingImmediate();
void EndVulkanCommandRecordingImmediate();

void DestroyAllDescLayouts();


}
}

#pragma once

#include "CoreDefines.h"
#include "Math/VectorTypes.h"
#include "Platform/PlatformGameAPI.h"
#include "Graphics/Common/GraphicsCommon.h"

namespace Tk
{
namespace Graphics
{

typedef struct vulkan_vertex_position
{
    v4f position;
} VulkanVertexPosition;

typedef struct vulkan_vertex_uv
{
    v2f uv;
} VulkanVertexUV;

typedef struct vulkan_vertex_normal
{
    v3f normal;
} VulkanVertexNormal;

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

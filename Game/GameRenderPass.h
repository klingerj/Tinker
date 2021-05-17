#pragma once

#include "PlatformGameAPI.h"

using namespace Tk;
using namespace Platform;

struct GameRenderPass
{
    Platform::FramebufferHandle framebuffer;
    Platform::ShaderHandle shader; // TODO: change this, pick shader perm or have asset manager store shaders
    uint32 renderWidth;
    uint32 renderHeight;
    const char* debugLabel;
};

void StartRenderPass(GameRenderPass* renderPass, GraphicsCommandStream* graphicsCommandStream);
void EndRenderPass(GameRenderPass* renderPass, GraphicsCommandStream* graphicsCommandStream);
void DrawMeshDataCommand(GraphicsCommandStream* graphicsCommandStream, uint32 numIndices,
    uint32 numInstances, ResourceHandle indexBufferHandle, ResourceHandle positionBufferHandle,
    ResourceHandle uvBufferHandle, ResourceHandle normalBufferHandle, ShaderHandle shaderHandle,
    DescriptorSetDescHandles* descriptors, const char* debugLabel);

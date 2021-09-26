#pragma once

#include "Platform/Graphics/GraphicsCommon.h"

struct GameRenderPass
{
    Tk::Platform::Graphics::FramebufferHandle framebuffer;
    uint32 renderPassID;
    uint32 renderWidth;
    uint32 renderHeight;
    const char* debugLabel;
};

void StartRenderPass(GameRenderPass* renderPass, Tk::Platform::Graphics::GraphicsCommandStream* graphicsCommandStream);
void EndRenderPass(GameRenderPass* renderPass, Tk::Platform::Graphics::GraphicsCommandStream* graphicsCommandStream);
void DrawMeshDataCommand(Tk::Platform::Graphics::GraphicsCommandStream* graphicsCommandStream, uint32 numIndices,
    uint32 numInstances, Tk::Platform::Graphics::ResourceHandle indexBufferHandle,
    uint32 shaderID, uint32 blendState, uint32 depthState,
    Tk::Platform::Graphics::DescriptorHandle* descriptors, const char* debugLabel);

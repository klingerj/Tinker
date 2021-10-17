#pragma once

#include "Graphics/Common/GraphicsCommon.h"

struct GameRenderPass
{
    Tk::Core::Graphics::FramebufferHandle framebuffer;
    uint32 renderPassID;
    uint32 renderWidth;
    uint32 renderHeight;
    const char* debugLabel;
};

void StartRenderPass(GameRenderPass* renderPass, Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream);
void EndRenderPass(GameRenderPass* renderPass, Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream);
void DrawMeshDataCommand(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream, uint32 numIndices,
    uint32 numInstances, Tk::Core::Graphics::ResourceHandle indexBufferHandle,
    uint32 shaderID, uint32 blendState, uint32 depthState,
    Tk::Core::Graphics::DescriptorHandle* descriptors, const char* debugLabel);

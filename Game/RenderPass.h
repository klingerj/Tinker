#pragma once

#include "PlatformGameAPI.h"

struct GameRenderPass
{
    Tk::Platform::FramebufferHandle framebuffer;
    Tk::Platform::ShaderHandle shader; // TODO: change this, pick shader perm or have asset manager store shaders
    uint32 renderWidth;
    uint32 renderHeight;
    const char* debugLabel;
};

void StartRenderPass(GameRenderPass* renderPass, Tk::Platform::GraphicsCommandStream* graphicsCommandStream);
void EndRenderPass(GameRenderPass* renderPass, Tk::Platform::GraphicsCommandStream* graphicsCommandStream);
void DrawMeshDataCommand(Tk::Platform::GraphicsCommandStream* graphicsCommandStream, uint32 numIndices,
    uint32 numInstances, Tk::Platform::ResourceHandle indexBufferHandle, Tk::Platform::ShaderHandle shaderHandle,
    Tk::Platform::DescriptorHandle* descriptors, const char* debugLabel);

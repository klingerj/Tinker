#pragma once

#include "../Include/PlatformGameAPI.h"

using namespace Tinker;

typedef struct game_render_pass
{
    Platform::FramebufferHandle framebuffer;
    Platform::ShaderHandle shader; // TODO: change this, pick shader perm or have asset manager store shaders
    Platform::DescriptorSetDescHandles descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER]; // TODO: this shouldn't be owned by the render pass
    uint32 renderWidth;
    uint32 renderHeight;
    const char* debugLabel;
} GameRenderPass;

void RecordAllCommands(GameRenderPass* renderPass, const Platform::PlatformAPIFuncs* platformFuncs, Platform::GraphicsCommandStream* graphicsCommandStream);

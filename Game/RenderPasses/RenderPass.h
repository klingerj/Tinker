#pragma once

#include "Graphics/Common/GraphicsCommon.h"

typedef struct frame_render_params
{
  uint32 swapChainWidth;
  uint32 swapChainHeight;
} FrameRenderParams;

struct GameRenderPass;
#define RENDER_PASS_EXEC_FUNC(name)                                                      \
  void name(GameRenderPass* renderPass,                                                  \
            Tk::Graphics::GraphicsCommandStream* graphicsCommandStream,                  \
            const FrameRenderParams& frameRenderParams)

inline RENDER_PASS_EXEC_FUNC(RenderPassExecStub)
{
  TINKER_ASSERT(0 && "Render Pass Exec Func empty!\n");
}

struct GameRenderPass
{
  Tk::Graphics::ResourceHandle inputResources[2]; // TODO: revisit :) 
  Tk::Graphics::ResourceHandle colorRTs[MAX_MULTIPLE_RENDERTARGETS];
  Tk::Graphics::ResourceHandle depthRT;
  const char* debugLabel;
  uint32 numColorRTs;

  typedef RENDER_PASS_EXEC_FUNC(RenderPassExecuteFunc);
  RenderPassExecuteFunc* ExecuteFn = RenderPassExecStub;

  void Init()
  {
    for (uint32 i = 0; i < ARRAYCOUNT(colorRTs); ++i)
    {
      colorRTs[i] = Tk::Graphics::DefaultResHandle_Invalid;
    }
    for (uint32 i = 0; i < ARRAYCOUNT(inputResources); ++i)
    {
      inputResources[i] = Tk::Graphics::DefaultResHandle_Invalid;
    }
    depthRT = Tk::Graphics::DefaultResHandle_Invalid;
    numColorRTs = 0;
    debugLabel = NULL;
  }
};

void StartRenderPass(GameRenderPass* renderPass,
                     Tk::Graphics::GraphicsCommandStream* graphicsCommandStream,
                     uint32 renderWidth, uint32 renderHeight);
void EndRenderPass(GameRenderPass* renderPass,
                   Tk::Graphics::GraphicsCommandStream* graphicsCommandStream);

struct View;
struct Scene;
void RecordRenderPassCommands(GameRenderPass* renderPass, View* view, Scene* scene,
                              Tk::Graphics::GraphicsCommandStream* graphicsCommandStream,
                              uint32 shaderID, uint32 blendState, uint32 depthState,
                              Tk::Graphics::DescriptorHandle* descriptors);

#pragma once

#include "Graphics/Common/GraphicsCommon.h"

struct GameRenderPass;
#define RENDER_PASS_EXEC_FUNC(name)                                                      \
  void name(GameRenderPass* renderPass,                                                  \
            Tk::Graphics::GraphicsCommandStream* graphicsCommandStream)

inline RENDER_PASS_EXEC_FUNC(RenderPassExecStub)
{
  TINKER_ASSERT(0 && "Render Pass Exec Func empty!\n");
}

struct GameRenderPass
{
  Tk::Graphics::ResourceHandle colorRTs[MAX_MULTIPLE_RENDERTARGETS];
  Tk::Graphics::ResourceHandle depthRT;
  const char* debugLabel;
  uint32 numColorRTs;
  uint32 renderWidth;
  uint32 renderHeight;

  typedef RENDER_PASS_EXEC_FUNC(RenderPassExecuteFunc);
  RenderPassExecuteFunc* ExecuteFn = RenderPassExecStub;

  void Init()
  {
    numColorRTs = 0;
    renderWidth = 0;
    renderWidth = 0;
    debugLabel = NULL;
    depthRT = Tk::Graphics::DefaultResHandle_Invalid;
    for (uint32 i = 0; i < ARRAYCOUNT(colorRTs); ++i)
    {
      colorRTs[i] = Tk::Graphics::DefaultResHandle_Invalid;
    }
  }
};

void StartRenderPass(GameRenderPass* renderPass,
                     Tk::Graphics::GraphicsCommandStream* graphicsCommandStream);
void EndRenderPass(GameRenderPass* renderPass,
                   Tk::Graphics::GraphicsCommandStream* graphicsCommandStream);

struct View;
struct Scene;
void RecordRenderPassCommands(GameRenderPass* renderPass, View* view, Scene* scene,
                              Tk::Graphics::GraphicsCommandStream* graphicsCommandStream,
                              uint32 shaderID, uint32 blendState, uint32 depthState,
                              Tk::Graphics::DescriptorHandle* descriptors);

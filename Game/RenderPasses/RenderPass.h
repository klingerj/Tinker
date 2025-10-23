#pragma once

#include "Graphics/Common/GraphicsCommon.h"
#include "Game/GraphicsTypes.h"

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
  Tk::Graphics::DescriptorGroup descriptorGroup;
  const char* debugLabel;
  uint32 numColorRTs;
  uint32 numInputResources;

  typedef RENDER_PASS_EXEC_FUNC(RenderPassExecuteFunc);
  RenderPassExecuteFunc* ExecuteFn = RenderPassExecStub;

  void Init()
  {
    for (auto& colorRT : colorRTs)
    {
      colorRT = Tk::Graphics::DefaultResHandle_Invalid;
    }

    for (auto& inputResource : inputResources)
    {
      inputResource = Tk::Graphics::DefaultResHandle_Invalid;
    }

    descriptorGroup.Init();
    depthRT = Tk::Graphics::DefaultResHandle_Invalid;
    debugLabel = NULL;
    numColorRTs = 0;
    numInputResources = 0;
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

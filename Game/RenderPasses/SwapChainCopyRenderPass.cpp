#include "SwapChainCopyRenderPass.h"
#include "Game/GraphicsTypes.h"
#include "Platform/PlatformGameApi.h"

extern GameGraphicsData gameGraphicsData;

namespace SwapChainCopyRenderPass
{
  RENDER_PASS_EXEC_FUNC(Execute)
  {
    graphicsCommandStream->CmdLayoutTransition(
      gameGraphicsData.m_computeColorHandle, Tk::Graphics::ImageLayout::eRenderOptimal,
      Tk::Graphics::ImageLayout::eShaderRead, "Transition hdr compute to shader read");
    graphicsCommandStream->CmdLayoutTransition(
      Tk::Graphics::GetCurrentSwapChainImage(Tk::Platform::GetPlatformWindowHandles()),
      Tk::Graphics::ImageLayout::eUndefined, Tk::Graphics::ImageLayout::eRenderOptimal,
      "Transition swap chain to render_optimal");

    StartRenderPass(renderPass, graphicsCommandStream, frameRenderParams.swapChainWidth,
                    frameRenderParams.swapChainHeight);
    // TODO: pass descriptors to the render pass via the render graph 
    // also, should probably just expand it to have inputs and outputs... 
    Tk::Graphics::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
      descriptors[i] = Tk::Graphics::DefaultDescHandle_Invalid;
    }
    descriptors[0] = gameGraphicsData.m_swapChainCopyDescHandle;
    descriptors[1] = defaultQuad.m_descriptor;

    graphicsCommandStream->CmdDraw(
      DEFAULT_QUAD_NUM_INDICES, 1, 0, 0, Tk::Graphics::SHADER_ID_QUAD_BLIT_RGBA8,
      Tk::Graphics::BlendState::eReplace, Tk::Graphics::DepthState::eOff_NoCull,
      defaultQuad.m_indexBuffer.gpuBufferHandle, MAX_DESCRIPTOR_SETS_PER_SHADER,
      descriptors, "Draw asset");
    EndRenderPass(renderPass, graphicsCommandStream);

    graphicsCommandStream->CmdLayoutTransition(
      Tk::Graphics::GetCurrentSwapChainImage(Tk::Platform::GetPlatformWindowHandles()),
      Tk::Graphics::ImageLayout::eRenderOptimal, Tk::Graphics::ImageLayout::ePresent,
      "Transition swap chain to present");
  }
} //namespace SwapChainCopyRenderPass

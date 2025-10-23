#include "ZPrepassRenderPass.h"
#include "Game/BindlessSystem.h"
#include "Game/GraphicsTypes.h"

extern GameGraphicsData gameGraphicsData;
extern Scene MainScene;
extern View MainView;

namespace ZPrepassRenderPass
{
  RENDER_PASS_EXEC_FUNC(Execute)
  {
    graphicsCommandStream->CmdLayoutTransition(
      renderPass->depthRT, Tk::Graphics::ImageLayout::eUndefined,
      Tk::Graphics::ImageLayout::eTransferDst,
      "Transition main view depth to transfer_dst");
    graphicsCommandStream->CmdClear(renderPass->depthRT, v4f(1.0f, 0.0f, 0.0f, 0.0f),
                                    "Clear main view depth buffer");
    graphicsCommandStream->CmdLayoutTransition(
      renderPass->depthRT, Tk::Graphics::ImageLayout::eTransferDst,
      Tk::Graphics::ImageLayout::eDepthOptimal,
      "Transition main view depth to depth_attachment_optimal");

    StartRenderPass(renderPass, graphicsCommandStream, frameRenderParams.swapChainWidth,
                    frameRenderParams.swapChainHeight);
    RecordRenderPassCommands(renderPass, &MainView, &MainScene, graphicsCommandStream,
                             Tk::Graphics::SHADER_ID_BASIC_ZPrepass,
                             Tk::Graphics::BlendState::eNoColorAttachment,
                             Tk::Graphics::DepthState::eTestOnWriteOn_CCW, renderPass->descriptorGroup.descriptors);
    EndRenderPass(renderPass, graphicsCommandStream);
  }
} //namespace ZPrepassRenderPass

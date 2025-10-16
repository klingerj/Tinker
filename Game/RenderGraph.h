#pragma once

#include "RenderPasses/RenderPass.h"

namespace Tk
{
  namespace Graphics { struct GraphicsCommandStream; }

  namespace Platform { struct WindowHandles; }
} //namespace Tk

namespace RenderGraph
{
  void Create(const FrameRenderParams& frameRenderParams);
  void Destroy();
  void Run(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream,
           const FrameRenderParams& frameRenderParams,
           const Tk::Platform::WindowHandles* windowHandles);
} //namespace RenderGraph

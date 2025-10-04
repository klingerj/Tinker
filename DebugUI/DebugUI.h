#pragma once

namespace Tk
{
  namespace Graphics
  {
    struct GraphicsCommandStream;
    struct ResourceHandle;
  } //namespace Graphics
} //namespace Tk

namespace DebugUI
{
  void Init(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream);
  void Shutdown();
  void NewFrame();
  void Render(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream,
              Tk::Graphics::ResourceHandle renderTarget);
  void RenderAndSubmitMultiViewports(
    Tk::Graphics::GraphicsCommandStream* graphicsCommandStream);
  void ToggleEnable();

  void UI_MainMenu();
  void UI_PerformanceOverview();
  void UI_RenderPassStats();
} //namespace DebugUI

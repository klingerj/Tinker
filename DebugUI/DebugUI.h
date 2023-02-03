#pragma once

namespace Tk
{
namespace Core
{
namespace Graphics
{
    struct GraphicsCommandStream;
    struct ResourceHandle;
}
}
}

namespace DebugUI
{
    void Init(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream);
    void Shutdown();
    void NewFrame();
    void Render(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream, Tk::Core::Graphics::ResourceHandle renderTarget);

    void UI_RenderPassStats();
}

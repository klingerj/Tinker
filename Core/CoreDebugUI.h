#pragma once

namespace Tk
{
namespace Core
{

namespace Graphics
{
    struct GraphicsCommandStream;
}

namespace DebugUI
{
    void Init(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream);
    void Shutdown();
    void Render(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream);

    void UI_RenderPassStats();
}
}
}

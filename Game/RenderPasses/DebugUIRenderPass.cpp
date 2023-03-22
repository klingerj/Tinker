#include "DebugUIRenderPass.h"

#include "Game/GraphicsTypes.h"

extern GameGraphicsData gameGraphicsData;

namespace DebugUIRenderPass
{

    RENDER_PASS_EXEC_FUNC(Execute)
    {
        DebugUI::Render(graphicsCommandStream, gameGraphicsData.m_rtColorHandle);
    }

}

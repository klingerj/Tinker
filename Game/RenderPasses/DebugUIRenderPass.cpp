#include "DebugUIRenderPass.h"

extern GameGraphicsData gameGraphicsData;

namespace DebugUIRenderPass
{

    RENDER_PASS_EXEC_FUNC(Execute)
    {
        DebugUI::Render(graphicsCommandStream, renderPass->colorRTs[0]);
    }

}

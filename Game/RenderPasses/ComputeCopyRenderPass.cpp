#include "ComputeCopyRenderPass.h"

extern GameGraphicsData gameGraphicsData;

namespace ComputeCopyRenderPass
{

    RENDER_PASS_EXEC_FUNC(Execute)
    {

        // Transition main view render target from render optimal to shader read
        graphicsCommandStream->CmdTransitionLayout(gameGraphicsData.m_rtColorHandle, Graphics::ImageLayout::eRenderOptimal, Graphics::ImageLayout::eShaderRead, "Transition main view render target to shader read");

        // Transition compute copy target to UAV
        graphicsCommandStream->CmdTransitionLayout(gameGraphicsData.m_computeColorHandle, Graphics::ImageLayout::eUndefined, Graphics::ImageLayout::eGeneral, "Transition compute copy target to UAV dst");

        // Dispatch the CS
        const uint32 groupSize = 16;
        graphicsCommandStream->CmdDispatch((renderPass->renderWidth  + groupSize - 1) / groupSize, (renderPass->renderHeight + groupSize - 1) / groupSize, 1, Graphics::SHADER_ID_COMPUTE_GRAYSCALE, "Compute Copy");
    }

}

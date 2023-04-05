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
        Graphics::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
        for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        {
            descriptors[i] = Graphics::DefaultDescHandle_Invalid;
        }
        descriptors[0] = gameGraphicsData.m_computeCopyDescHandle;
        graphicsCommandStream->CmdDispatch(1, 1, 1, Graphics::SHADER_ID_COMPUTE_COPY, descriptors, "Compute Copy");
    }

}

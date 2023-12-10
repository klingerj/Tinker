#include "ComputeCopyRenderPass.h"
#include "Game/GraphicsTypes.h"

extern GameGraphicsData gameGraphicsData;

namespace ComputeCopyRenderPass
{

    RENDER_PASS_EXEC_FUNC(Execute)
    {
        graphicsCommandStream->CmdLayoutTransition(gameGraphicsData.m_rtColorHandle, Tk::Graphics::ImageLayout::eRenderOptimal, Tk::Graphics::ImageLayout::eGeneral, "Transition main view render target to UAV");
        graphicsCommandStream->CmdLayoutTransition(gameGraphicsData.m_computeColorHandle, Tk::Graphics::ImageLayout::eUndefined, Tk::Graphics::ImageLayout::eGeneral, "Transition compute copy target to UAV");

        // Dispatch the CS
        const uint32 groupSize = 16;
        Tk::Graphics::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
        for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        {
            descriptors[i] = Tk::Graphics::DefaultDescHandle_Invalid;
        }
        descriptors[0] = gameGraphicsData.m_computeCopyDescHandle;
        graphicsCommandStream->CmdDispatch(THREADGROUP_ROUND(renderPass->renderWidth, groupSize), THREADGROUP_ROUND(renderPass->renderHeight, groupSize), 1u, Tk::Graphics::SHADER_ID_COMPUTE_COPY, MAX_DESCRIPTOR_SETS_PER_SHADER, descriptors, "Compute Copy");
    }

}

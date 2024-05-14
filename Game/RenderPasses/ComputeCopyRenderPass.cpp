#include "ComputeCopyRenderPass.h"
#include "Game/GraphicsTypes.h"

#include "Game/BindlessSystem.h"

namespace ComputeCopyRenderPass
{

    RENDER_PASS_EXEC_FUNC(Execute)
    {
        graphicsCommandStream->CmdLayoutTransition(renderPass->colorRTs[0], Tk::Graphics::ImageLayout::eRenderOptimal, Tk::Graphics::ImageLayout::eGeneral, "Transition main view render target to UAV");
        graphicsCommandStream->CmdLayoutTransition(renderPass->colorRTs[1], Tk::Graphics::ImageLayout::eUndefined, Tk::Graphics::ImageLayout::eGeneral, "Transition compute copy target to UAV");

        // Dispatch the CS
        const uint32 groupSize = 16;
        Tk::Graphics::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
        for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        {
            descriptors[i] = Tk::Graphics::DefaultDescHandle_Invalid;
        }
        descriptors[0] = BindlessSystem::GetBindlessDescriptorFromID(BindlessSystem::BindlessArrayID::eConstants);
        descriptors[1] = BindlessSystem::GetBindlessDescriptorFromID(BindlessSystem::BindlessArrayID::eConstants);
        descriptors[2] = BindlessSystem::GetBindlessDescriptorFromID(BindlessSystem::BindlessArrayID::eTexturesRGBA8Sampled);
        descriptors[3] = BindlessSystem::GetBindlessDescriptorFromID(BindlessSystem::BindlessArrayID::eTexturesRGBA8RW);
        graphicsCommandStream->CmdDispatch(THREADGROUP_ROUND(renderPass->renderWidth, groupSize), THREADGROUP_ROUND(renderPass->renderHeight, groupSize), 1u, Tk::Graphics::SHADER_ID_COMPUTE_COPY, MAX_DESCRIPTOR_SETS_PER_SHADER, descriptors, "Compute Copy");

        graphicsCommandStream->CmdLayoutTransition(renderPass->colorRTs[1], Tk::Graphics::ImageLayout::eGeneral, Tk::Graphics::ImageLayout::eRenderOptimal, "Transition compute copy target to RT");
    }

}

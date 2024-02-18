#include "ForwardRenderPass.h"
#include "Game/GraphicsTypes.h"

#include "Game/BindlessSystem.h"

extern GameGraphicsData gameGraphicsData;
extern Scene MainScene;
extern View MainView;

namespace ForwardRenderPass
{

    RENDER_PASS_EXEC_FUNC(Execute)
    {
        graphicsCommandStream->CmdLayoutTransition(renderPass->colorRTs[0], Tk::Graphics::ImageLayout::eUndefined, Tk::Graphics::ImageLayout::eTransferDst, "Transition main view color to transfer_dst");
        graphicsCommandStream->CmdClear(renderPass->colorRTs[0], v4f(0.0f, 0.0f, 0.0f, 0.0f), "Clear main view color buffer");
        graphicsCommandStream->CmdLayoutTransition(renderPass->colorRTs[0], Tk::Graphics::ImageLayout::eTransferDst, Tk::Graphics::ImageLayout::eRenderOptimal, "Transition main view color to render_optimal");

        Tk::Graphics::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0] = BindlessSystem::GetBindlessConstantBufferDescriptor();
        descriptors[1] = gameGraphicsData.m_DescData_Instance;
        descriptors[3] = BindlessSystem::GetBindlessDescriptorFromID(BindlessSystem::BindlessArrayID::eTexturesSampled);

        StartRenderPass(renderPass, graphicsCommandStream);

        RecordRenderPassCommands(renderPass, &MainView, &MainScene, graphicsCommandStream, Tk::Graphics::SHADER_ID_BASIC_MainView, Tk::Graphics::BlendState::eAlphaBlend, Tk::Graphics::DepthState::eTestOnWriteOn_CCW, descriptors);

        // TODO: handle this more elegantly
        UpdateAnimatedPoly(&gameGraphicsData.m_animatedPolygon);
        DrawAnimatedPoly(&gameGraphicsData.m_animatedPolygon, Tk::Graphics::SHADER_ID_ANIMATEDPOLY_MainView, Tk::Graphics::BlendState::eAlphaBlend, Tk::Graphics::DepthState::eTestOnWriteOn_CCW, graphicsCommandStream);

        EndRenderPass(renderPass, graphicsCommandStream);
    }

}

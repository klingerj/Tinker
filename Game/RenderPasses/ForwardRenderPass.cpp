#include "ForwardRenderPass.h"

#include "Game/GraphicsTypes.h"

extern GameGraphicsData gameGraphicsData;
extern Scene MainScene;
extern View MainView;

namespace ForwardRenderPass
{

    RENDER_PASS_EXEC_FUNC(Execute)
    {
        Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

        // Transition of depth buffer from layout undefined to transfer_dst (required for clear command)
        command->CmdTransitionLayout(renderPass->colorRTs[0], Graphics::ImageLayout::eUndefined, Graphics::ImageLayout::eTransferDst, "Transition main view color to transfer_dst");
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Clear color buffer
        command->CmdClear(renderPass->colorRTs[0], v4f(0.0f, 0.0f, 0.0f, 0.0f), "Clear main view color buffer");
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Transition of depth buffer from transfer dst to depth_attachment_optimal
        command->CmdTransitionLayout(renderPass->colorRTs[0], Graphics::ImageLayout::eTransferDst, Graphics::ImageLayout::eRenderOptimal, "Transition main view color to render_optimal");
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Draw
        Graphics::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0] = gameGraphicsData.m_DescData_Global;
        descriptors[1] = gameGraphicsData.m_DescData_Instance;

        StartRenderPass(renderPass, graphicsCommandStream);

        RecordRenderPassCommands(renderPass, &MainView, &MainScene, graphicsCommandStream, Graphics::SHADER_ID_BASIC_MainView, Graphics::BlendState::eAlphaBlend, Graphics::DepthState::eTestOnWriteOn_CCW, descriptors);

        // TODO: handle this more elegantly
        UpdateAnimatedPoly(&gameGraphicsData.m_animatedPolygon);
        DrawAnimatedPoly(&gameGraphicsData.m_animatedPolygon, gameGraphicsData.m_DescData_Global, Graphics::SHADER_ID_ANIMATEDPOLY_MainView, Graphics::BlendState::eAlphaBlend, Graphics::DepthState::eTestOnWriteOn_CCW, graphicsCommandStream);

        EndRenderPass(renderPass, graphicsCommandStream);
    }

}

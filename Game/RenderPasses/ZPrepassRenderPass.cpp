#include "ZPrepassRenderPass.h"

extern GameGraphicsData gameGraphicsData;
extern Scene MainScene;
extern View MainView;

namespace ZPrepassRenderPass
{

    RENDER_PASS_EXEC_FUNC(Execute)
    {
        Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

        // Transition of depth buffer from layout undefined to transfer_dst (required for clear command)
        command->CmdTransitionLayout(renderPass->depthRT, Graphics::ImageLayout::eUndefined, Graphics::ImageLayout::eTransferDst, "Transition main view depth to transfer_dst");
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Clear depth buffer
        command->CmdClear(renderPass->depthRT, v4f(1.0f, 0.0f, 0.0f, 0.0f), "Clear main view depth buffer");
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Transition of depth buffer from transfer dst to depth_attachment_optimal
        command->CmdTransitionLayout(renderPass->depthRT, Graphics::ImageLayout::eTransferDst, Graphics::ImageLayout::eDepthOptimal, "Transition main view depth to depth_attachment_optimal");
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Draw
        Graphics::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0] = gameGraphicsData.m_DescData_Global;
        descriptors[1] = gameGraphicsData.m_DescData_Instance;

        StartRenderPass(renderPass, graphicsCommandStream);
        RecordRenderPassCommands(renderPass, &MainView, &MainScene, graphicsCommandStream, Graphics::SHADER_ID_BASIC_ZPrepass, Graphics::BlendState::eNoColorAttachment, Graphics::DepthState::eTestOnWriteOn_CCW, descriptors);
        EndRenderPass(renderPass, graphicsCommandStream);
    }

}

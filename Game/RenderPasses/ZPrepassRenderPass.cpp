#include "ZPrepassRenderPass.h"

extern GameGraphicsData gameGraphicsData;
extern Scene MainScene;
extern View MainView;

namespace ZPrepassRenderPass
{

    RENDER_PASS_EXEC_FUNC(Execute)
    {
        // Transition of depth buffer from layout undefined to transfer_dst
        graphicsCommandStream->CmdTransitionLayout(renderPass->depthRT, Graphics::ImageLayout::eUndefined, Graphics::ImageLayout::eTransferDst, "Transition main view depth to transfer_dst");

        // Clear depth buffer
        graphicsCommandStream->CmdClear(renderPass->depthRT, v4f(1.0f, 0.0f, 0.0f, 0.0f), "Clear main view depth buffer");

        // Transition of depth buffer from transfer dst to depth_attachment_optimal
        graphicsCommandStream->CmdTransitionLayout(renderPass->depthRT, Graphics::ImageLayout::eTransferDst, Graphics::ImageLayout::eDepthOptimal, "Transition main view depth to depth_attachment_optimal");

        // Draw
        Graphics::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0] = gameGraphicsData.m_DescData_Global;
        descriptors[1] = gameGraphicsData.m_DescData_Instance;

        StartRenderPass(renderPass, graphicsCommandStream);
        RecordRenderPassCommands(renderPass, &MainView, &MainScene, graphicsCommandStream, Graphics::SHADER_ID_BASIC_ZPrepass, Graphics::BlendState::eNoColorAttachment, Graphics::DepthState::eTestOnWriteOn_CCW, descriptors);
        EndRenderPass(renderPass, graphicsCommandStream);
    }

}

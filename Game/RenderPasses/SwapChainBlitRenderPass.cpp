#include "SwapChainBlitRenderPass.h"

#include "Game/GraphicsTypes.h"

extern GameGraphicsData gameGraphicsData;

namespace SwapChainBlitRenderPass
{

    RENDER_PASS_EXEC_FUNC(Execute)
    {

        {
            Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

            // Transition main view render target from render optimal to shader read
            command->CmdTransitionLayout(gameGraphicsData.m_rtColorHandle, Graphics::ImageLayout::eRenderOptimal, Graphics::ImageLayout::eShaderRead, "Transition main view render target to shader read for blit");
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // Transition of swap chain to render optimal
            command->CmdTransitionLayout(Graphics::IMAGE_HANDLE_SWAP_CHAIN, Graphics::ImageLayout::eUndefined, Graphics::ImageLayout::eRenderOptimal, "Transition swap chain to render_optimal");
            ++graphicsCommandStream->m_numCommands;
            ++command;
        }

        StartRenderPass(renderPass, graphicsCommandStream);

        {
            Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

            command->m_commandType = Graphics::GraphicsCommand::eDrawCall;
            command->debugLabel = "Draw default quad";
            command->m_numIndices = DEFAULT_QUAD_NUM_INDICES;
            command->m_numInstances = 1;
            command->m_vertOffset = 0;
            command->m_indexOffset = 0;
            command->m_indexBufferHandle = defaultQuad.m_indexBuffer.gpuBufferHandle;
            command->m_shader = Graphics::SHADER_ID_SWAP_CHAIN_BLIT;
            command->m_blendState = Graphics::BlendState::eReplace;
            command->m_depthState = Graphics::DepthState::eOff_NoCull;
            for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
            {
                command->m_descriptors[i] = Graphics::DefaultDescHandle_Invalid;
            }
            command->m_descriptors[0] = gameGraphicsData.m_swapChainBlitDescHandle;
            command->m_descriptors[1] = defaultQuad.m_descriptor;
            ++graphicsCommandStream->m_numCommands;
            ++command;
        }

        EndRenderPass(renderPass, graphicsCommandStream);

        // Transition swap chain to present
        {
            Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];
            command->CmdTransitionLayout(Graphics::IMAGE_HANDLE_SWAP_CHAIN, Graphics::ImageLayout::eRenderOptimal, Graphics::ImageLayout::ePresent, "Transition swap chain to present");
            ++graphicsCommandStream->m_numCommands;
            ++command;
        }
    }

}

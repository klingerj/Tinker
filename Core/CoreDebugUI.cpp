#include "CoreDebugUI.h"
#include "CoreDefines.h"
#include "Math/VectorTypes.h"
#include "Graphics/Common/GraphicsCommon.h"

#include "imgui.h"

static const uint32 MAX_VERTS = 1024 * 1024;
static const uint32 MAX_IDXS = MAX_VERTS * 3;

namespace Tk
{
namespace Core
{
namespace DebugUI
{

static Graphics::ResourceHandle indexBuffer = Graphics::DefaultResHandle_Invalid;
static Graphics::ResourceHandle positionBuffer = Graphics::DefaultResHandle_Invalid;
static Graphics::ResourceHandle uvBuffer = Graphics::DefaultResHandle_Invalid;
static Graphics::ResourceHandle colorBuffer = Graphics::DefaultResHandle_Invalid;
static Graphics::DescriptorHandle vbDesc = Graphics::DefaultDescHandle_Invalid;
static Graphics::DescriptorHandle texDesc = Graphics::DefaultDescHandle_Invalid;

void Init()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "Tinker Graphics";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    Graphics::ResourceDesc desc;
    desc.resourceType = Graphics::ResourceType::eBuffer1D;
    desc.bufferUsage = Graphics::BufferUsage::eTransientVertex;
    
    desc.dims = v3ui(MAX_VERTS * sizeof(v2f), 0, 0);
    positionBuffer = Graphics::CreateResource(desc);
    desc.dims = v3ui(MAX_VERTS * sizeof(v2f), 0, 0);
    uvBuffer = Graphics::CreateResource(desc);
    desc.dims = v3ui(MAX_VERTS * sizeof(uint32), 0, 0);
    colorBuffer = Graphics::CreateResource(desc);

    desc.bufferUsage = Graphics::BufferUsage::eTransientIndex;
    desc.dims = v3ui((MAX_VERTS - 2) * 3 * sizeof(uint32), 0, 0);
    indexBuffer = Graphics::CreateResource(desc);
    
    vbDesc = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_IMGUI_VBS);

    Graphics::DescriptorSetDataHandles descDataHandles = {};
    descDataHandles.handles[0] = positionBuffer;
    descDataHandles.handles[1] = uvBuffer;
    descDataHandles.handles[2] = colorBuffer;

    Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_IMGUI_VBS, vbDesc, &descDataHandles);

    // Font texture
    int width, height;
    unsigned char* pixels = NULL;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    //TODO: memcpy font texture, immediate submit to gpu buffer
    io.Fonts->SetTexID((ImTextureID)((uint64)texDesc.m_hDesc));
}

void Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "";

    Graphics::DestroyResource(indexBuffer);
    indexBuffer = Graphics::DefaultResHandle_Invalid;
    Graphics::DestroyResource(positionBuffer);
    positionBuffer = Graphics::DefaultResHandle_Invalid;
    Graphics::DestroyResource(uvBuffer);
    uvBuffer = Graphics::DefaultResHandle_Invalid;
    Graphics::DestroyResource(colorBuffer);
    colorBuffer = Graphics::DefaultResHandle_Invalid;

    Graphics::DestroyDescriptor(vbDesc);
    vbDesc = Graphics::DefaultDescHandle_Invalid;
    Graphics::DestroyDescriptor(texDesc);
    texDesc = Graphics::DefaultDescHandle_Invalid;
}

void Render(Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    // TODO: walk draw data and record cmds
    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData->Valid)
    {
        TINKER_ASSERT(0);
        // TODO: log
        return;
    }

    if (drawData->CmdListsCount)
    {
        TINKER_ASSERT(drawData->TotalIdxCount <= MAX_IDXS);
        TINKER_ASSERT(drawData->TotalVtxCount <= MAX_VERTS);

        // Transition of swap chain to render optimal - TODO this is temporary???
        command->m_commandType = Graphics::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition swap chain to render_optimal";
        command->m_imageHandle = Graphics::IMAGE_HANDLE_SWAP_CHAIN;
        command->m_startLayout = Graphics::ImageLayout::ePresent;
        command->m_endLayout = Graphics::ImageLayout::eRenderOptimal;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        command->m_commandType = Graphics::GraphicsCmd::eRenderPassBegin;
        command->debugLabel = "Imgui draw";
        command->m_numColorRTs = 1;
        command->m_colorRTs[0] = Graphics::IMAGE_HANDLE_SWAP_CHAIN; // TODO: need handle to whatever RT, or do we draw it onto the swap chain?
        command->m_depthRT = Graphics::DefaultResHandle_Invalid;
        command->m_renderWidth  = (uint32)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        command->m_renderHeight = (uint32)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        ++graphicsCommandStream->m_numCommands;
        ++command;

        uint32 vtxCtr = 0, idxCtr = 0;
        for (int32 uiCmdList = 0; uiCmdList < drawData->CmdListsCount; ++uiCmdList)
        {
            ImDrawList* currDrawList = drawData->CmdLists[uiCmdList];

            uint32* idxBufPtr = (uint32*)Graphics::MapResource(indexBuffer);
            v2f* posBufPtr    =    (v2f*)Graphics::MapResource(positionBuffer);
            v2f* uvBufPtr     =    (v2f*)Graphics::MapResource(uvBuffer);
            uint32* colorBufPtr  =    (uint32*)Graphics::MapResource(colorBuffer);

            uint32 numIdxs = currDrawList->IdxBuffer.size();
            memcpy(idxBufPtr + idxCtr, currDrawList->IdxBuffer.begin(), sizeof(ImDrawIdx) * numIdxs);
            idxCtr += numIdxs;

            uint32 numVtxs = currDrawList->VtxBuffer.size();
            for (uint32 uiVtx = 0; uiVtx < numVtxs; ++uiVtx)
            {
                const ImDrawVert& vert = currDrawList->VtxBuffer[uiVtx];
                posBufPtr[vtxCtr + uiVtx]   = v2f(vert.pos.x, vert.pos.y);
                uvBufPtr[vtxCtr + uiVtx]    = v2f(vert.uv.x, vert.uv.y);
                colorBufPtr[vtxCtr + uiVtx] = vert.col;
            }
            vtxCtr += numVtxs;

            // Record draw calls
            for (int32 uiCmd = 0; uiCmd < currDrawList->CmdBuffer.size(); ++uiCmd)
            {
                const ImDrawCmd& cmd = currDrawList->CmdBuffer[uiCmd];

                command->m_commandType = Graphics::GraphicsCmd::ePushConstant;
                command->debugLabel = "Imgui push constant";
                command->m_shaderForLayout = Graphics::SHADER_ID_IMGUI_DEBUGUI;
                {
                    uint8* data = command->m_pushConstantData;
                    memcpy(data, &cmd.ClipRect, sizeof(&cmd.ClipRect));
                }
                ++graphicsCommandStream->m_numCommands;
                ++command;

                command->m_commandType = Graphics::GraphicsCmd::eDrawCall;
                command->debugLabel = "Draw imgui element";
                command->m_numIndices = cmd.ElemCount;
                command->m_numInstances = 1;
                command->m_vertOffset = cmd.VtxOffset; // TODO: finish routing these thru to the backend, and also need to set them to 0 in all other places where we make draw calls rn, grep it
                command->m_indexOffset = cmd.IdxOffset;
                command->m_indexBufferHandle = indexBuffer;
                command->m_shader = Graphics::SHADER_ID_IMGUI_DEBUGUI;
                command->m_blendState = Graphics::BlendState::eAlphaBlend;
                command->m_depthState = Graphics::DepthState::eOff;
                for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
                {
                    command->m_descriptors[i] = Graphics::DefaultDescHandle_Invalid;
                }
                command->m_descriptors[0] = texDesc;
                command->m_descriptors[1] = vbDesc;
                ++graphicsCommandStream->m_numCommands;
                ++command;
            }
        }

        command->m_commandType = Graphics::GraphicsCmd::eRenderPassEnd;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Transition of swap chain to present - TODO this is temporary???
        command->m_commandType = Graphics::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition swap chain to present";
        command->m_imageHandle = Graphics::IMAGE_HANDLE_SWAP_CHAIN;
        command->m_startLayout = Graphics::ImageLayout::eRenderOptimal;
        command->m_endLayout = Graphics::ImageLayout::ePresent;
        ++graphicsCommandStream->m_numCommands;
        ++command;
    }
}

void UI_RenderPassStats()
{
    const char* names[3] = { "ZPrepass", "MainView", "PostGraph" };
    float timings[3] = { 0.012f, 0.53f, 1.7f };

    if (ImGui::Begin("Render Pass Timings"))
    {
        if (ImGui::BeginMenuBar())
        {
            for (uint32 i = 0; i < 3; ++i)
            {
                ImGui::Text("%s: %.2f\n", names[i], timings[i]);
            }
        }

        ImGui::End();
    }
}

}
}
}

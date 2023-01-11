#include "CoreDebugUI.h"
#include "CoreDefines.h"
#include "Math/VectorTypes.h"
#include "Graphics/Common/GraphicsCommon.h"

#ifdef VULKAN
#include "backends/imgui_impl_vulkan.h"
#else
// TODO: D3D12
#endif

#include "imgui.h"

static const uint32 MAX_VERTS = 1024 * 1024;
static const uint32 MAX_IDXS = (1024 * 1024 - 2) * 3;

static Tk::Core::Graphics::ResourceHandle indexBuffer;
static Tk::Core::Graphics::ResourceHandle positionBuffer;
static Tk::Core::Graphics::ResourceHandle uvBuffer;
static Tk::Core::Graphics::ResourceHandle colorBuffer;

namespace Tk
{
namespace Core
{
namespace DebugUI
{
    void Init()
    {
        ImGuiIO& io = ImGui::GetIO();
        io.BackendRendererName = "Tinker Graphics";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

        //TODO: create gfx resources
        Tk::Core::Graphics::ResourceDesc desc;
        desc.resourceType = Tk::Core::Graphics::ResourceType::eBuffer1D;
        desc.bufferUsage = Tk::Core::Graphics::BufferUsage::eTransientVertex;
        
        desc.dims = v3ui(MAX_VERTS * sizeof(v2f), 0, 0);
        positionBuffer = Tk::Core::Graphics::CreateResource(desc);
        desc.dims = v3ui(MAX_VERTS * sizeof(v2f), 0, 0);
        uvBuffer = Tk::Core::Graphics::CreateResource(desc);
        desc.dims = v3ui(MAX_VERTS * sizeof(v4f), 0, 0);
        colorBuffer = Tk::Core::Graphics::CreateResource(desc);

        desc.bufferUsage = Tk::Core::Graphics::BufferUsage::eTransientIndex;
        desc.dims = v3ui((MAX_VERTS - 2) * 3 * sizeof(uint32), 0, 0);
        indexBuffer = Tk::Core::Graphics::CreateResource(desc);
        
        //TODO: create descriptor for vbs and texture sets

        int width, height;
        unsigned char* pixels = NULL;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        //TODO: load font texture, immediate submit
        io.Fonts->SetTexID((ImTextureID) 3 /* get raw descriptor handle for image */);
    }

    void Shutdown()
    {
        ImGuiIO& io = ImGui::GetIO();
        io.BackendRendererName = "";
        // TODO: destroy gfx resources
    }

    void Render(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream)
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

            command->m_commandType = Tk::Core::Graphics::GraphicsCmd::eRenderPassBegin;
            command->debugLabel = "Imgui draw";
            command->m_numColorRTs = 1;
            command->m_colorRTs[0] = Tk::Core::Graphics::IMAGE_HANDLE_SWAP_CHAIN; // TODO: need handle to whatever RT, or do we draw it onto the swap chain?
            command->m_depthRT = Tk::Core::Graphics::DefaultResHandle_Invalid;
            command->m_renderWidth  = (uint32)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
            command->m_renderHeight = (uint32)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
            ++graphicsCommandStream->m_numCommands;
            ++command;

            uint32 vtxCtr = 0, idxCtr = 0;
            for (int32 uiCmdList = 0; uiCmdList < drawData->CmdListsCount; ++uiCmdList)
            {
                ImDrawList* currDrawList = drawData->CmdLists[uiCmdList];

                uint32* idxBufPtr = (uint32*)Tk::Core::Graphics::MapResource(indexBuffer);
                v2f* posBufPtr    =    (v2f*)Tk::Core::Graphics::MapResource(positionBuffer);
                v2f* uvBufPtr     =    (v2f*)Tk::Core::Graphics::MapResource(uvBuffer);
                v4f* colorBufPtr  =    (v4f*)Tk::Core::Graphics::MapResource(colorBuffer);

                uint32 numIdxs = currDrawList->IdxBuffer.size();
                memcpy(idxBufPtr + idxCtr, currDrawList->IdxBuffer.begin(), sizeof(ImDrawIdx) * numIdxs);
                idxCtr += numIdxs;

                uint32 numVtxs = currDrawList->VtxBuffer.size();
                for (uint32 uiVtx = 0; uiVtx < numVtxs; ++uiVtx)
                {
                    const ImDrawVert& vert = currDrawList->VtxBuffer[uiVtx];
                    posBufPtr[vtxCtr + uiVtx]   = v2f(vert.pos.x, vert.pos.y);
                    uvBufPtr[vtxCtr + uiVtx]    = v2f(vert.uv.x, vert.uv.y);

                    v4f colorUnpacked = {};
                    colorUnpacked.x = (vert.col & 0xF) / 255.f;
                    colorUnpacked.y = ((vert.col >> 8) & 0xF) / 255.f;
                    colorUnpacked.z = ((vert.col >> 16) & 0xF) / 255.f;
                    colorUnpacked.w = ((vert.col >> 24) & 0xF) / 255.f;
                    colorBufPtr[vtxCtr + uiVtx] = colorUnpacked;
                }
                vtxCtr += numVtxs;

                // Record draw calls
                for (int32 uiCmd = 0; uiCmd < currDrawList->CmdBuffer.size(); ++uiCmd)
                {
                    const ImDrawCmd& cmd = currDrawList->CmdBuffer[uiCmd];

                    command->m_commandType = Graphics::GraphicsCmd::eDrawCall;
                    command->debugLabel = "Draw imgui element";
                    command->m_numIndices = cmd.ElemCount;// numIdxs;
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
                    //command->m_descriptors[0] = /* font image handle */;
                    //command->m_descriptors[1] = /* descriptor for vbs */;
                    ++graphicsCommandStream->m_numCommands;
                    ++command;
                }
            }

            command->m_commandType = Graphics::GraphicsCmd::eRenderPassEnd;
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

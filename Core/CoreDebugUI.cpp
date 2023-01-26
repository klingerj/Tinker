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
static Graphics::ResourceHandle fontTexture = Graphics::DefaultResHandle_Invalid;
static Graphics::DescriptorHandle vbDesc = Graphics::DefaultDescHandle_Invalid;
static Graphics::DescriptorHandle texDesc = Graphics::DefaultDescHandle_Invalid;

void Init(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "Tinker Graphics";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    // Vertex buffers
    {
        Graphics::ResourceDesc desc;
        desc.resourceType = Graphics::ResourceType::eBuffer1D;
        desc.bufferUsage = Graphics::BufferUsage::eTransientVertex;

        desc.dims = v3ui(MAX_VERTS * sizeof(v2f), 0, 0);
        desc.debugLabel = "Imgui pos vtx buf";
        positionBuffer = Graphics::CreateResource(desc);
        desc.dims = v3ui(MAX_VERTS * sizeof(v2f), 0, 0);
        desc.debugLabel = "Imgui uv vtx buf";
        uvBuffer = Graphics::CreateResource(desc);
        desc.dims = v3ui(MAX_VERTS * sizeof(uint32), 0, 0);
        desc.debugLabel = "Imgui color vtx buf";
        colorBuffer = Graphics::CreateResource(desc);

        desc.bufferUsage = Graphics::BufferUsage::eTransientIndex;
        desc.dims = v3ui((MAX_VERTS - 2) * 3 * sizeof(uint32), 0, 0);
        desc.debugLabel = "Imgui idx buf";
        indexBuffer = Graphics::CreateResource(desc);

        vbDesc = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_IMGUI_VBS);

        Graphics::DescriptorSetDataHandles descDataHandles = {};
        descDataHandles.handles[0] = positionBuffer;
        descDataHandles.handles[1] = uvBuffer;
        descDataHandles.handles[2] = colorBuffer;

        Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_IMGUI_VBS, vbDesc, &descDataHandles);
    }

    // Font texture
    {
        int width, height;
        unsigned char* pixels = NULL;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        //TODO: memcpy font texture, immediate submit to gpu buffer
        io.Fonts->SetTexID((ImTextureID)((uint64)texDesc.m_hDesc));

        // Image
        Graphics::ResourceDesc desc;
        desc.resourceType = Graphics::ResourceType::eImage2D;
        desc.arrayEles = 1;
        desc.imageFormat = Graphics::ImageFormat::RGBA8_SRGB;
        desc.dims = v3ui(width, height, 1);
        desc.debugLabel = "Imgui font image";
        fontTexture = Graphics::CreateResource(desc);

        // Staging buffer
        Graphics::ResourceHandle imageStagingBufferHandle;
        uint32 textureSizeInBytes = desc.dims.x * desc.dims.y * 4; // 4 bytes per pixel since RGBA8
        desc.dims = v3ui(textureSizeInBytes, 0, 0);
        desc.resourceType = Graphics::ResourceType::eBuffer1D; // staging buffer is just a 1D buffer
        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        desc.debugLabel = "Imgui font staging buffer";
        imageStagingBufferHandle = Graphics::CreateResource(desc);
        
        // Copy to GPU
        void* stagingBufferMemPtr = Graphics::MapResource(imageStagingBufferHandle);
        memcpy(stagingBufferMemPtr, pixels, textureSizeInBytes);

        // Command recording and submission
        Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

        // Transition to transfer dst optimal layout
        command->m_commandType = Graphics::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition imgui font image layout to transfer dst optimal";
        command->m_imageHandle = fontTexture;
        command->m_startLayout = Graphics::ImageLayout::eUndefined;
        command->m_endLayout = Graphics::ImageLayout::eTransferDst;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Texture buffer copy
        command->m_commandType = Graphics::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update imgui font texture data";
        command->m_sizeInBytes = textureSizeInBytes;
        command->m_srcBufferHandle = imageStagingBufferHandle;
        command->m_dstBufferHandle = fontTexture;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Transition to shader read optimal layout
        command->m_commandType = Graphics::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition imgui font image layout to shader read optimal";
        command->m_imageHandle = fontTexture;
        command->m_startLayout = Graphics::ImageLayout::eTransferDst;
        command->m_endLayout = Graphics::ImageLayout::eShaderRead;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Perform the copy from staging buffer to device local buffer
        Graphics::SubmitCmdsImmediate(graphicsCommandStream);
        graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream

        Graphics::UnmapResource(imageStagingBufferHandle);
        Graphics::DestroyResource(imageStagingBufferHandle);

        // Descriptor
        texDesc = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_IMGUI_TEX);
        Graphics::DescriptorSetDataHandles descHandles = {};
        descHandles.InitInvalid();
        descHandles.handles[0] = fontTexture;
        Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_IMGUI_TEX, texDesc, &descHandles);
    }
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
    Graphics::DestroyResource(fontTexture);
    fontTexture = Graphics::DefaultResHandle_Invalid;

    Graphics::DestroyDescriptor(vbDesc);
    vbDesc = Graphics::DefaultDescHandle_Invalid;
    Graphics::DestroyDescriptor(texDesc);
    texDesc = Graphics::DefaultDescHandle_Invalid;
}

void Render(Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

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
        /*command->m_commandType = Graphics::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition swap chain to render_optimal";
        command->m_imageHandle = Graphics::IMAGE_HANDLE_SWAP_CHAIN;
        command->m_startLayout = Graphics::ImageLayout::ePresent;
        command->m_endLayout = Graphics::ImageLayout::eRenderOptimal;
        ++graphicsCommandStream->m_numCommands;
        ++command;*/

        const uint32 fbWidth = (uint32)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        const uint32 fbHeight = (uint32)(drawData->DisplaySize.y * drawData->FramebufferScale.y);

        command->m_commandType = Graphics::GraphicsCmd::eRenderPassBegin;
        command->debugLabel = "Begin Imgui render pass";
        command->m_numColorRTs = 1;
        command->m_colorRTs[0] = Graphics::IMAGE_HANDLE_SWAP_CHAIN;
        command->m_depthRT = Graphics::DefaultResHandle_Invalid;
        command->m_renderWidth  = fbWidth;
        command->m_renderHeight = fbHeight;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        const v2f scissorWindowMin = v2f(drawData->DisplayPos.x, drawData->DisplayPos.y);
        const v2f scissorScale = v2f(drawData->FramebufferScale.x, drawData->FramebufferScale.y);

        const v2f scale = v2f(2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y);
        const v2f translate = v2f(-1.0f - drawData->DisplayPos.x * scale.x, -1.0f - drawData->DisplayPos.y * scale.y);

        uint32* idxBufPtr = (uint32*)Graphics::MapResource(indexBuffer);
        v2f* posBufPtr = (v2f*)Graphics::MapResource(positionBuffer);
        v2f* uvBufPtr = (v2f*)Graphics::MapResource(uvBuffer);
        uint32* colorBufPtr = (uint32*)Graphics::MapResource(colorBuffer);

        uint32 vtxCtr = 0, idxCtr = 0;
        for (int32 uiCmdList = 0; uiCmdList < drawData->CmdListsCount; ++uiCmdList)
        {
            ImDrawList* currDrawList = drawData->CmdLists[uiCmdList];

            uint32 numIdxs = currDrawList->IdxBuffer.size();
            for (uint32 uiIdx = 0; uiIdx < numIdxs; ++uiIdx)
            {
                idxBufPtr[uiIdx + idxCtr] = (uint32)(currDrawList->IdxBuffer[uiIdx]);
            }

            uint32 numVtxs = currDrawList->VtxBuffer.size();
            for (uint32 uiVtx = 0; uiVtx < numVtxs; ++uiVtx)
            {
                const ImDrawVert& vert = currDrawList->VtxBuffer[uiVtx];
                posBufPtr[vtxCtr + uiVtx]   = v2f(vert.pos.x, vert.pos.y);
                uvBufPtr[vtxCtr + uiVtx]    = v2f(vert.uv.x, vert.uv.y);
                colorBufPtr[vtxCtr + uiVtx] = vert.col;
            }

            // Record draw calls
            for (int32 uiCmd = 0; uiCmd < currDrawList->CmdBuffer.size(); ++uiCmd)
            {
                const ImDrawCmd& cmd = currDrawList->CmdBuffer[uiCmd];

                command->m_commandType = Graphics::GraphicsCmd::ePushConstant;
                command->debugLabel = "Imgui push constant";
                command->m_shaderForLayout = Graphics::SHADER_ID_IMGUI_DEBUGUI;
                {
                    float* data = (float*)command->m_pushConstantData;
                    data[0] = scale.x;
                    data[1] = scale.y;
                    data[2] = translate.x;
                    data[3] = translate.y;
                }
                ++graphicsCommandStream->m_numCommands;
                ++command;

                // Calc tight scissor
                v2f scissorMin = {};
                v2f scissorMax = {};
                scissorMin = v2f((cmd.ClipRect.x - scissorWindowMin.x), (cmd.ClipRect.y - scissorWindowMin.y)) * scissorScale;
                scissorMax = v2f((cmd.ClipRect.z - scissorWindowMin.x), (cmd.ClipRect.w - scissorWindowMin.y)) * scissorScale;
                scissorMin.x = Max(scissorMin.x, 0.0f);
                scissorMin.y = Max(scissorMin.y, 0.0f);
                scissorMax.x = Min(scissorMax.x, (float)fbWidth);
                scissorMax.y = Min(scissorMax.y, (float)fbHeight);
                if (scissorMax.x <= scissorMin.x ||
                    scissorMax.y <= scissorMin.y)
                {
                    continue;
                }
                
                command->m_commandType = Graphics::GraphicsCmd::eSetScissor;
                command->debugLabel = "Set render pass scissor state";
                command->m_scissorOffsetX = (int32)scissorMin.x;
                command->m_scissorOffsetY = (int32)scissorMin.y;
                command->m_scissorWidth = uint32(scissorMax.x - scissorMin.x);
                command->m_scissorHeight = uint32(scissorMax.y - scissorMin.y);
                ++graphicsCommandStream->m_numCommands;
                ++command;

                command->m_commandType = Graphics::GraphicsCmd::eDrawCall;
                command->debugLabel = "Draw imgui element";
                command->m_numIndices = cmd.ElemCount;
                command->m_numInstances = 1;
                command->m_vertOffset = cmd.VtxOffset + vtxCtr;
                command->m_indexOffset = cmd.IdxOffset + idxCtr;
                command->m_indexBufferHandle = indexBuffer;
                command->m_shader = Graphics::SHADER_ID_IMGUI_DEBUGUI;
                command->m_blendState = Graphics::BlendState::eAlphaBlend;
                command->m_depthState = Graphics::DepthState::eOff_NoCull;
                for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
                {
                    command->m_descriptors[i] = Graphics::DefaultDescHandle_Invalid;
                }
                command->m_descriptors[0] = texDesc;
                command->m_descriptors[1] = vbDesc;
                ++graphicsCommandStream->m_numCommands;
                ++command;
            }

            idxCtr += numIdxs;
            vtxCtr += numVtxs;
        }

        Graphics::UnmapResource(indexBuffer);
        Graphics::UnmapResource(positionBuffer);
        Graphics::UnmapResource(uvBuffer);
        Graphics::UnmapResource(colorBuffer);

        command->m_commandType = Graphics::GraphicsCmd::eRenderPassEnd;
        command->debugLabel = "End Imgui render pass";
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
    ImGui::ShowDemoWindow();

    /*const char* names[3] = { "ZPrepass", "MainView", "PostGraph" };
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
    }*/
}

}
}
}

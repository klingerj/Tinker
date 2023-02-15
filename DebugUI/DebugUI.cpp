#include "DebugUI.h"

#include "CoreDefines.h"
#include "Math/VectorTypes.h"
#include "Platform/PlatformGameAPI.h"
#include "Graphics/Common/GraphicsCommon.h"

#include "imgui.h"

static const uint32 MAX_VERTS = 1024 * 1024;
static const uint32 MAX_IDXS = MAX_VERTS * 3;

namespace DebugUI
{

static Tk::Graphics::ResourceHandle indexBuffer = Tk::Graphics::DefaultResHandle_Invalid;
static Tk::Graphics::ResourceHandle positionBuffer = Tk::Graphics::DefaultResHandle_Invalid;
static Tk::Graphics::ResourceHandle uvBuffer = Tk::Graphics::DefaultResHandle_Invalid;
static Tk::Graphics::ResourceHandle colorBuffer = Tk::Graphics::DefaultResHandle_Invalid;
static Tk::Graphics::ResourceHandle fontTexture = Tk::Graphics::DefaultResHandle_Invalid;
static Tk::Graphics::DescriptorHandle vbDesc = Tk::Graphics::DefaultDescHandle_Invalid;
static Tk::Graphics::DescriptorHandle texDesc = Tk::Graphics::DefaultDescHandle_Invalid;

static bool g_enable = false;

void ToggleEnable()
{
    g_enable = !g_enable;
}

void* ImGuiMemWrapper_Malloc(size_t sz, void* user_data)
{
    (void)user_data;
    return malloc(sz);
}

void ImGuiMemWrapper_Free(void* ptr, void* user_data)
{
    (void)user_data;
    free(ptr);
}

void Init(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    // ImGui startup
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingWithShift = true;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // TODO: support this

    io.BackendRendererName = "Tinker Graphics";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    
    Tk::Platform::ImguiCreate(ImGui::GetCurrentContext(), ImGuiMemWrapper_Malloc, ImGuiMemWrapper_Free);

    // Vertex buffers
    {
        Tk::Graphics::ResourceDesc desc;
        desc.resourceType = Tk::Graphics::ResourceType::eBuffer1D;
        desc.bufferUsage = Tk::Graphics::BufferUsage::eTransientVertex;

        desc.dims = v3ui(MAX_VERTS * sizeof(v2f), 0, 0);
        desc.debugLabel = "Imgui pos vtx buf";
        positionBuffer = Tk::Graphics::CreateResource(desc);
        desc.dims = v3ui(MAX_VERTS * sizeof(v2f), 0, 0);
        desc.debugLabel = "Imgui uv vtx buf";
        uvBuffer = Tk::Graphics::CreateResource(desc);
        desc.dims = v3ui(MAX_VERTS * sizeof(uint32), 0, 0);
        desc.debugLabel = "Imgui color vtx buf";
        colorBuffer = Tk::Graphics::CreateResource(desc);

        desc.bufferUsage = Tk::Graphics::BufferUsage::eTransientIndex;
        desc.dims = v3ui((MAX_VERTS - 2) * 3 * sizeof(uint32), 0, 0);
        desc.debugLabel = "Imgui idx buf";
        indexBuffer = Tk::Graphics::CreateResource(desc);

        vbDesc = Tk::Graphics::CreateDescriptor(Tk::Graphics::DESCLAYOUT_ID_IMGUI_VBS);

        Tk::Graphics::DescriptorSetDataHandles descDataHandles = {};
        descDataHandles.handles[0] = positionBuffer;
        descDataHandles.handles[1] = uvBuffer;
        descDataHandles.handles[2] = colorBuffer;

        Tk::Graphics::WriteDescriptor(Tk::Graphics::DESCLAYOUT_ID_IMGUI_VBS, vbDesc, &descDataHandles);
    }

    // Font texture
    {
        int width, height;
        unsigned char* pixels = NULL;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        io.Fonts->SetTexID((ImTextureID)((uint64)texDesc.m_hDesc));

        // Image
        Tk::Graphics::ResourceDesc desc;
        desc.resourceType = Tk::Graphics::ResourceType::eImage2D;
        desc.arrayEles = 1;
        desc.imageFormat = Tk::Graphics::ImageFormat::RGBA8_SRGB;
        desc.dims = v3ui(width, height, 1);
        desc.debugLabel = "Imgui font image";
        fontTexture = Tk::Graphics::CreateResource(desc);

        // Staging buffer
        Tk::Graphics::ResourceHandle imageStagingBufferHandle;
        uint32 textureSizeInBytes = desc.dims.x * desc.dims.y * 4; // 4 bytes per pixel since RGBA8
        desc.dims = v3ui(textureSizeInBytes, 0, 0);
        desc.resourceType = Tk::Graphics::ResourceType::eBuffer1D; // staging buffer is just a 1D buffer
        desc.bufferUsage = Tk::Graphics::BufferUsage::eStaging;
        desc.debugLabel = "Imgui font staging buffer";
        imageStagingBufferHandle = Tk::Graphics::CreateResource(desc);
        
        // Copy to GPU
        void* stagingBufferMemPtr = Tk::Graphics::MapResource(imageStagingBufferHandle);
        memcpy(stagingBufferMemPtr, pixels, textureSizeInBytes);

        // Command recording and submission
        Tk::Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

        // Transition to transfer dst optimal layout
        command->m_commandType = Tk::Graphics::GraphicsCommand::eLayoutTransition;
        command->debugLabel = "Transition imgui font image layout to transfer dst optimal";
        command->m_imageHandle = fontTexture;
        command->m_startLayout = Tk::Graphics::ImageLayout::eUndefined;
        command->m_endLayout = Tk::Graphics::ImageLayout::eTransferDst;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Texture buffer copy
        command->m_commandType = Tk::Graphics::GraphicsCommand::eMemTransfer;
        command->debugLabel = "Update imgui font texture data";
        command->m_sizeInBytes = textureSizeInBytes;
        command->m_srcBufferHandle = imageStagingBufferHandle;
        command->m_dstBufferHandle = fontTexture;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Transition to shader read optimal layout
        command->m_commandType = Tk::Graphics::GraphicsCommand::eLayoutTransition;
        command->debugLabel = "Transition imgui font image layout to shader read optimal";
        command->m_imageHandle = fontTexture;
        command->m_startLayout = Tk::Graphics::ImageLayout::eTransferDst;
        command->m_endLayout = Tk::Graphics::ImageLayout::eShaderRead;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Perform the copy from staging buffer to device local buffer
        Tk::Graphics::SubmitCmdsImmediate(graphicsCommandStream);
        graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream

        Tk::Graphics::UnmapResource(imageStagingBufferHandle);
        Tk::Graphics::DestroyResource(imageStagingBufferHandle);

        // Descriptor
        texDesc = Tk::Graphics::CreateDescriptor(Tk::Graphics::DESCLAYOUT_ID_IMGUI_TEX);
        Tk::Graphics::DescriptorSetDataHandles descHandles = {};
        descHandles.InitInvalid();
        descHandles.handles[0] = fontTexture;
        Tk::Graphics::WriteDescriptor(Tk::Graphics::DESCLAYOUT_ID_IMGUI_TEX, texDesc, &descHandles);
    }
}

void Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "";

    Tk::Graphics::DestroyResource(indexBuffer);
    indexBuffer = Tk::Graphics::DefaultResHandle_Invalid;
    Tk::Graphics::DestroyResource(positionBuffer);
    positionBuffer = Tk::Graphics::DefaultResHandle_Invalid;
    Tk::Graphics::DestroyResource(uvBuffer);
    uvBuffer = Tk::Graphics::DefaultResHandle_Invalid;
    Tk::Graphics::DestroyResource(colorBuffer);
    colorBuffer = Tk::Graphics::DefaultResHandle_Invalid;
    Tk::Graphics::DestroyResource(fontTexture);
    fontTexture = Tk::Graphics::DefaultResHandle_Invalid;

    Tk::Graphics::DestroyDescriptor(vbDesc);
    vbDesc = Tk::Graphics::DefaultDescHandle_Invalid;
    Tk::Graphics::DestroyDescriptor(texDesc);
    texDesc = Tk::Graphics::DefaultDescHandle_Invalid;

    Tk::Platform::ImguiDestroy();
    ImGui::DestroyContext(ImGui::GetCurrentContext());
}

void NewFrame()
{
    Tk::Platform::ImguiNewFrame();
    ImGui::NewFrame();
}

void Render(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream, Tk::Graphics::ResourceHandle renderTarget)
{
    ImGui::Render();

    ImGuiIO& io = ImGui::GetIO();
    
    // TODO: support this
    /*if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }*/

    Tk::Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData->Valid)
    {
        // Well, this should never happen
        TINKER_ASSERT(0 && "Invalid imgui draw data");
        return;
    }

    if (drawData->CmdListsCount)
    {
        TINKER_ASSERT(drawData->TotalIdxCount <= MAX_IDXS);
        TINKER_ASSERT(drawData->TotalVtxCount <= MAX_VERTS);

        const uint32 fbWidth = (uint32)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        const uint32 fbHeight = (uint32)(drawData->DisplaySize.y * drawData->FramebufferScale.y);

        command->m_commandType = Tk::Graphics::GraphicsCommand::eRenderPassBegin;
        command->debugLabel = "Imgui render pass";
        command->m_numColorRTs = 1;
        command->m_colorRTs[0] = renderTarget;
        command->m_depthRT = Tk::Graphics::DefaultResHandle_Invalid;
        command->m_renderWidth  = fbWidth;
        command->m_renderHeight = fbHeight;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        const v2f scissorWindowMin = v2f(drawData->DisplayPos.x, drawData->DisplayPos.y);
        const v2f scissorScale = v2f(drawData->FramebufferScale.x, drawData->FramebufferScale.y);

        const v2f scale = v2f(2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y);
        const v2f translate = v2f(-1.0f - drawData->DisplayPos.x * scale.x, -1.0f - drawData->DisplayPos.y * scale.y);

        uint32* idxBufPtr = (uint32*)Tk::Graphics::MapResource(indexBuffer);
        v2f* posBufPtr = (v2f*)Tk::Graphics::MapResource(positionBuffer);
        v2f* uvBufPtr = (v2f*)Tk::Graphics::MapResource(uvBuffer);
        uint32* colorBufPtr = (uint32*)Tk::Graphics::MapResource(colorBuffer);

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

                command->m_commandType = Tk::Graphics::GraphicsCommand::ePushConstant;
                command->debugLabel = "Imgui push constant";
                command->m_shaderForLayout = Tk::Graphics::SHADER_ID_IMGUI_DEBUGUI;
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
                
                command->m_commandType = Tk::Graphics::GraphicsCommand::eSetScissor;
                command->debugLabel = "Set render pass scissor state";
                command->m_scissorOffsetX = (int32)scissorMin.x;
                command->m_scissorOffsetY = (int32)scissorMin.y;
                command->m_scissorWidth = uint32(scissorMax.x - scissorMin.x);
                command->m_scissorHeight = uint32(scissorMax.y - scissorMin.y);
                ++graphicsCommandStream->m_numCommands;
                ++command;

                command->m_commandType = Tk::Graphics::GraphicsCommand::eDrawCall;
                command->debugLabel = "Draw imgui element";
                command->m_numIndices = cmd.ElemCount;
                command->m_numInstances = 1;
                command->m_vertOffset = cmd.VtxOffset + vtxCtr;
                command->m_indexOffset = cmd.IdxOffset + idxCtr;
                command->m_indexBufferHandle = indexBuffer;
                command->m_shader = Tk::Graphics::SHADER_ID_IMGUI_DEBUGUI;
                command->m_blendState = Tk::Graphics::BlendState::eAlphaBlend;
                command->m_depthState = Tk::Graphics::DepthState::eOff_NoCull;
                for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
                {
                    command->m_descriptors[i] = Tk::Graphics::DefaultDescHandle_Invalid;
                }
                command->m_descriptors[0] = texDesc;
                command->m_descriptors[1] = vbDesc;
                ++graphicsCommandStream->m_numCommands;
                ++command;
            }

            idxCtr += numIdxs;
            vtxCtr += numVtxs;
        }

        Tk::Graphics::UnmapResource(indexBuffer);
        Tk::Graphics::UnmapResource(positionBuffer);
        Tk::Graphics::UnmapResource(uvBuffer);
        Tk::Graphics::UnmapResource(colorBuffer);

        command->m_commandType = Tk::Graphics::GraphicsCommand::eRenderPassEnd;
        command->debugLabel = "End Imgui render pass";
        ++graphicsCommandStream->m_numCommands;
        ++command;
    }
}

#include "Graphics/Common/GPUTimestamps.h"
void UI_RenderPassStats()
{
    using namespace Tk;
    using namespace Graphics;

    if (!g_enable)
    {
        return;
    }

    static bool selectedOverview = false;
    static bool selectedRPTimings = false;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Performance"))
        {
            if (ImGui::MenuItem("Overview", NULL, &selectedOverview)) {}
            if (ImGui::MenuItem("GPU Render Pass Timings", NULL, &selectedRPTimings)) {}

            ImGui::EndMenu();
        }

    }
    ImGui::EndMainMenuBar();

    if (selectedOverview)
    {
        if (ImGui::Begin("Performance Overview"))
        {
            GPUTimestamps::TimestampData timestampData = GPUTimestamps::GetTimestampData();
            ImGui::Text("%s: %.2f\n", "Total Frame Time", timestampData.totalFrameTimeInUS);
        }
        ImGui::End();
    }

    if (selectedRPTimings)
    {
        if (ImGui::Begin("GPU Render Pass Timings", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGuiTableFlags_ tableFlags = 
                (ImGuiTableFlags_)
                (ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingFixedSame | 
                ImGuiTableFlags_PadOuterX | 
                ImGuiTableFlags_Resizable | 
                ImGuiTableFlags_Sortable | 
                ImGuiTableFlags_SortTristate);

            if (ImGui::BeginTable("GPU Render Pass Timings Table", 5, tableFlags))
            {
                ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.4f, 0.3f, 0.0f, 0.2f));
                ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.2f));
                ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.4f, 0.3f, 0.0f, 0.2f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.8f, 0.5f));

                // Column headers
                ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                ImGui::TableNextColumn();
                ImGui::Text("%s", "Pass name");
                ImGui::TableNextColumn();
                ImGui::Text("%s", "Inst");
                ImGui::TableNextColumn();
                ImGui::Text("%s", "Avg");
                ImGui::TableNextColumn();
                ImGui::Text("%s", "Std dev");
                ImGui::TableNextColumn();
                ImGui::Text("%s", "Max");

                // Timestamp data rows
                GPUTimestamps::TimestampData timestampData = GPUTimestamps::GetTimestampData();
                for (uint32 i = 0; i < timestampData.numTimestamps; ++i)
                {
                    const GPUTimestamps::Timestamp& currTimestamp = timestampData.timestamps[i];

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", currTimestamp.name);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", currTimestamp.timeInst);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", 0.0f /* avg time */);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", 0.0f /* std dev */);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", 0.0f /* max time */);
                }

                ImGui::EndTable();
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
            }
        }
        ImGui::End();
    }
}

}

#include "DebugUI.h"
#include "CoreDefines.h"
#include "DataStructures/Vector.h"
#include "Game/GraphicsTypes.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "Math/VectorTypes.h"
#include "Platform/PlatformGameAPI.h"
#include "ThirdParty/imgui-docking/imgui.h"

static const uint32 MAX_VERTS = 1024 * 1024;
static const uint32 MAX_IDXS = MAX_VERTS * 3;

namespace DebugUI
{
  static Tk::Graphics::ResourceHandle indexBuffer =
    Tk::Graphics::DefaultResHandle_Invalid;
  static Tk::Graphics::ResourceHandle positionBuffer =
    Tk::Graphics::DefaultResHandle_Invalid;
  static Tk::Graphics::ResourceHandle uvBuffer = Tk::Graphics::DefaultResHandle_Invalid;
  static Tk::Graphics::ResourceHandle colorBuffer =
    Tk::Graphics::DefaultResHandle_Invalid;
  static Tk::Graphics::ResourceHandle fontTexture =
    Tk::Graphics::DefaultResHandle_Invalid;
  static Tk::Graphics::DescriptorHandle vbDesc = Tk::Graphics::DefaultDescHandle_Invalid;
  static Tk::Graphics::DescriptorHandle texDesc = Tk::Graphics::DefaultDescHandle_Invalid;
  static Tk::Graphics::CommandBuffer cmdBuffer[64] = {}; // TODO: dynamically allocate

  static Tk::Graphics::MemoryMappedBufferPtr idxBufPtr = {};
  static Tk::Graphics::MemoryMappedBufferPtr posBufPtr = {};
  static Tk::Graphics::MemoryMappedBufferPtr uvBufPtr = {};
  static Tk::Graphics::MemoryMappedBufferPtr colorBufPtr = {};
  static uint32 accumVtxOffset = 0;
  static uint32 accumIdxOffset = 0;

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

  static Tk::Platform::WindowHandles GetWindowHandlesFromImguiViewport(
    ImGuiViewport* viewport)
  {
    Tk::Platform::WindowHandles windowHandles = {};
    windowHandles.hInstance = Tk::Platform::GetPlatformWindowHandles()->hInstance;
    windowHandles.hWindow = (uint64)viewport->PlatformHandleRaw;
    return windowHandles;
  }

  static v2ui GetFramebufferDimsFromImguiDrawData(ImDrawData* drawData)
  {
    uint32 fbWidth = (uint32)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    uint32 fbHeight = (uint32)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
    return v2ui(fbWidth, fbHeight);
  }

  static void ImguiCreateSwapChainForViewport(ImGuiViewport* viewport)
  {
    Tk::Platform::WindowHandles windowHandles =
      GetWindowHandlesFromImguiViewport(viewport);
    Tk::Graphics::CreateSwapChain(&windowHandles, (uint32)viewport->Size.x,
                                  (uint32)viewport->Size.y);
  }

  static void ImguiDestroySwapChainForViewport(ImGuiViewport* viewport)
  {
    // Only call for viewports that aren't the main viewport
    if (viewport->ParentViewportId != 0)
    {
      Tk::Platform::WindowHandles windowHandles =
        GetWindowHandlesFromImguiViewport(viewport);
      Tk::Graphics::DestroySwapChain(&windowHandles);
    }
  }

  static void ImguiResizeSwapChainForViewport(ImGuiViewport* viewport, ImVec2 size)
  {
    Tk::Platform::WindowHandles windowHandles =
      GetWindowHandlesFromImguiViewport(viewport);
    Tk::Graphics::DestroySwapChain(&windowHandles);
    Tk::Graphics::CreateSwapChain(&windowHandles, (uint32)viewport->Size.x,
                                  (uint32)viewport->Size.y);
  }

  void Init(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream)
  {
    // ImGui startup
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingWithShift = true;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    io.BackendRendererName = "Tinker Graphics";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    Tk::Platform::ImguiCreate(ImGui::GetCurrentContext(), ImGuiMemWrapper_Malloc,
                              ImGuiMemWrapper_Free);

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      // Manually hook up renderer interface for imgui multi viewport
      platform_io.Renderer_CreateWindow = ImguiCreateSwapChainForViewport;
      platform_io.Renderer_DestroyWindow = ImguiDestroySwapChainForViewport;
      platform_io.Renderer_SetWindowSize = ImguiResizeSwapChainForViewport;
    }

    // Vertex buffers
    {
      Tk::Graphics::ResourceDesc desc;
      desc.resourceType = Tk::Graphics::ResourceType::eBuffer1D;
      desc.bufferUsage = Tk::Graphics::BufferUsage::eTransient;

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
      desc.dims = v3ui(MAX_VERTS * 3 * sizeof(uint32), 0, 0);
      desc.debugLabel = "Imgui idx buf";
      indexBuffer = Tk::Graphics::CreateResource(desc);

      vbDesc = Tk::Graphics::CreateDescriptor(Tk::Graphics::DESCLAYOUT_ID_IMGUI_VBS);

      Tk::Graphics::DescriptorSetDataHandles descDataHandles = {};
      descDataHandles.handles[0] = positionBuffer;
      descDataHandles.handles[1] = uvBuffer;
      descDataHandles.handles[2] = colorBuffer;
      Tk::Graphics::WriteDescriptorSimple(vbDesc, &descDataHandles);
    }

    for (uint32 i = 0; i < 64; ++i)
    {
      cmdBuffer[i] = Tk::Graphics::CreateCommandBuffer();
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
      desc.imageUsageFlags = Tk::Graphics::ImageUsageFlags::Sampled
                             | Tk::Graphics::ImageUsageFlags::TransferDst;
      desc.dims = v3ui(width, height, 1);
      desc.debugLabel = "Imgui font image";
      fontTexture = Tk::Graphics::CreateResource(desc);

      // Staging buffer
      Tk::Graphics::ResourceHandle imageStagingBufferHandle;
      uint32 textureSizeInBytes =
        desc.dims.x * desc.dims.y * 4; // 4 bytes per pixel since RGBA8
      desc.dims = v3ui(textureSizeInBytes, 0, 0);
      desc.resourceType =
        Tk::Graphics::ResourceType::eBuffer1D; // staging buffer is just a 1D buffer
      desc.bufferUsage = Tk::Graphics::BufferUsage::eStaging;
      desc.debugLabel = "Imgui font staging buffer";
      imageStagingBufferHandle = Tk::Graphics::CreateResource(desc);

      // Copy to GPU
      Tk::Graphics::MemoryMappedBufferPtr stagingBufferMemPtr =
        Tk::Graphics::MapResource(imageStagingBufferHandle);
      stagingBufferMemPtr.MemcpyInto(pixels, textureSizeInBytes);

      // Command recording
      graphicsCommandStream->CmdCommandBufferBegin(cmdBuffer[0],
                                                   "Begin debug ui cmd buffer");
      graphicsCommandStream->CmdLayoutTransition(
        fontTexture, Tk::Graphics::ImageLayout::eUndefined,
        Tk::Graphics::ImageLayout::eTransferDst,
        "Transition imgui font image layout to transfer dst optimal");
      graphicsCommandStream->CmdCopy(imageStagingBufferHandle, fontTexture,
                                     textureSizeInBytes,
                                     "Update imgui font texture data");
      graphicsCommandStream->CmdLayoutTransition(
        fontTexture, Tk::Graphics::ImageLayout::eTransferDst,
        Tk::Graphics::ImageLayout::eShaderRead,
        "Transition imgui font image layout to shader read optimal");
      // TODO: make one time submit function create the command buffer for me rather than
      // passing it here?
      graphicsCommandStream->CmdCommandBufferEnd(cmdBuffer[0]);
      Tk::Graphics::SubmitCmdsImmediate(graphicsCommandStream, cmdBuffer[0]);
      graphicsCommandStream->Clear();

      Tk::Graphics::UnmapResource(imageStagingBufferHandle);
      Tk::Graphics::DestroyResource(imageStagingBufferHandle);

      // Descriptor
      texDesc = Tk::Graphics::CreateDescriptor(Tk::Graphics::DESCLAYOUT_ID_IMGUI_TEX);
      Tk::Graphics::DescriptorSetDataHandles descHandles = {};
      descHandles.InitInvalid();
      descHandles.handles[0] = fontTexture;
      Tk::Graphics::WriteDescriptorSimple(texDesc, &descHandles);
    }
  }

  void Shutdown()
  {
    Tk::Graphics::UnmapResource(indexBuffer);
    Tk::Graphics::UnmapResource(positionBuffer);
    Tk::Graphics::UnmapResource(uvBuffer);
    Tk::Graphics::UnmapResource(colorBuffer);

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
  }

  void NewFrame()
  {
    Tk::Platform::ImguiNewFrame();
    ImGui::NewFrame();
  }

  static void RenderSingleImguiViewport(
    ImDrawData* drawData, Tk::Graphics::GraphicsCommandStream* graphicsCommandStream,
    Tk::Graphics::ResourceHandle renderTarget, uint32 shaderID)
  {
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

      const v2ui fbDims = GetFramebufferDimsFromImguiDrawData(drawData);
      const uint32 fbWidth = fbDims.x;
      const uint32 fbHeight = fbDims.y;

      Tk::Graphics::ResourceHandle colorRTs[] = { renderTarget };
      graphicsCommandStream->CmdRenderPassBegin(fbWidth, fbHeight, 1u, colorRTs,
                                                Tk::Graphics::DefaultResHandle_Invalid,
                                                "Imgui render pass");

      const v2f scissorWindowMin =
        v2f(drawData->DisplayPos.x - drawData->OwnerViewport->Pos.x,
            drawData->DisplayPos.y - drawData->OwnerViewport->Pos.y);
      const v2f scissorScale =
        v2f(drawData->FramebufferScale.x, drawData->FramebufferScale.y);

      const v2f scale =
        v2f(1.0f / drawData->DisplaySize.x, 1.0f / drawData->DisplaySize.y);
      const v2f translate =
        v2f(-drawData->DisplayPos.x * scale.x, -drawData->DisplayPos.y * scale.y);

      for (int32 uiCmdList = 0; uiCmdList < drawData->CmdListsCount; ++uiCmdList)
      {
        ImDrawList* currDrawList = drawData->CmdLists[uiCmdList];

        TINKER_ASSERT(accumIdxOffset == idxBufPtr.m_offset / sizeof(uint32));
        TINKER_ASSERT(accumVtxOffset == posBufPtr.m_offset / sizeof(v2f));

        uint32 numIdxs = currDrawList->IdxBuffer.size();
        idxBufPtr.MemcpyInto(currDrawList->IdxBuffer.Data, sizeof(uint32) * numIdxs);

        uint32 numVtxs = currDrawList->VtxBuffer.size();
        for (uint32 uiVtx = 0; uiVtx < numVtxs; ++uiVtx)
        {
          const ImDrawVert& vert = currDrawList->VtxBuffer[uiVtx];
          posBufPtr.MemcpyInto(&vert.pos, sizeof(v2f));
          uvBufPtr.MemcpyInto(&vert.uv, sizeof(v2f));
          colorBufPtr.MemcpyInto(&vert.col, sizeof(uint32));
        }

        // Record draw calls

        Tk::Graphics::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
        for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        {
          descriptors[i] = Tk::Graphics::DefaultDescHandle_Invalid;
        }
        descriptors[0] = texDesc;
        descriptors[1] = vbDesc;

        for (int32 uiCmd = 0; uiCmd < currDrawList->CmdBuffer.size(); ++uiCmd)
        {
          const ImDrawCmd& cmd = currDrawList->CmdBuffer[uiCmd];

          float data[4];
          data[0] = scale.x;
          data[1] = scale.y;
          data[2] = translate.x;
          data[3] = translate.y;
          graphicsCommandStream->CmdPushConstant(
            Tk::Graphics::SHADER_ID_IMGUI_DEBUGUI, &data[0],
            ARRAYCOUNT(data) * sizeof(float), "Imgui push constant");

          // Calc tight scissor
          v2f scissorMin = {};
          v2f scissorMax = {};
          scissorMin = v2f((cmd.ClipRect.x - drawData->DisplayPos.x),
                           (cmd.ClipRect.y - drawData->DisplayPos.y))
                       * scissorScale;
          scissorMax = v2f((cmd.ClipRect.z - drawData->DisplayPos.x),
                           (cmd.ClipRect.w - drawData->DisplayPos.y))
                       * scissorScale;
          scissorMin.x = Max(scissorMin.x, 0.0f);
          scissorMin.y = Max(scissorMin.y, 0.0f);
          scissorMax.x = Min(scissorMax.x, (float)fbWidth);
          scissorMax.y = Min(scissorMax.y, (float)fbHeight);
          if (scissorMax.x <= scissorMin.x || scissorMax.y <= scissorMin.y)
          {
            continue;
          }

          graphicsCommandStream->CmdSetViewport(
            0.0f, 0.0f, (float)fbWidth, (float)fbHeight, DEPTH_MIN, DEPTH_MAX,
            "Set debug ui render pass viewport state");
          graphicsCommandStream->CmdSetScissor((int32)scissorMin.x, (int32)scissorMin.y,
                                               uint32(scissorMax.x - scissorMin.x),
                                               uint32(scissorMax.y - scissorMin.y),
                                               "Set debug ui render pass scissor state");
          graphicsCommandStream->CmdDraw(
            cmd.ElemCount, 1u, cmd.VtxOffset + accumVtxOffset,
            cmd.IdxOffset + accumIdxOffset, shaderID,
            Tk::Graphics::BlendState::eAlphaBlend, Tk::Graphics::DepthState::eOff_NoCull,
            indexBuffer, MAX_DESCRIPTOR_SETS_PER_SHADER, descriptors, "Imgui draw call");
        }

        accumIdxOffset += numIdxs;
        accumVtxOffset += numVtxs;
        TINKER_ASSERT(accumIdxOffset == idxBufPtr.m_offset / sizeof(uint32));
        TINKER_ASSERT(accumVtxOffset == posBufPtr.m_offset / sizeof(v2f));
      }

      graphicsCommandStream->CmdRenderPassEnd("End Imgui render pass");
    }
  }

  void Render(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream,
              Tk::Graphics::ResourceHandle renderTarget)
  {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Render();

    idxBufPtr = Tk::Graphics::MapResource(indexBuffer);
    posBufPtr = Tk::Graphics::MapResource(positionBuffer);
    uvBufPtr = Tk::Graphics::MapResource(uvBuffer);
    colorBufPtr = Tk::Graphics::MapResource(colorBuffer);
    accumVtxOffset = 0;
    accumIdxOffset = 0;

    ImDrawData* drawData = ImGui::GetDrawData();
    RenderSingleImguiViewport(drawData, graphicsCommandStream, renderTarget,
                              Tk::Graphics::SHADER_ID_IMGUI_DEBUGUI);
  }

  void RenderAndSubmitMultiViewports(
    Tk::Graphics::GraphicsCommandStream* graphicsCommandStream)
  {
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      ImGui::UpdatePlatformWindows();

      ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
      for (int iViewport = 1; iViewport < platform_io.Viewports.Size; iViewport++)
      {
        Tk::Platform::WindowHandles windowHandles =
          GetWindowHandlesFromImguiViewport(platform_io.Viewports[iViewport]);
        bool shouldRenderFrame = Tk::Graphics::AcquireFrame(&windowHandles);
        if (!shouldRenderFrame)
        {
          // TODO: skip this viewport or something
        }
      }

      for (int iViewport = 1; iViewport < platform_io.Viewports.Size; iViewport++)
      {
        if ((platform_io.Viewports[iViewport]->Flags & ImGuiViewportFlags_Minimized) == 0)
        {
          Tk::Platform::WindowHandles windowHandles =
            GetWindowHandlesFromImguiViewport(platform_io.Viewports[iViewport]);
          ImDrawData* drawData = platform_io.Viewports[iViewport]->DrawData;
          const v2ui fbDims = GetFramebufferDimsFromImguiDrawData(drawData);
          Tk::Graphics::ResourceHandle swapChainImage =
            Tk::Graphics::GetCurrentSwapChainImage(&windowHandles);

          graphicsCommandStream->CmdCommandBufferBegin(cmdBuffer[iViewport],
                                                       "Imgui begin multi viewport");

          // Clear swap chain
          graphicsCommandStream->CmdLayoutTransition(
            swapChainImage, Tk::Graphics::ImageLayout::eUndefined,
            Tk::Graphics::ImageLayout::eRenderOptimal,
            "Transition swap chain to render optimal");

          graphicsCommandStream->CmdRenderPassBegin(
            fbDims.x, fbDims.y, 1, &swapChainImage,
            Tk::Graphics::DefaultResHandle_Invalid,
            "Imgui begin swap chain blit render pass");
          graphicsCommandStream->CmdSetViewport(
            0.0f, 0.0f, (float)fbDims.x, (float)fbDims.y, DEPTH_MIN, DEPTH_MAX,
            "Set imgui swap chain blit viewport state");
          graphicsCommandStream->CmdSetScissor(0, 0, fbDims.x, fbDims.y,
                                               "Set imgui swap chain blit scissor state");

          Tk::Graphics::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
          for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
          {
            descriptors[i] = Tk::Graphics::DefaultDescHandle_Invalid;
          }
          descriptors[0] = defaultQuad.m_descriptor;

          graphicsCommandStream->CmdDraw(
            DEFAULT_QUAD_NUM_INDICES, 1, 0, 0, Tk::Graphics::SHADER_ID_QUAD_BLIT_CLEAR,
            Tk::Graphics::BlendState::eReplace, Tk::Graphics::DepthState::eOff_NoCull,
            defaultQuad.m_indexBuffer.gpuBufferHandle, MAX_DESCRIPTOR_SETS_PER_SHADER,
            descriptors, "Draw asset");
          graphicsCommandStream->CmdRenderPassEnd(
            "Imgui end swap chain blit render pass");

          // Draw all imgui elements into swap chain
          RenderSingleImguiViewport(drawData, graphicsCommandStream, swapChainImage,
                                    Tk::Graphics::SHADER_ID_IMGUI_DEBUGUI_RGBA8);

          graphicsCommandStream->CmdLayoutTransition(
            swapChainImage, Tk::Graphics::ImageLayout::eRenderOptimal,
            Tk::Graphics::ImageLayout::ePresent, "Transition swap chain to present");
          graphicsCommandStream->CmdCommandBufferEnd(cmdBuffer[iViewport]);

          Tk::Graphics::ProcessGraphicsCommandStream(graphicsCommandStream);
          Tk::Graphics::SubmitFrameToGPU(&windowHandles, cmdBuffer[iViewport]);
          Tk::Graphics::PresentToSwapChain(&windowHandles);
          graphicsCommandStream->Clear();
        }
      }
    }
  }
} //namespace DebugUI

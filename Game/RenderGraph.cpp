#include "RenderGraph.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "GraphicsTypes.h"
#include "RenderPasses/ComputeCopyRenderPass.h"
#include "RenderPasses/DebugUIRenderPass.h"
#include "RenderPasses/ForwardRenderPass.h"
#include "RenderPasses/RenderPass.h"
#include "RenderPasses/SwapChainCopyRenderPass.h"
#include "RenderPasses/ToneMappingRenderPass.h"
#include "RenderPasses/ZPrepassRenderPass.h"

using namespace Tk;
using namespace Graphics;

namespace RenderGraph
{
  enum
  {
    eRenderPass_ZPrePass = 0,
    eRenderPass_MainView,
    eRenderPass_ToneMapping,
    eRenderPass_ComputeCopy,
    eRenderPass_DebugUI,
    eRenderPass_SwapChainCopy,
    eRenderPass_Max,
  };

  // All gpu resources needed to render frame
  ResourceHandle m_rtColorHandle;
  ResourceHandle m_rtDepthHandle;
  ResourceHandle m_rtColorToneMappedHandle;
  ResourceHandle m_computeColorHandle;
  DescriptorHandle m_toneMappingDescHandle;
  DescriptorHandle m_swapChainCopyDescHandle;

  typedef struct hardcoded_render_graph_t
  {
    GameRenderPass renderPassList[eRenderPass_Max];
  } HardcodedRenderGraph;

  HardcodedRenderGraph g_hardcodedRenderGraph = {};

  static void WriteToneMappingResources()
  {
    Graphics::DescriptorSetDataHandles toneMapHandles = {};
    toneMapHandles.InitInvalid();
    toneMapHandles.handles[0] = m_rtColorHandle;
    Graphics::WriteDescriptorSimple(m_toneMappingDescHandle, &toneMapHandles);

    Graphics::DescriptorSetDataHandles vbHandles = {};
    vbHandles.InitInvalid();
    vbHandles.handles[0] = defaultQuad.m_positionBuffer.gpuBufferHandle;
    vbHandles.handles[1] = defaultQuad.m_uvBuffer.gpuBufferHandle;
    vbHandles.handles[2] = defaultQuad.m_normalBuffer.gpuBufferHandle;
    Graphics::WriteDescriptorSimple(defaultQuad.m_descriptor, &vbHandles);
  }

  static void WriteSwapChainCopyResources()
  {
    Graphics::DescriptorSetDataHandles swapChainCopyHandles = {};
    swapChainCopyHandles.InitInvalid();
    swapChainCopyHandles.handles[0] = m_computeColorHandle;
    Graphics::WriteDescriptorSimple(m_swapChainCopyDescHandle, &swapChainCopyHandles);

    Graphics::DescriptorSetDataHandles vbHandles = {};
    vbHandles.InitInvalid();
    vbHandles.handles[0] = defaultQuad.m_positionBuffer.gpuBufferHandle;
    vbHandles.handles[1] = defaultQuad.m_uvBuffer.gpuBufferHandle;
    vbHandles.handles[2] = defaultQuad.m_normalBuffer.gpuBufferHandle;
    Graphics::WriteDescriptorSimple(defaultQuad.m_descriptor, &vbHandles);
  }

  static void Create(HardcodedRenderGraph* hardcodedRenderGraph,
                     const FrameRenderParams& frameRenderParams)
  {
    // Graphics resources
    Graphics::ResourceDesc desc;
    desc.resourceType = Graphics::ResourceType::eImage2D;
    desc.arrayEles = 1;
    desc.dims =
      v3ui(frameRenderParams.swapChainWidth, frameRenderParams.swapChainHeight, 1);
    desc.imageFormat = Graphics::ImageFormat::RGBA16_Float;
    desc.imageUsageFlags = Graphics::ImageUsageFlags::RenderTarget
                           | Graphics::ImageUsageFlags::Sampled
                           | Graphics::ImageUsageFlags::TransferDst
                           | Graphics::ImageUsageFlags::UAV;
    desc.debugLabel = "MainViewColor";
    m_rtColorHandle = Graphics::CreateResource(desc);

    desc.debugLabel = "MainViewColorTonemapped";
    m_rtColorToneMappedHandle = Graphics::CreateResource(desc);

    desc.imageFormat = Graphics::ImageFormat::Depth_32F;
    desc.imageUsageFlags =
      Graphics::ImageUsageFlags::DepthStencil | Graphics::ImageUsageFlags::TransferDst;
    desc.debugLabel = "MainViewDepth";
    m_rtDepthHandle = Graphics::CreateResource(desc);

    desc.imageFormat = Graphics::ImageFormat::RGBA16_Float;
    desc.imageUsageFlags = Graphics::ImageUsageFlags::RenderTarget
                           | Graphics::ImageUsageFlags::UAV
                           | Graphics::ImageUsageFlags::Sampled;
    desc.debugLabel = "MainViewColor_ComputeCopy";
    m_computeColorHandle = Graphics::CreateResource(desc);

    // Additional descriptors
    // TODO: these can probably be moved into the bindless system
    // or at least somewhere else.

    // Tone mapping
    m_toneMappingDescHandle =
      Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_QUAD_BLIT_TEX);
    WriteToneMappingResources();

    // Swap chain copy
    m_swapChainCopyDescHandle =
      Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_QUAD_BLIT_TEX);
    WriteSwapChainCopyResources();

    // Render graph nodes/passes
    hardcodedRenderGraph->renderPassList[eRenderPass_ZPrePass].Init();
    hardcodedRenderGraph->renderPassList[eRenderPass_ZPrePass].numColorRTs = 0;
    hardcodedRenderGraph->renderPassList[eRenderPass_ZPrePass].depthRT = m_rtDepthHandle;
    hardcodedRenderGraph->renderPassList[eRenderPass_ZPrePass].debugLabel = "Z Prepass";
    hardcodedRenderGraph->renderPassList[eRenderPass_ZPrePass].ExecuteFn =
      ZPrepassRenderPass::Execute;

    hardcodedRenderGraph->renderPassList[eRenderPass_MainView].Init();
    hardcodedRenderGraph->renderPassList[eRenderPass_MainView].numColorRTs = 1;
    hardcodedRenderGraph->renderPassList[eRenderPass_MainView].colorRTs[0] =
      m_rtColorHandle;
    hardcodedRenderGraph->renderPassList[eRenderPass_MainView].depthRT = m_rtDepthHandle;
    hardcodedRenderGraph->renderPassList[eRenderPass_MainView].debugLabel =
      "Main Forward Render View";
    hardcodedRenderGraph->renderPassList[eRenderPass_MainView].ExecuteFn =
      ForwardRenderPass::Execute;

    hardcodedRenderGraph->renderPassList[eRenderPass_ToneMapping].Init();
    hardcodedRenderGraph->renderPassList[eRenderPass_ToneMapping].numColorRTs = 1;
    hardcodedRenderGraph->renderPassList[eRenderPass_ToneMapping].colorRTs[0] =
      m_rtColorToneMappedHandle;
    hardcodedRenderGraph->renderPassList[eRenderPass_ToneMapping].inputResources[0] =
      m_rtColorHandle;
    hardcodedRenderGraph->renderPassList[eRenderPass_ToneMapping].depthRT =
      DefaultResHandle_Invalid;
    hardcodedRenderGraph->renderPassList[eRenderPass_ToneMapping].debugLabel =
      "Tone Mapping";
    hardcodedRenderGraph->renderPassList[eRenderPass_ToneMapping].ExecuteFn =
      ToneMappingRenderPass::Execute;

    hardcodedRenderGraph->renderPassList[eRenderPass_ComputeCopy].Init();
    hardcodedRenderGraph->renderPassList[eRenderPass_ComputeCopy].numColorRTs = 1;
    hardcodedRenderGraph->renderPassList[eRenderPass_ComputeCopy].colorRTs[0] =
      m_rtColorToneMappedHandle;
    hardcodedRenderGraph->renderPassList[eRenderPass_ComputeCopy].colorRTs[1] =
      m_computeColorHandle;
    hardcodedRenderGraph->renderPassList[eRenderPass_ComputeCopy].depthRT =
      DefaultResHandle_Invalid;
    hardcodedRenderGraph->renderPassList[eRenderPass_ComputeCopy].debugLabel =
      "Compute Copy";
    hardcodedRenderGraph->renderPassList[eRenderPass_ComputeCopy].ExecuteFn =
      ComputeCopyRenderPass::Execute;

    hardcodedRenderGraph->renderPassList[eRenderPass_DebugUI].Init();
    hardcodedRenderGraph->renderPassList[eRenderPass_DebugUI].numColorRTs = 1;
    hardcodedRenderGraph->renderPassList[eRenderPass_DebugUI].colorRTs[0] =
      m_computeColorHandle;
    hardcodedRenderGraph->renderPassList[eRenderPass_DebugUI].depthRT =
      DefaultResHandle_Invalid;
    hardcodedRenderGraph->renderPassList[eRenderPass_DebugUI].debugLabel = "Debug UI";
    hardcodedRenderGraph->renderPassList[eRenderPass_DebugUI].ExecuteFn =
      DebugUIRenderPass::Execute;

    hardcodedRenderGraph->renderPassList[eRenderPass_SwapChainCopy].Init();
    hardcodedRenderGraph->renderPassList[eRenderPass_SwapChainCopy].numColorRTs = 1;
    hardcodedRenderGraph->renderPassList[eRenderPass_SwapChainCopy].depthRT =
      DefaultResHandle_Invalid;
    hardcodedRenderGraph->renderPassList[eRenderPass_SwapChainCopy].debugLabel =
      "Swap Chain Copy";
    hardcodedRenderGraph->renderPassList[eRenderPass_SwapChainCopy].ExecuteFn =
      SwapChainCopyRenderPass::Execute;
  }

  static void Destroy(HardcodedRenderGraph* hardcodedRenderGraph)
  {
    // TODO: have an option to only destroy ones that are required to be
    // destroyed and recreated when the window resizes.
    // This just destroys all of them currently
    Graphics::DestroyResource(m_rtColorHandle);
    m_rtColorHandle = DefaultResHandle_Invalid;

    Graphics::DestroyResource(m_rtDepthHandle);
    m_rtDepthHandle = DefaultResHandle_Invalid;

    Graphics::DestroyResource(m_rtColorToneMappedHandle);
    m_rtColorToneMappedHandle = DefaultResHandle_Invalid;

    Graphics::DestroyResource(m_computeColorHandle);
    m_computeColorHandle = DefaultResHandle_Invalid;

    Graphics::DestroyDescriptor(gameGraphicsData.m_toneMappingDescHandle);
    gameGraphicsData.m_toneMappingDescHandle = Graphics::DefaultDescHandle_Invalid;

    Graphics::DestroyDescriptor(gameGraphicsData.m_swapChainCopyDescHandle);
    gameGraphicsData.m_swapChainCopyDescHandle = Graphics::DefaultDescHandle_Invalid;
  }

  static void Run(HardcodedRenderGraph* hardcodedRenderGraph,
                  Tk::Graphics::GraphicsCommandStream* graphicsCommandStream,
                  const FrameRenderParams& frameRenderParams,
                  const Tk::Platform::WindowHandles* windowHandles)
  {
    //TIMED_SCOPED_BLOCK("Render graph gfx cmd recording");

    // Have to set the swap chain handle manually
    // TODO: replace this during graphics command processing automatically
    hardcodedRenderGraph->renderPassList[eRenderPass_SwapChainCopy].colorRTs[0] =
      Graphics::GetCurrentSwapChainImage(windowHandles);

    for (GameRenderPass& currRP : hardcodedRenderGraph->renderPassList)
    {
      graphicsCommandStream->CmdDebugMarkerStart(currRP.debugLabel);
      currRP.ExecuteFn(&currRP, graphicsCommandStream, frameRenderParams);
      graphicsCommandStream->CmdTimestamp(currRP.debugLabel);
      graphicsCommandStream->CmdDebugMarkerEnd();
    }
  }

  void Create(const FrameRenderParams& frameRenderParams)
  {
    Create(&g_hardcodedRenderGraph, frameRenderParams);
  }

  void Destroy()
  {
    Destroy(&g_hardcodedRenderGraph);
  }

  void Run(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream,
           const FrameRenderParams& frameRenderParams,
           const Tk::Platform::WindowHandles* windowHandles)
  {
    Run(&g_hardcodedRenderGraph, graphicsCommandStream, frameRenderParams, windowHandles);
  }
} //namespace RenderGraph

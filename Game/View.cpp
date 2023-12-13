#include "View.h"
#include "Scene.h"
#include "AssetManager.h"
#include "Graphics/Common/GraphicsCommon.h"

void Init(View* view)
{
    *view = {};
}

void Update(View* view, Tk::Graphics::DescriptorSetDataHandles* descDataHandles)
{
    DescriptorData_Global globalData = {};
    globalData.viewProj = view->m_projMatrix * view->m_viewMatrix;

    // Globals
    Tk::Graphics::ResourceHandle bufferHandle = descDataHandles[0].handles[0];
    Tk::Graphics::MemoryMappedBufferPtr descDataBufferPtr_Global = Tk::Graphics::MapResource(bufferHandle);
    descDataBufferPtr_Global.MemcpyInto(&globalData, sizeof(DescriptorData_Global));
    Tk::Graphics::UnmapResource(bufferHandle);
}

#pragma once

#include "Platform/PlatformGameAPI.h"
#include "Graphics/Common/GraphicsCommon.h"

namespace Tk
{
namespace Core
{
namespace Graphics
{

/*
struct VirtualTexture
{
    Graphics::ResourceHandle m_fallback;
    Graphics::ResourceHandle m_pageTable;
    Graphics::ResourceHandle m_frames;
    Graphics::DescriptorHandle m_desc;
    // TODO: move this, it's for terrain, not this
    // terrain texturing data
    Graphics::ResourceHandle m_terrainData;
    Graphics::DescriptorHandle m_desc_terrain;
    // terrain mesh
    Graphics::ResourceHandle m_terrainVtxPos;
    Graphics::DescriptorHandle m_desc_terrainPos;
    Graphics::ResourceHandle m_terrainIdx;

    uint32 m_numPages;
    uint32 m_numFrames;
    v2ui m_frameDims;
    v2ui m_fallbackDims;

    void Reset()
    {
        m_fallback  = Graphics::DefaultResHandle_Invalid;
        m_pageTable = Graphics::DefaultResHandle_Invalid;
        m_frames    = Graphics::DefaultResHandle_Invalid;

        m_numPages = 0;
        m_numFrames = 0;
        m_frameDims = v2ui(0, 0);
        m_fallbackDims = v2ui(0, 0);
    }
    
    void Create(Tk::Platform::Graphics::GraphicsCommandStream* graphicsCommandStream, uint32 numPages, uint32 numFrames, const v2ui& pageDims, const v2ui& fallbackDims)
    {
        m_numPages = numPages; // pages in page table
        m_numFrames = numFrames; // number of textures actually in gpu memory

        Platform::Graphics::ResourceDesc desc;
        desc.resourceType = Platform::Graphics::ResourceType::eImage2D;

        // Create fallback texture
        desc.dims = v3ui(fallbackDims.x, fallbackDims.y, 1);
        desc.imageFormat = Platform::Graphics::ImageFormat::RGBA8_SRGB; // TODO: take format as parameter
        desc.arrayEles = 1;
        m_fallbackDims = fallbackDims;
        m_fallback = Platform::Graphics::CreateResource(desc);

        // Create texture of frames
        desc.dims = v3ui(pageDims.x, pageDims.y, 1);
        desc.arrayEles = numFrames;
        desc.imageFormat = Platform::Graphics::ImageFormat::RGBA8_SRGB; // TODO: take format as parameter
        m_frameDims = fallbackDims;
        m_frames = Platform::Graphics::CreateResource(desc);

        // Create page table - buffer
        desc.resourceType = Platform::Graphics::ResourceType::eBuffer1D;
        desc.bufferUsage = Platform::Graphics::BufferUsage::eUniform;
        desc.dims = v3ui(128 * sizeof(v4ui), 0, 0); // stores an array of uints that indicate the which page is being used
        m_pageTable = Platform::Graphics::CreateResource(desc);

        m_desc = Platform::Graphics::CreateDescriptor(Platform::Graphics::DESCLAYOUT_ID_VIRTUAL_TEXTURE);

        Platform::Graphics::DescriptorSetDataHandles descHandles = {};
        descHandles.InitInvalid();
        descHandles.handles[0] = m_frames;
        descHandles.handles[1] = m_pageTable;
        descHandles.handles[2] = m_fallback;
        Platform::Graphics::WriteDescriptor(Platform::Graphics::DESCLAYOUT_ID_VIRTUAL_TEXTURE, &m_desc, 1, &descHandles, 3);

        // With immediate submission, write texture data to gpu and initialize page table
        InitializeGPUTextures(platformFuncs, graphicsCommandStream);
        //-----

        InitializeTerrainData(platformFuncs, graphicsCommandStream);
    }

    void InitializeTerrainData(Tk::Platform::GraphicsCommandStream* graphicsCommandStream)
    {
        // Terrain-specific stuff - TODO: move these outta here
        m_desc_terrain = Platform::Graphics::CreateDescriptor(Platform::Graphics::DESCLAYOUT_ID_TERRAIN_DATA);

        Platform::Graphics::ResourceDesc desc;
        desc.resourceType = Platform::Graphics::ResourceType::eBuffer1D;
        desc.bufferUsage = Platform::Graphics::BufferUsage::eUniform;
        desc.dims = v3ui(sizeof(uint32) * 4, 0, 0); // only need 2 uints right now, for number of terrain tiles along x and y
        m_terrainData = Platform::Graphics::CreateResource(desc);

        Platform::Graphics::DescriptorSetDataHandles descHandles = {};
        descHandles.InitInvalid();
        descHandles.handles[0] = m_terrainData;
        Platform::Graphics::WriteDescriptor(Platform::Graphics::DESCLAYOUT_ID_TERRAIN_DATA, &m_desc_terrain, 1, &descHandles, 1);

        // Terrain stuff
        void* bufferMapped = Platform::Graphics::MapResource(m_terrainData);
        uint32* terrainData = (uint32*)bufferMapped;
        terrainData[0] = 4;
        terrainData[1] = 4;
        Platform::Graphics::UnmapResource(m_terrainData);

        // Terrain vertex positions
        desc.resourceType = Platform::Graphics::ResourceType::eBuffer1D;
        desc.bufferUsage = Platform::Graphics::BufferUsage::eVertex;
        desc.dims = v3ui(sizeof(v4f) * 4, 0, 0);
        m_terrainVtxPos = Platform::Graphics::CreateResource(desc);

        desc.resourceType = Platform::Graphics::ResourceType::eBuffer1D;
        desc.bufferUsage = Platform::Graphics::BufferUsage::eIndex;
        desc.dims = v3ui(sizeof(uint32) * 6, 0, 0);
        m_terrainIdx = Platform::Graphics::CreateResource(desc);

        m_desc_terrainPos = Platform::Graphics::CreateDescriptor(Platform::Graphics::DESCLAYOUT_ID_POSONLY_VBS);

        Platform::Graphics::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
        for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        {
            descDataHandles[i].InitInvalid();
        }
        descDataHandles[0].handles[0] = m_terrainVtxPos;

        Platform::Graphics::WriteDescriptor(Platform::Graphics::DESCLAYOUT_ID_POSONLY_VBS, &m_desc_terrainPos, 1, &descDataHandles[0], 1);
        
        // Mem transfer vtx/idx buffers from staging buffers
        desc.resourceType = Platform::Graphics::ResourceType::eBuffer1D;
        desc.dims = v3ui(sizeof(v4f) * 4, 1, 1);
        desc.bufferUsage = Platform::Graphics::BufferUsage::eStaging;
        Platform::Graphics::ResourceHandle stagingBufferVtx = Platform::Graphics::CreateResource(desc);
        void* stagingBufferMapped = Platform::Graphics::MapResource(stagingBufferVtx);
        const float scale = 20.0f;
        ((v4f*)stagingBufferMapped)[0] = v4f(-1.0f * scale, -1.0f * scale, -1.0f, 1);
        ((v4f*)stagingBufferMapped)[1] = v4f( 1.0f * scale, -1.0f * scale, -1.0f, 1);
        ((v4f*)stagingBufferMapped)[2] = v4f(-1.0f * scale,  1.0f * scale, -1.0f, 1);
        ((v4f*)stagingBufferMapped)[3] = v4f( 1.0f * scale,  1.0f * scale, -1.0f, 1);
        Platform::Graphics::UnmapResource(stagingBufferVtx);

        desc.resourceType = Platform::Graphics::ResourceType::eBuffer1D;
        desc.dims = v3ui(sizeof(uint32) * 6, 1, 1);
        desc.bufferUsage = Platform::Graphics::BufferUsage::eStaging;
        Platform::Graphics::ResourceHandle stagingBufferIdx = Platform::Graphics::CreateResource(desc);
        stagingBufferMapped = Platform::Graphics::MapResource(stagingBufferIdx);
        ((uint32*)stagingBufferMapped)[0] = 0;
        ((uint32*)stagingBufferMapped)[1] = 1;
        ((uint32*)stagingBufferMapped)[2] = 2;
        ((uint32*)stagingBufferMapped)[3] = 2;
        ((uint32*)stagingBufferMapped)[4] = 1;
        ((uint32*)stagingBufferMapped)[5] = 3;
        Platform::Graphics::UnmapResource(stagingBufferIdx);

        Tk::Platform::Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

        command->m_commandType = Platform::Graphics::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Terrain Vtx Pos Buf";
        command->m_sizeInBytes = sizeof(v4f) * 4;
        command->m_srcBufferHandle = stagingBufferVtx;
        command->m_dstBufferHandle = m_terrainVtxPos;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        command->m_commandType = Platform::Graphics::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Terrain Vtx Idx Buf";
        command->m_sizeInBytes = sizeof(uint32) * 6;
        command->m_srcBufferHandle = stagingBufferIdx;
        command->m_dstBufferHandle = m_terrainIdx;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        Platform::Graphics::SubmitCmdsImmediate(graphicsCommandStream);
        graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream

        Platform::Graphics::DestroyResource(stagingBufferVtx);
        Platform::Graphics::DestroyResource(stagingBufferIdx);
    }

    void InitializeGPUTextures(Tk::Platform::GraphicsCommandStream* graphicsCommandStream)
    {
        // Initialize fallback texture to white
        Platform::Graphics::ResourceDesc desc;
        desc.resourceType = Platform::Graphics::ResourceType::eBuffer1D;
        desc.dims = v3ui(m_fallbackDims.x * m_fallbackDims.y * sizeof(uint32), 1, 1);
        desc.bufferUsage = Platform::Graphics::BufferUsage::eStaging;
        Platform::Graphics::ResourceHandle stagingTexture_fallback = Platform::Graphics::CreateResource(desc);
        void* stagingBufferMapped = Platform::Graphics::MapResource(stagingTexture_fallback);

        uint32* texPixels = (uint32*)stagingBufferMapped;
        for (uint32 i = 0; i < m_fallbackDims.x * m_fallbackDims.y; ++i)
        {
            texPixels[i] = 0xFF00FFFF;
        }
        Platform::Graphics::UnmapResource(stagingTexture_fallback);

        // Initialize each physical frame texture to a different color 
        desc.dims = v3ui(m_frameDims.x * m_frameDims.y * m_numFrames * sizeof(uint32), 0, 0);
        desc.bufferUsage = Platform::Graphics::BufferUsage::eStaging;
        Platform::Graphics::ResourceHandle stagingTexture_frames = Platform::Graphics::CreateResource(desc);
        stagingBufferMapped = Platform::Graphics::MapResource(stagingTexture_frames);

        texPixels = (uint32*)stagingBufferMapped;
        for (uint32 frame = 0; frame < m_numFrames; ++frame)
        {
            for (uint32 i = 0; i < m_frameDims.x; ++i)
            {
                for (uint32 j = 0; j < m_frameDims.y; ++j)
                {
                    const uint32 pxOffset = frame * m_frameDims.x * m_frameDims.y + m_frameDims.x * i + j;
                    uint8 pixel[4] = {};
                    pixel[0] = (uint8)frame * 10;
                    pixel[1] = 0;
                    pixel[2] = 0;
                    pixel[3] = 0xFF;

                    texPixels[pxOffset] = *(uint32*)pixel;
                }
            }
        }
        Platform::Graphics::UnmapResource(stagingTexture_frames);

        // Initialize page table
        const uint32 MAX_PAGES = 128; // TODO: move this to some definition
        uint32 pageEntries[MAX_PAGES] = {};
        for (uint32 i = 0; i < MAX_PAGES; ++i)
        {
            pageEntries[i] = UINT32_MAX;
        }

        // Put an entry for each frame into the page table
        // TODO: ASSUMES that number of frames is <= the number of pages, need to assert or check that or something
        // TODO: make this actually correct
        for (uint32 i = 0; i < m_numFrames; ++i)
        {
            pageEntries[i] = i;
        }

        void* bufferMapped = Platform::Graphics::MapResource(m_pageTable);
        memcpy(bufferMapped, pageEntries, sizeof(uint32) * MAX_PAGES);
        Platform::Graphics::UnmapResource(m_pageTable);

        Tk::Platform::Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

        // Transition to transfer dst optimal layout
        command->m_commandType = Platform::Graphics::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition virtual fallback image layout to transfer dst optimal";
        command->m_imageHandle = m_fallback;
        command->m_startLayout = Platform::Graphics::ImageLayout::eUndefined;
        command->m_endLayout = Platform::Graphics::ImageLayout::eTransferDst;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Texture buffer copy
        command->m_commandType = Platform::Graphics::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update virtual fallback texture data";
        command->m_sizeInBytes = m_fallbackDims.x * m_fallbackDims.y * sizeof(uint32);
        command->m_srcBufferHandle = stagingTexture_fallback;
        command->m_dstBufferHandle = m_fallback;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        command->m_commandType = Platform::Graphics::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition virtual fallback image layout to shader read optimal";
        command->m_imageHandle = m_fallback;
        command->m_startLayout = Platform::Graphics::ImageLayout::eTransferDst;
        command->m_endLayout = Platform::Graphics::ImageLayout::eShaderRead;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Transition to transfer dst optimal layout
        command->m_commandType = Platform::Graphics::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition virtual frames array image layout to transfer dst optimal";
        command->m_imageHandle = m_frames;
        command->m_startLayout = Platform::Graphics::ImageLayout::eUndefined;
        command->m_endLayout = Platform::Graphics::ImageLayout::eTransferDst;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Texture buffer copy
        command->m_commandType = Platform::Graphics::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update virtual frames array texture data";
        command->m_sizeInBytes = m_frameDims.x * m_frameDims.y * m_numFrames * sizeof(uint32);
        command->m_srcBufferHandle = stagingTexture_frames;
        command->m_dstBufferHandle = m_frames;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        command->m_commandType = Platform::Graphics::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition virtual frames array image layout to shader read optimal";
        command->m_imageHandle = m_frames;
        command->m_startLayout = Platform::Graphics::ImageLayout::eTransferDst;
        command->m_endLayout = Platform::Graphics::ImageLayout::eShaderRead;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        Platform::Graphics::SubmitCmdsImmediate(graphicsCommandStream);
        graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream

        Platform::Graphics::DestroyResource(stagingTexture_fallback);
        Platform::Graphics::DestroyResource(stagingTexture_frames);
    }

    void Destroy()
    {
        Platform::Graphics::DestroyResource(m_fallback);
        Platform::Graphics::DestroyResource(m_pageTable);
        Platform::Graphics::DestroyResource(m_frames);
        Platform::Graphics::DestroyDescriptor(m_desc);

        // TODO: move these too
        Platform::Graphics::DestroyResource(m_terrainData);
        Platform::Graphics::DestroyDescriptor(m_desc_terrain);
        Platform::Graphics::DestroyResource(m_terrainVtxPos);
        Platform::Graphics::DestroyResource(m_terrainIdx);
        Platform::Graphics::DestroyDescriptor(m_desc_terrainPos);

        Reset();
    }

    void Update()
    {
        // TODO: check if state has changed somehow, update the LRU cache eventually, copy new textures to the GPU
        // TODO: need to figure out a way for the user to configure an update behavior, probably some template trick

        const uint32 MAX_PAGES = 128; // TODO: move this to some definition
        v4ui pageEntries[MAX_PAGES] = {};
        for (uint32 i = 0; i < MAX_PAGES; ++i)
        {
            pageEntries[i] = v4ui(UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX);
        }

        // Put an entry for each frame into the page table
        // TODO: ASSUMES that number of frames is <= the number of pages, need to assert or check that or something
        // TODO: make this actually correct
        for (uint32 i = 0; i < m_numFrames; ++i)
        {
            pageEntries[i] = v4ui(i, UINT32_MAX, UINT32_MAX, UINT32_MAX);
        }

        void* bufferMapped = Platform::Graphics::MapResource(m_pageTable);
        memcpy(bufferMapped, pageEntries, sizeof(v4ui) * MAX_PAGES);
        Platform::Graphics::UnmapResource(m_pageTable);

        // Terrain stuff
        bufferMapped = Platform::Graphics::MapResource(m_terrainData);
        uint32* terrainData = (uint32*)bufferMapped;
        terrainData[0] = 4;
        terrainData[1] = 4;
        Platform::Graphics::UnmapResource(m_terrainData);
    }
};*/

}
}
}

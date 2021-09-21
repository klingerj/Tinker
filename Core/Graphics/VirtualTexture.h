#pragma once

#include "Platform/PlatformGameAPI.h"

namespace Tk
{
namespace Core
{
namespace Graphics
{

struct VirtualTexture
{
    Platform::ResourceHandle m_fallback;
    Platform::ResourceHandle m_pageTable;
    Platform::ResourceHandle m_frames;
    Platform::DescriptorHandle m_desc;
    // TODO: move this, it's for terrain, not this
    Platform::ResourceHandle m_terrainData;
    Platform::DescriptorHandle m_desc_terrain;


    uint32 m_numPages;
    uint32 m_numFrames;
    v2ui m_frameDims;
    v2ui m_fallbackDims;

    void Reset()
    {
        m_fallback = Platform::DefaultResHandle_Invalid;
        m_pageTable = Platform::DefaultResHandle_Invalid;
        m_frames = Platform::DefaultResHandle_Invalid;

        m_numPages = 0;
        m_numFrames = 0;
        m_frameDims = v2ui(0, 0);
        m_fallbackDims = v2ui(0, 0);
    }
    
    void Create(const Tk::Platform::PlatformAPIFuncs* platformFuncs, Tk::Platform::GraphicsCommandStream* graphicsCommandStream, uint32 numPages, uint32 numFrames, const v2ui& pageDims, const v2ui& fallbackDims)
    {
        m_numPages = numPages; // pages in page table
        m_numFrames = numFrames; // number of textures actually in gpu memory

        Platform::ResourceDesc desc;
        desc.resourceType = Platform::ResourceType::eImage2D;

        // Create fallback texture
        desc.dims = v3ui(fallbackDims.x, fallbackDims.y, 1);
        desc.imageFormat = Platform::ImageFormat::RGBA8_SRGB; // TODO: take format as parameter
        desc.arrayEles = 1;
        m_fallbackDims = fallbackDims;
        m_fallback = platformFuncs->CreateResource(desc);

        // Create texture of frames
        desc.dims = v3ui(pageDims.x, pageDims.y, 1);
        desc.arrayEles = numFrames;
        desc.imageFormat = Platform::ImageFormat::RGBA8_SRGB; // TODO: take format as parameter
        m_frameDims = fallbackDims;
        m_frames = platformFuncs->CreateResource(desc);

        // Create page table - buffer
        desc.resourceType = Platform::ResourceType::eBuffer1D;
        desc.bufferUsage = Platform::BufferUsage::eUniform;
        desc.dims = v3ui(numPages * sizeof(uint32), 0, 0); // stores an array of uints that indicate the which page is being used
        m_pageTable = platformFuncs->CreateResource(desc);

        m_desc = platformFuncs->CreateDescriptor(Platform::DESCLAYOUT_ID_VIRTUAL_TEXTURE);

        Platform::DescriptorSetDataHandles descHandles = {};
        descHandles.InitInvalid();
        descHandles.handles[0] = m_frames;
        descHandles.handles[1] = m_pageTable;
        descHandles.handles[2] = m_fallback;
        platformFuncs->WriteDescriptor(Platform::DESCLAYOUT_ID_VIRTUAL_TEXTURE, &m_desc, 1, &descHandles, 3);

        // With immediate submission, write texture data to gpu and initialize page table
        InitializeGPUTextures(platformFuncs, graphicsCommandStream);
        //-----

        // Terrain-specific stuff - TODO: move these outta here
        m_desc_terrain = platformFuncs->CreateDescriptor(Platform::DESCLAYOUT_ID_TERRAIN_DATA);

        desc.resourceType = Platform::ResourceType::eBuffer1D;
        desc.dims = v3ui(numPages * sizeof(uint32), 0, 0); // stores an array of uints that indicate the which page is being used
        desc.dims = v3ui(sizeof(uint32) * 4, 0, 0); // only need 2 uints right now, for number of terrain tiles along x and y
        m_terrainData = platformFuncs->CreateResource(desc);

        descHandles.InitInvalid();
        descHandles.handles[0] = m_terrainData;
        platformFuncs->WriteDescriptor(Platform::DESCLAYOUT_ID_TERRAIN_DATA, &m_desc_terrain, 1, &descHandles, 1);
        //-----
    }

    void InitializeGPUTextures(const Tk::Platform::PlatformAPIFuncs* platformFuncs, Tk::Platform::GraphicsCommandStream* graphicsCommandStream)
    {
        // Initialize fallback texture to white
        Platform::ResourceDesc desc;
        desc.resourceType = Platform::ResourceType::eBuffer1D;
        desc.dims = v3ui(m_fallbackDims.x * m_fallbackDims.y * sizeof(uint32), 1, 1);
        desc.bufferUsage = Platform::BufferUsage::eStaging;
        Platform::ResourceHandle stagingTexture_fallback = platformFuncs->CreateResource(desc);
        void* stagingBufferMapped = platformFuncs->MapResource(stagingTexture_fallback);

        uint32* texPixels = (uint32*)stagingBufferMapped;
        for (uint32 i = 0; i < m_fallbackDims.x * m_fallbackDims.y; ++i)
        {
            texPixels[i] = 0xFFFFFFFF;
        }
        platformFuncs->UnmapResource(stagingTexture_fallback);

        // Initialize each physical frame texture to a different color 
        desc.dims = v3ui(m_frameDims.x * m_frameDims.y * m_numFrames * sizeof(uint32), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eStaging;
        Platform::ResourceHandle stagingTexture_frames = platformFuncs->CreateResource(desc);
        stagingBufferMapped = platformFuncs->MapResource(stagingTexture_frames);

        texPixels = (uint32*)stagingBufferMapped;
        for (uint32 frame = 0; frame < m_numFrames; ++frame)
        {
            for (uint32 i = 0; i < m_frameDims.x; ++i)
            {
                for (uint32 j = 0; j < m_frameDims.y; ++j)
                {
                    const uint32 pxOffset = frame * m_frameDims.x * m_frameDims.y + m_frameDims.x * i + j;
                    uint8 pixel[4] = {};
                    pixel[0] = 0xFF;
                    pixel[1] = 0;
                    pixel[2] = 0;
                    pixel[3] = 0xFF;

                    texPixels[pxOffset] = *(uint32*)pixel;
                }
            }
        }
        platformFuncs->UnmapResource(stagingTexture_frames);

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
        // TODO: create the staging buffer here for this too and copy the stuff in
        Tk::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

        // Transition to transfer dst optimal layout
        command->m_commandType = Platform::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition virtual fallback image layout to transfer dst optimal";
        command->m_imageHandle = m_fallback;
        command->m_startLayout = Platform::ImageLayout::eUndefined;
        command->m_endLayout = Platform::ImageLayout::eTransferDst;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Texture buffer copy
        command->m_commandType = Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update virtual fallback texture data";
        command->m_sizeInBytes = m_fallbackDims.x * m_fallbackDims.y * sizeof(uint32);
        command->m_srcBufferHandle = stagingTexture_fallback;
        command->m_dstBufferHandle = m_fallback;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Transition to transfer dst optimal layout
        command->m_commandType = Platform::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition virtual fallback image layout to transfer dst optimal";
        command->m_imageHandle = m_frames;
        command->m_startLayout = Platform::ImageLayout::eUndefined;
        command->m_endLayout = Platform::ImageLayout::eTransferDst;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Texture buffer copy
        command->m_commandType = Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update virtual fallback texture data";
        command->m_sizeInBytes = m_frameDims.x * m_frameDims.y * m_numFrames * sizeof(uint32);
        command->m_srcBufferHandle = stagingTexture_frames;
        command->m_dstBufferHandle = m_frames;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        platformFuncs->SubmitCmdsImmediate(graphicsCommandStream);
        graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream

        // TODO: issue a bunch of mem transfer cmds and copy all this data to the actually gpu resources, then delete all the staging buffers
        
        platformFuncs->DestroyResource(stagingTexture_fallback);
        platformFuncs->DestroyResource(stagingTexture_frames);
    }

    void Destroy(const Tk::Platform::PlatformAPIFuncs* platformFuncs)
    {
        platformFuncs->DestroyResource(m_fallback);
        platformFuncs->DestroyResource(m_pageTable);
        platformFuncs->DestroyResource(m_frames);
        platformFuncs->DestroyDescriptor(m_desc);

        // TODO: move these too
        platformFuncs->DestroyResource(m_terrainData);
        platformFuncs->DestroyDescriptor(m_desc_terrain);

        Reset();
    }

    void Update(const Tk::Platform::PlatformAPIFuncs* platformFuncs)
    {
        // TODO: check if state has changed somehow, update the LRU cache eventually, copy new textures to the GPU
        // TODO: need to figure out a way for the user to configure an update behavior, probably some template trick
    }
};

}
}
}

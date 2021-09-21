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
    Platform::ResourceHandle m_pages;
    Platform::DescriptorHandle m_desc;
    // TODO: move this, it's for terrain, not this
    Platform::ResourceHandle m_terrainData;
    Platform::DescriptorHandle m_desc_terrain;


    uint32 m_numPages;
    uint32 m_numFrames;
    v2ui m_pageDims;
    v2ui m_fallbackDims;

    void Reset()
    {
        m_fallback = Platform::DefaultResHandle_Invalid;
        m_pageTable = Platform::DefaultResHandle_Invalid;
        m_pages = Platform::DefaultResHandle_Invalid;

        m_numPages = 0;
        m_numFrames = 0;
        m_pageDims = v2ui(0, 0);
        m_fallbackDims = v2ui(0, 0);
    }
    
    void Create(const Tk::Platform::PlatformAPIFuncs* platformFuncs, uint32 numPages, uint32 numFrames, const v2ui& pageDims, const v2ui& fallbackDims)
    {
        m_numPages = numPages; // pages in page table
        m_numFrames = numFrames; // number of textures actually in gpu memory

        Platform::ResourceDesc desc;
        desc.resourceType = Platform::ResourceType::eImage2D;

        // Create fallback texture
        desc.dims = v3ui(fallbackDims.x, fallbackDims.y, 1);
        desc.imageFormat = Platform::ImageFormat::RGBA8_SRGB; // TODO: take format as parameter
        m_fallbackDims = fallbackDims;
        m_fallback = platformFuncs->CreateResource(desc);

        // Create texture of frames - TODO: make this a texture array, need to edit graphics backend to support this
        desc.dims = v3ui(pageDims.x, pageDims.y, 1);
        // TODO: array elements = numFrames
        desc.imageFormat = Platform::ImageFormat::RGBA8_SRGB; // TODO: take format as parameter
        m_pageDims = fallbackDims;
        m_pages = platformFuncs->CreateResource(desc);

        // Create page table - buffer
        desc.resourceType = Platform::ResourceType::eBuffer1D;
        desc.dims = v3ui(numPages * sizeof(uint32), 0, 0); // stores an array of uints that indicate the which page is being used
        m_pageTable = platformFuncs->CreateResource(desc);

        m_desc = platformFuncs->CreateDescriptor(Tk::Platform::DESCLAYOUT_ID_VIRTUAL_TEXTURE);

        Platform::DescriptorSetDataHandles descHandles = {};
        descHandles.InitInvalid();
        descHandles.handles[0] = m_pages;
        descHandles.handles[1] = m_pageTable;
        descHandles.handles[2] = m_fallback;
        platformFuncs->WriteDescriptor(Tk::Platform::SHADER_ID_BASIC_VirtualTexture, &m_desc, 1, &descHandles, 3);

        // With immediate submission, write texture data to gpu and initialize page table
        InitializeGPUTextures(platformFuncs);
        //-----

        // Terrain-specific stuff - TODO: move these outta here
        m_desc_terrain = platformFuncs->CreateDescriptor(Tk::Platform::DESCLAYOUT_ID_TERRAIN_DATA);

        desc.resourceType = Platform::ResourceType::eBuffer1D;
        desc.dims = v3ui(sizeof(uint32) * 4, 0, 0); // only need 2 uints right now, for number of terrain tiles along x and y
        m_terrainData = platformFuncs->CreateResource(desc);

        descHandles.InitInvalid();
        descHandles.handles[0] = m_terrainData;
        platformFuncs->WriteDescriptor(Tk::Platform::SHADER_ID_BASIC_VirtualTexture, &m_desc_terrain, 1, &descHandles, 1);
        //-----
    }

    void InitializeGPUTextures(const Tk::Platform::PlatformAPIFuncs* platformFuncs)
    {
        // Initialize fallback texture to white
        Platform::ResourceDesc desc;
        desc.dims = v3ui(m_fallbackDims.x, m_fallbackDims.y, 0);
        desc.bufferUsage = Platform::BufferUsage::eStaging;
        Platform::ResourceHandle stagingTexture_fallback = platformFuncs->CreateResource(desc);
        void* stagingBufferMapped = platformFuncs->MapResource(stagingTexture_fallback);

        uint32* texPixels = (uint32*)stagingBufferMapped;
        for (uint32 i = 0; i < desc.dims.x; ++i)
        {
            for (uint32 j = 0; j < desc.dims.y; ++j)
            {
                const uint32 pxOffset = i + j * desc.dims.x;
                texPixels[pxOffset] = 0xFFFFFFFF;
            }
        }
        platformFuncs->UnmapResource(stagingTexture_fallback);

        // Initialize each physical frame texture to a different color 
        desc.dims = v3ui(m_pageDims.x * m_numFrames, m_pageDims.y, 0);
        desc.bufferUsage = Platform::BufferUsage::eStaging;
        Platform::ResourceHandle stagingTexture_frames = platformFuncs->CreateResource(desc);
        stagingBufferMapped = platformFuncs->MapResource(stagingTexture_frames);

        // TODO: probably change this a bit, going to change it to be a texture array resource
        texPixels = (uint32*)stagingBufferMapped;
        for (uint32 frame = 0; frame < m_numFrames; ++frame)
        {
            for (uint32 i = 0; i < m_pageDims.x; ++i)
            {
                for (uint32 j = 0; j < m_pageDims.y; ++j)
                {
                    const uint32 pxOffset = j * frame + i * m_pageDims.x * frame;
                    uint8 pixel[4] = {};
                    pixel[0] = (uint8)frame;
                    pixel[1] = (uint8)(i % m_numFrames);
                    pixel[2] = (uint8)(j % m_numFrames);
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

        // TODO: issue a bunch of mem transfer cmds and copy all this data to the actually gpu resources, then delete all the staging buffers
    }

    void Destroy(const Tk::Platform::PlatformAPIFuncs* platformFuncs)
    {
        platformFuncs->DestroyResource(m_fallback);
        platformFuncs->DestroyResource(m_pageTable);
        platformFuncs->DestroyResource(m_pages);
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

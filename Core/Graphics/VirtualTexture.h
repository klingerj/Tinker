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
    // TODO: descriptor

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

        // Create texture of frames
        desc.dims = v3ui(pageDims.x * numFrames, pageDims.y, 0);
        desc.imageFormat = Platform::ImageFormat::RGBA8_SRGB; // TODO: take format as parameter
        m_pageDims = fallbackDims;
        m_pages = platformFuncs->CreateResource(desc);

        // Create page table - buffer
        desc.resourceType = Platform::ResourceType::eBuffer1D;
        desc.dims = v3ui(numPages * sizeof(float), 0, 0); // stores an array of floats that indicate the UV offset into the page texture
        m_pageTable = platformFuncs->CreateResource(desc);

        // TODO: create descriptor using new layout for these descriptors
    }

    void Destroy()
    {
        platformFuncs->DestroyResource(m_fallback);
        platformFuncs->DestroyResource(m_pageTable);
        platformFuncs->DestroyResource(m_pages);
        Reset();
    }
};

}
}
}

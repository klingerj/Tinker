#include "AssetManager.h"
#include "Platform/PlatformGameAPI.h"
#include "Utility/Logging.h"
#include "Utility/ScopedTimer.h"
#include "Mem.h"
#include "Asset/AssetLoader.h"

#include <string.h>

using namespace Tk;
using namespace Core;

AssetManager g_AssetManager;

//static Tk::Core::LinearAllocator g_MeshBufferAllocator; // Persists after meshes are uploaded to the GPU
//static Tk::Core::LinearAllocator g_TextureBufferAllocator; // Dealloc'd after all textures are uploaded to the GPU

void AssetManager::LoadAllAssets(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    // Meshes
    m_numMeshAssets = 4;

    m_numTextureAssets = 2;
    /*
    // Textures
    accumFileOffset = 0;
    uint32 totalActualTextureSize = 0;
    uint32 actualTextureSizes[numTextureAssets] = {};
    // Parse each texture file and dump the actual texture contents into the allocator
    for (uint32 uiAsset = 0; uiAsset < numTextureAssets; ++uiAsset)
    {
        uint8* currentTextureFile = textureFileDataBuffer + accumFileOffset;
        uint32 currentTextureFileSize = textureFileSizes[uiAsset];
        accumFileOffset += currentTextureFileSize;

        uint32 actualTextureSizeInBytes = 0;

        if (strncmp(fileExt, "bmp", 3) == 0)
        {
            Asset::BMPInfo info = Asset::GetBMPInfo(currentTextureFile);

            // If it's a 24-bit bmp, pad to 32 bits. The 4th byte will be 255 (alpha of 1).
            uint32 bpp = 0;
            switch (info.bitsPerPixel)
            {
                case 24: // pad 24bpp to 32bpp
                case 32:
                {
                    bpp = 32;
                    break;
                }

                default:
                {
                    // Unsupported bmp variant
                    TINKER_ASSERT(0);
                }
            }

            actualTextureSizeInBytes = info.width * info.height * bpp / 8;
            m_allTextureMetadata[uiAsset].m_dims = v3ui(info.width, info.height, 1);
            m_allTextureMetadata[uiAsset].m_bitsPerPixel = info.bitsPerPixel; // store the original bpp in the texture metadata
        }
        else
        {
            TINKER_ASSERT(0);
            // TODO: support other file types, e.g. png
        }

        actualTextureSizes[uiAsset] = actualTextureSizeInBytes;
        totalActualTextureSize += actualTextureSizeInBytes;
    }

    // Allocate exactly enough space for each texture, cache-line aligned
    m_textureBufferAllocator.Init(totalActualTextureSize, CACHE_LINE);

    accumFileOffset = 0;
    for (uint32 uiAsset = 0; uiAsset < numTextureAssets; ++uiAsset)
    {
        uint8* currentTextureFile = textureFileDataBuffer + accumFileOffset;
        uint32 currentTextureFileSize = textureFileSizes[uiAsset];
        accumFileOffset += currentTextureFileSize;

        uint8* textureBuffer = m_textureBufferAllocator.Alloc(actualTextureSizes[uiAsset], 1);

        if (strncmp(fileExt, "bmp", 3) == 0)
        {
            uint8* textureBytesStart = currentTextureFile + sizeof(Asset::BMPHeader) + sizeof(Asset::BMPInfo);

            // Copy the bmp bytes in - no parsing for bmp
            // If the bits per pixel is 24, copy pixel by pixel and write a 0xFF byte for each alpha byte;
            uint32 numPixels = m_allTextureMetadata[uiAsset].m_dims.x * m_allTextureMetadata[uiAsset].m_dims.y;
            switch (m_allTextureMetadata[uiAsset].m_bitsPerPixel)
            {
                case 24:
                {
                    for (uint32 uiPixel = 0; uiPixel < numPixels; ++uiPixel)
                    {
                        memcpy(textureBuffer, textureBytesStart, 3);
                        textureBuffer += 3;
                        textureBytesStart += 3;
                        textureBuffer[0] = 255; // write fully opaque alpha
                        ++textureBuffer;
                    }
                    break;
                }

                case 32:
                {
                    memcpy(textureBuffer, textureBytesStart, actualTextureSizes[uiAsset]);
                    break;
                }

                default:
                {
                    // Unsupported bmp variant
                    TINKER_ASSERT(0);
                    break;
                }
            }
        }
        else
        {
            TINKER_ASSERT(0);
            // TODO: support other file types, e.g. png
        }
    }*/
}

/*void AssetManager::InitAssetGraphicsResources(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    // Meshes
    for (uint32 uiAsset = 0; uiAsset < m_numMeshAssets; ++uiAsset)
    {
        
    }

    // Textures
    Graphics::ResourceHandle imageStagingBufferHandles[TINKER_MAX_TEXTURES] = {};

    uint32 accumTextureOffset = 0;
    for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
    {
        // Create texture handles
        Graphics::ResourceDesc desc;
        desc.resourceType = Graphics::ResourceType::eImage2D;
        desc.arrayEles = 1;

        desc.imageFormat = Graphics::ImageFormat::RGBA8_SRGB; // TODO: don't hard code this
        desc.dims = m_allTextureMetadata[uiAsset].m_dims;
        m_allTextureGraphicsHandles[uiAsset] = Graphics::CreateResource(desc);

        uint32 textureSizeInBytes = m_allTextureMetadata[uiAsset].m_dims.x * m_allTextureMetadata[uiAsset].m_dims.y * 4; // 4 bytes per pixel since RGBA8
        desc.dims = v3ui(textureSizeInBytes, 0, 0);
        desc.resourceType = Graphics::ResourceType::eBuffer1D; // staging buffer is just a 1D buffer
        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        imageStagingBufferHandles[uiAsset] = Graphics::CreateResource(desc);
        void* stagingBufferMemPtr = Graphics::MapResource(imageStagingBufferHandles[uiAsset]);

        // Copy texture data into the staging buffer
        uint8* currentTextureFile = m_textureBufferAllocator.m_ownedMemPtr + accumTextureOffset;
        accumTextureOffset += textureSizeInBytes;
        memcpy(stagingBufferMemPtr, currentTextureFile, textureSizeInBytes);
    }

    // Graphics command to copy from staging buffer to gpu local buffer
    Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    // Create, submit, and execute the buffer copy commands
    for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
    {
        uint32 textureSizeInBytes = m_allTextureMetadata[uiAsset].m_dims.x * m_allTextureMetadata[uiAsset].m_dims.y * 4; // 4 bytes per pixel since RGBA8

        // Transition to transfer dst optimal layout
        command->m_commandType = Graphics::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition image layout to transfer dst optimal";
        command->m_imageHandle = m_allTextureGraphicsHandles[uiAsset];
        command->m_startLayout = Graphics::ImageLayout::eUndefined;
        command->m_endLayout = Graphics::ImageLayout::eTransferDst;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Texture buffer copy
        command->m_commandType = Graphics::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Asset Texture Data";
        command->m_sizeInBytes = textureSizeInBytes;
        command->m_srcBufferHandle = imageStagingBufferHandles[uiAsset];
        command->m_dstBufferHandle = m_allTextureGraphicsHandles[uiAsset];
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // TODO: transition image to shader read optimal?
    }

    // Perform the copies
    Graphics::SubmitCmdsImmediate(graphicsCommandStream);
    graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream

    // Destroy the staging buffers
    for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
    {
        Graphics::UnmapResource(imageStagingBufferHandles[uiAsset]);
        Graphics::DestroyResource(imageStagingBufferHandles[uiAsset]);
    }

    m_textureBufferAllocator.ExplicitFree();
}*/

void AssetManager::DestroyAllMeshData()
{

    /*
    for (uint32 uiAssetID = 0; uiAssetID < m_numMeshAssets; ++uiAssetID)
    {
        StaticMeshData* meshData = &g_allStaticMeshGraphicsHandles[uiAssetID];

        Graphics::DestroyResource(meshData->m_positionBuffer.gpuBufferHandle);
        meshData->m_positionBuffer.gpuBufferHandle = Core::Graphics::DefaultResHandle_Invalid;
        Graphics::DestroyResource(meshData->m_uvBuffer.gpuBufferHandle);
        meshData->m_uvBuffer.gpuBufferHandle = Core::Graphics::DefaultResHandle_Invalid;
        Graphics::DestroyResource(meshData->m_normalBuffer.gpuBufferHandle);
        meshData->m_normalBuffer.gpuBufferHandle = Core::Graphics::DefaultResHandle_Invalid;
        Graphics::DestroyResource(meshData->m_indexBuffer.gpuBufferHandle);
        meshData->m_indexBuffer.gpuBufferHandle = Core::Graphics::DefaultResHandle_Invalid;

        Graphics::DestroyDescriptor(meshData->m_descriptor);
        meshData->m_descriptor = Core::Graphics::DefaultDescHandle_Invalid;
    }*/
}

void AssetManager::DestroyAllTextureData()
{
    for (uint32 uiAssetID = 0; uiAssetID < m_numTextureAssets; ++uiAssetID)
    {
        Graphics::ResourceHandle textureHandle = GetTextureGraphicsDataByID(uiAssetID);
        Graphics::DestroyResource(textureHandle);
    }
}

const Tk::Core::Graphics::StaticMeshData* AssetManager::GetMeshGraphicsDataByID(uint32 meshID)
{
    TINKER_ASSERT(meshID != TINKER_INVALID_HANDLE);
    TINKER_ASSERT(meshID < TINKER_MAX_MESHES);
    return nullptr;//g_allStaticMeshGraphicsHandles[meshID];
}

Graphics::ResourceHandle AssetManager::GetTextureGraphicsDataByID(uint32 textureID) const
{
    TINKER_ASSERT(textureID != TINKER_INVALID_HANDLE);
    TINKER_ASSERT(textureID < TINKER_MAX_TEXTURES);
    return m_allTextureGraphicsHandles[textureID];
}

const MeshAttributeData& AssetManager::GetMeshAttrDataByID(uint32 meshID) const
{
    TINKER_ASSERT(meshID != TINKER_INVALID_HANDLE);
    TINKER_ASSERT(meshID < TINKER_MAX_MESHES);
    return m_allMeshData[meshID];
}


#include "AssetManager.h"
#include "Utility/Logging.h"
#include "AssetFileParsing.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "Platform/PlatformGameAPI.h"
#include "Mem.h"
#include "Utility/ScopedTimer.h"

#include <string.h>

using namespace Tk;
using namespace Platform;
using namespace Core;

AssetManager g_AssetManager;

static const uint64 AssetFileMemorySize = 1024 * 1024 * 1024;
static Tk::Core::LinearAllocator g_AssetFileScratchMemory;

// For storing dumping obj vertex data during parsing
static Tk::Core::Asset::OBJParseScratchBuffers ScratchBuffers;

// For storing final obj vertex data
static Tk::Core::LinearAllocator VertPosAllocator;
static Tk::Core::LinearAllocator VertUVAllocator;
static Tk::Core::LinearAllocator VertNormalAllocator;
static Tk::Core::LinearAllocator VertIndexAllocator;

void AssetManager::FreeMemory()
{
    m_meshBufferAllocator.ExplicitFree();
}

void AssetManager::LoadAllAssets()
{
    g_AssetFileScratchMemory.Init(AssetFileMemorySize, CACHE_LINE);

    // Meshes
    const uint32 numMeshAssets = 4;
    TINKER_ASSERT(numMeshAssets <= TINKER_MAX_MESHES);

    m_numMeshAssets = numMeshAssets;

    const char* meshFilePaths[numMeshAssets] =
    {
        ASSETS_PATH "UnitSphere\\sphere.obj",
        ASSETS_PATH "UnitCube\\cube.obj",
        ASSETS_PATH "FireElemental\\fire_elemental.obj",
        ASSETS_PATH "RTX3090\\rtx3090.obj"
    };

    uint32 totalMeshFileBytes = 0;
    uint32 meshFileSizes[numMeshAssets] = {};
    for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
    {
        uint32 fileSize = GetEntireFileSize(meshFilePaths[uiAsset]); // + 1 byte for manual EOF byte
        meshFileSizes[uiAsset] = fileSize;
        totalMeshFileBytes += fileSize;
    }

    // Allocate one large buffer to store a dump of all obj files.
    // Each obj file's data is separated by a single null byte to mark EOF
    uint8* objFileDataBuffer = (uint8*)g_AssetFileScratchMemory.Alloc(totalMeshFileBytes + numMeshAssets, CACHE_LINE); // + 1 EOF byte for each obj file -> + numMeshAssets

    // Now precalculate the size of the vertex attribute buffers needed to contain the obj data
    uint32 meshBufferSizes[numMeshAssets] = {};
    uint32 totalMeshBufferSize = 0;

    uint32 accumFileOffset = 0;

    {
        TIMED_SCOPED_BLOCK("Load OBJ files from disk - multithreaded");

        bool multithreadObjLoading = true;
        if (multithreadObjLoading)
        {
            Tk::Platform::WorkerJobList jobs;
            jobs.Init(numMeshAssets);
            for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
            {
                uint8* currentObjFile = objFileDataBuffer + accumFileOffset;
                uint32 currentObjFileSize = meshFileSizes[uiAsset];
                accumFileOffset += currentObjFileSize + 1; // Account for manual EOF byte

                jobs.m_jobs[uiAsset] = Platform::CreateNewThreadJob([=]()
                    {
                        ReadEntireFile(meshFilePaths[uiAsset], currentObjFileSize, currentObjFile);
                        currentObjFile[currentObjFileSize] = '\0'; // Mark EOF
                    });

            }
            Tk::Platform::EnqueueWorkerThreadJobList_Assisted(&jobs);
            jobs.WaitOnJobs();
            jobs.FreeList();
        }
        else
        {
            for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
            {
                uint8* currentObjFile = objFileDataBuffer + accumFileOffset;
                uint32 currentObjFileSize = meshFileSizes[uiAsset];
                accumFileOffset += currentObjFileSize + 1; // Account for manual EOF byte

                currentObjFile[currentObjFileSize] = '\0'; // Mark EOF
                ReadEntireFile(meshFilePaths[uiAsset], currentObjFileSize, currentObjFile);
            }
        }
    }

    // Parse OBJs
    {
        TIMED_SCOPED_BLOCK("Parse OBJ Files - single threaded");

        const uint32 MAX_VERT_BUFFER_SIZE = 1024 * 1024 * 128; // TODO: reduce this when we don't have to store all the attr buffers in the same linear allocator by doing graphics right here
        VertPosAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
        VertUVAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
        VertNormalAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
        VertIndexAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);

        ScratchBuffers = {};
        ScratchBuffers.VertPosAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
        ScratchBuffers.VertUVAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
        ScratchBuffers.VertNormalAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);

        accumFileOffset = 0;
        uint32 accumNumVerts = 0;
        for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
        {
            const uint8* currentObjFile = objFileDataBuffer + accumFileOffset;
            uint32 currentObjFileSize = meshFileSizes[uiAsset];
            accumFileOffset += currentObjFileSize + 1; // Account for manual EOF byte

            uint32 numObjVerts = 0;
            Tk::Core::Asset::ParseOBJ(VertPosAllocator, VertUVAllocator, VertNormalAllocator, VertIndexAllocator, ScratchBuffers, currentObjFile, currentObjFileSize, &numObjVerts);
            TINKER_ASSERT(numObjVerts);
            m_allMeshData[uiAsset].m_numVertices = numObjVerts;
            m_allMeshData[uiAsset].m_vertexBufferData_Pos = VertPosAllocator.m_ownedMemPtr + sizeof(v4f) * accumNumVerts;
            m_allMeshData[uiAsset].m_vertexBufferData_UV = VertUVAllocator.m_ownedMemPtr + sizeof(v2f) * accumNumVerts;
            m_allMeshData[uiAsset].m_vertexBufferData_Normal = VertNormalAllocator.m_ownedMemPtr + sizeof(v3f) * accumNumVerts;
            m_allMeshData[uiAsset].m_vertexBufferData_Index = VertIndexAllocator.m_ownedMemPtr + sizeof(uint32) * accumNumVerts;
            // TODO: create the graphics buffers right away and don't bother storing m_allMeshData at all

            ScratchBuffers.ResetState();
            accumNumVerts += numObjVerts;
        }
    }
    //-----

    // Textures
    const uint32 numTextureAssets = 2;
    TINKER_ASSERT(numTextureAssets <= TINKER_MAX_TEXTURES);

    m_numTextureAssets = numTextureAssets;

    const char* textureFilePaths[numTextureAssets] =
    {
        ASSETS_PATH "checkerboard512.bmp",
        ASSETS_PATH "checkerboardRGB512.bmp"
    };
    // Compute the actual size of the texture data to be allocated and copied to the GPU
    // TODO: get file extension from string
    const char* fileExt = "bmp";

    uint32 totalTextureFileBytes = 0;
    uint32 textureFileSizes[numTextureAssets] = {};
    for (uint32 uiAsset = 0; uiAsset < numTextureAssets; ++uiAsset)
    {
        uint32 fileSize = GetEntireFileSize(textureFilePaths[uiAsset]);
        textureFileSizes[uiAsset] = fileSize;
        totalTextureFileBytes += fileSize;
    }

    // Allocate one large buffer to store a dump of all texture files.
    uint8* textureFileDataBuffer = (uint8*)g_AssetFileScratchMemory.Alloc(totalTextureFileBytes, CACHE_LINE);

    accumFileOffset = 0;

    bool multithreadTextureLoading = true;
    if (multithreadTextureLoading)
    {
        Platform::WorkerJobList jobs;
        jobs.Init(numTextureAssets);
        for (uint32 uiAsset = 0; uiAsset < numTextureAssets; ++uiAsset)
        {
            uint8* currentTextureFile = textureFileDataBuffer + accumFileOffset;
            uint32 currentTextureFileSize = textureFileSizes[uiAsset];
            accumFileOffset += currentTextureFileSize;

            jobs.m_jobs[uiAsset] = Platform::CreateNewThreadJob([=]()
                {
                    ReadEntireFile(textureFilePaths[uiAsset], currentTextureFileSize, currentTextureFile);
                });

            EnqueueWorkerThreadJob(jobs.m_jobs[uiAsset]);
        }
        jobs.WaitOnJobs();
        jobs.FreeList();
    }
    else
    {
        // Dump each file into the one big buffer
        for (uint32 uiAsset = 0; uiAsset < numTextureAssets; ++uiAsset)
        {
            uint8* currentTextureFile = textureFileDataBuffer + accumFileOffset;
            uint32 currentTextureFileSize = textureFileSizes[uiAsset];
            accumFileOffset += currentTextureFileSize;

            // Read each file into the buffer
            ReadEntireFile(textureFilePaths[uiAsset], currentTextureFileSize, currentTextureFile);
        }
    }

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
    }
}

void AssetManager::InitAssetGraphicsResources(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    // Meshes
    for (uint32 uiAsset = 0; uiAsset < m_numMeshAssets; ++uiAsset)
    {
        // Create buffer handles
        Graphics::ResourceDesc desc;
        desc.resourceType = Core::Graphics::ResourceType::eBuffer1D;

        Graphics::ResourceHandle stagingBufferHandle_Pos, stagingBufferHandle_UV, stagingBufferHandle_Norm, stagingBufferHandle_Idx;
        void* stagingBufferMemPtr_Pos, *stagingBufferMemPtr_UV, *stagingBufferMemPtr_Norm, *stagingBufferMemPtr_Idx;

        // Positions
        desc.dims = v3ui(m_allMeshData[uiAsset].m_numVertices * sizeof(v4f), 0, 0);
        desc.bufferUsage = Graphics::BufferUsage::eVertex;
        m_allStaticMeshGraphicsHandles[uiAsset].m_positionBuffer.gpuBufferHandle = Graphics::CreateResource(desc);

        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        stagingBufferHandle_Pos = Graphics::CreateResource(desc);
        stagingBufferMemPtr_Pos = Graphics::MapResource(stagingBufferHandle_Pos);

        // UVs
        desc.dims = v3ui(m_allMeshData[uiAsset].m_numVertices * sizeof(v2f), 0, 0);
        desc.bufferUsage = Graphics::BufferUsage::eVertex;
        m_allStaticMeshGraphicsHandles[uiAsset].m_uvBuffer.gpuBufferHandle = Graphics::CreateResource(desc);
        
        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        stagingBufferHandle_UV = Graphics::CreateResource(desc);
        stagingBufferMemPtr_UV = Graphics::MapResource(stagingBufferHandle_UV);

        // Normals
        desc.dims = v3ui(m_allMeshData[uiAsset].m_numVertices * sizeof(v4f), 0, 0);
        desc.bufferUsage = Graphics::BufferUsage::eVertex;
        m_allStaticMeshGraphicsHandles[uiAsset].m_normalBuffer.gpuBufferHandle = Graphics::CreateResource(desc);
        
        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        stagingBufferHandle_Norm = Graphics::CreateResource(desc);
        stagingBufferMemPtr_Norm = Graphics::MapResource(stagingBufferHandle_Norm);

        // Indices
        desc.dims = v3ui(m_allMeshData[uiAsset].m_numVertices * sizeof(uint32), 0, 0);
        desc.bufferUsage = Graphics::BufferUsage::eIndex;
        m_allStaticMeshGraphicsHandles[uiAsset].m_indexBuffer.gpuBufferHandle = Graphics::CreateResource(desc);
        
        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        stagingBufferHandle_Idx = Graphics::CreateResource(desc);
        stagingBufferMemPtr_Idx = Graphics::MapResource(stagingBufferHandle_Idx);

        m_allStaticMeshGraphicsHandles[uiAsset].m_numIndices = m_allMeshData[uiAsset].m_numVertices;

        // Descriptor
        CreateVertexBufferDescriptor(uiAsset);

        // Memcpy data into staging buffer
        const uint32 numPositionBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v4f);
        const uint32 numUVBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v2f);
        const uint32 numNormalBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v3f);
        const uint32 numIndexBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(uint32);

        v4f* positionBuffer = (v4f*)m_allMeshData[uiAsset].m_vertexBufferData_Pos;
        v2f* uvBuffer       = (v2f*)m_allMeshData[uiAsset].m_vertexBufferData_UV;
        v3f* normalBuffer   = (v3f*)m_allMeshData[uiAsset].m_vertexBufferData_Normal;
        uint32* indexBuffer = (uint32*)m_allMeshData[uiAsset].m_vertexBufferData_Index;
        memcpy(stagingBufferMemPtr_Pos, positionBuffer, numPositionBytes);
        memcpy(stagingBufferMemPtr_UV, uvBuffer, numUVBytes);
        //memcpy(stagingBufferMemPtr_Norm, normalBuffer, numNormalBytes);
        for (uint32 i = 0; i < m_allMeshData[uiAsset].m_numVertices; ++i)
        {
            memcpy(((uint8*)stagingBufferMemPtr_Norm) + sizeof(v4f) * i, &normalBuffer[i], sizeof(v3f));
            memset(((uint8*)stagingBufferMemPtr_Norm) + sizeof(v4f) * i + sizeof(v3f), 0, sizeof(float)); // add 0 as 4th channel of normal vector
        }
        memcpy(stagingBufferMemPtr_Idx, indexBuffer, numIndexBytes);
        //-----

        // Create, submit, and execute the buffer copy commands
        {
            // Graphics command to copy from staging buffer to gpu local buffer
            Tk::Core::Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

            // Position buffer copy
            command->m_commandType = Graphics::GraphicsCmd::eMemTransfer;
            command->debugLabel = "Update Asset Vtx Pos Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v4f);
            command->m_srcBufferHandle = stagingBufferHandle_Pos;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_positionBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // UV buffer copy
            command->m_commandType = Graphics::GraphicsCmd::eMemTransfer;
            command->debugLabel = "Update Asset Vtx UV Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v2f);
            command->m_srcBufferHandle = stagingBufferHandle_UV;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_uvBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // Normal buffer copy
            command->m_commandType = Graphics::GraphicsCmd::eMemTransfer;
            command->debugLabel = "Update Asset Vtx Norm Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v4f);
            command->m_srcBufferHandle = stagingBufferHandle_Norm;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_normalBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // Index buffer copy
            command->m_commandType = Graphics::GraphicsCmd::eMemTransfer;
            command->debugLabel = "Update Asset Vtx Idx Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(uint32);
            command->m_srcBufferHandle = stagingBufferHandle_Idx;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_indexBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // Perform the copies
            Graphics::SubmitCmdsImmediate(graphicsCommandStream);
            graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream
        }

        // Unmap the buffer resource
        Graphics::UnmapResource(stagingBufferHandle_Pos);
        Graphics::UnmapResource(stagingBufferHandle_UV);
        Graphics::UnmapResource(stagingBufferHandle_Norm);
        Graphics::UnmapResource(stagingBufferHandle_Idx);

        // Destroy the staging buffer resources
        Graphics::DestroyResource(stagingBufferHandle_Pos);
        Graphics::DestroyResource(stagingBufferHandle_UV);
        Graphics::DestroyResource(stagingBufferHandle_Norm);
        Graphics::DestroyResource(stagingBufferHandle_Idx);
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
}

void AssetManager::DestroyAllMeshData()
{
    for (uint32 uiAssetID = 0; uiAssetID < m_numMeshAssets; ++uiAssetID)
    {
        StaticMeshData* meshData = GetMeshGraphicsDataByID(uiAssetID);

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
    }
}

void AssetManager::DestroyAllTextureData()
{
    for (uint32 uiAssetID = 0; uiAssetID < m_numTextureAssets; ++uiAssetID)
    {
        Graphics::ResourceHandle textureHandle = GetTextureGraphicsDataByID(uiAssetID);
        Graphics::DestroyResource(textureHandle);
    }
}

void AssetManager::CreateVertexBufferDescriptor(uint32 meshID)
{
    StaticMeshData* data = g_AssetManager.GetMeshGraphicsDataByID(meshID);
    data->m_descriptor = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_ASSET_VBS);

    Core::Graphics::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    descDataHandles[0].InitInvalid();
    descDataHandles[0].handles[0] = data->m_positionBuffer.gpuBufferHandle;
    descDataHandles[0].handles[1] = data->m_uvBuffer.gpuBufferHandle;
    descDataHandles[0].handles[2] = data->m_normalBuffer.gpuBufferHandle;
    descDataHandles[1].InitInvalid();
    descDataHandles[2].InitInvalid();

    Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_ASSET_VBS, data->m_descriptor, &descDataHandles[0], 1);
}

StaticMeshData* AssetManager::GetMeshGraphicsDataByID(uint32 meshID)
{
    TINKER_ASSERT(meshID != TINKER_INVALID_HANDLE);
    TINKER_ASSERT(meshID < TINKER_MAX_MESHES);
    return &m_allStaticMeshGraphicsHandles[meshID];
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


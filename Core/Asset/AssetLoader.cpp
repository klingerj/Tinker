#include "AssetLoader.h"
#include "Platform/PlatformGameAPI.h"
#include "CoreDefines.h"
#include "Allocators.h"
#include "Utility/ScopedTimer.h"

#include <string.h>

#ifdef _ASSETS_DIR
#define ASSETS_PATH STRINGIFY(_ASSETS_DIR)
#else
//#define ASSETS_PATH "..\\Assets\\"
#endif

//#define ENABLE_ASSET_STREAMING
#define ENABLE_MULTITHREADED_FILE_LOADING 1

namespace Tk
{
namespace Core
{
namespace Asset
{

static const uint64 AssetFileMemorySize = 1024 * 1024 * 1024;
static Tk::Core::LinearAllocator g_AssetFileScratchMemory;

static uint8* g_ObjFileDataBuffer = nullptr;
static Tk::Core::LinearAllocator g_MeshBufferAllocator; // Persists after meshes are uploaded to the GPU

static uint8* g_TextureFileDataBuffer = nullptr;
static Tk::Core::LinearAllocator g_TextureBufferAllocator; // Dealloc'd after all textures are uploaded to the GPU

static void CreateTextureOnGPU(uint32 uiTexture)
{
    // Textures
    Graphics::ResourceHandle imageStagingBufferHandle = Res_Default();

    // Create texture handles
    Graphics::ResourceDesc desc;
    desc.resourceType = Graphics::ResourceType::eImage2D;
    desc.arrayEles = 1;

    desc.imageFormat = Graphics::ImageFormat::RGBA8_SRGB; // TODO: don't hard code this
    desc.dims = m_allTextureMetadata[uiTexture].m_dims;
    m_allTextureGraphicsHandles[uiTexture] = Graphics::CreateResource(desc);

    uint32 textureSizeInBytes = m_allTextureMetadata[uiTexture].m_dims.x * m_allTextureMetadata[uiTexture].m_dims.y * 4; // 4 bytes per pixel since RGBA8
    desc.dims = v3ui(textureSizeInBytes, 0, 0);
    desc.resourceType = Graphics::ResourceType::eBuffer1D; // staging buffer is just a 1D buffer
    desc.bufferUsage = Graphics::BufferUsage::eStaging;
    imageStagingBufferHandles[uiTexture] = Graphics::CreateResource(desc);
    void* stagingBufferMemPtr = Graphics::MapResource(imageStagingBufferHandles[uiTexture]);

    // Copy texture data into the staging buffer
    uint8* currentTextureFile = g_TextureBufferAllocator.m_ownedMemPtr + accumTextureOffset;
    accumTextureOffset += textureSizeInBytes;
    memcpy(stagingBufferMemPtr, currentTextureFile, textureSizeInBytes);

    // Graphics command to copy from staging buffer to gpu local buffer
    Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    // Create, submit, and execute the buffer copy commands
    uint32 textureSizeInBytes = m_allTextureMetadata[uiTexture].m_dims.x * m_allTextureMetadata[uiTexture].m_dims.y * 4; // 4 bytes per pixel since RGBA8

    // Transition to transfer dst optimal layout
    command->m_commandType = Graphics::GraphicsCmd::eLayoutTransition;
    command->debugLabel = "Transition image layout to transfer dst optimal";
    command->m_imageHandle = m_allTextureGraphicsHandles[uiTexture];
    command->m_startLayout = Graphics::ImageLayout::eUndefined;
    command->m_endLayout = Graphics::ImageLayout::eTransferDst;
    ++command;
    ++graphicsCommandStream->m_numCommands;

    // Texture buffer copy
    command->m_commandType = Graphics::GraphicsCmd::eMemTransfer;
    command->debugLabel = "Update Asset Texture Data";
    command->m_sizeInBytes = textureSizeInBytes;
    command->m_srcBufferHandle = imageStagingBufferHandles[uiTexture];
    command->m_dstBufferHandle = m_allTextureGraphicsHandles[uiTexture];
    ++command;
    ++graphicsCommandStream->m_numCommands;

    // TODO: transition image to shader read optimal?

    // Perform the copies
    Graphics::SubmitCmdsImmediate(graphicsCommandStream);
    graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream

    // Destroy the staging buffer
    Graphics::UnmapResource(imageStagingBufferHandle);
    Graphics::DestroyResource(imageStagingBufferHandle);
}

static void CreateMeshBuffersOnGPU(uint32 uiAsset)
{
    for (uint32 uiAsset = 0; uiAsset < m_numMeshAssets; ++uiAsset)
    {
        // Create buffer handles
        Graphics::ResourceDesc desc;
        desc.resourceType = Core::Graphics::ResourceType::eBuffer1D;

        Graphics::ResourceHandle stagingBufferHandle_Pos, stagingBufferHandle_UV, stagingBufferHandle_Norm, stagingBufferHandle_Idx;
        void* stagingBufferMemPtr_Pos, * stagingBufferMemPtr_UV, * stagingBufferMemPtr_Norm, * stagingBufferMemPtr_Idx;

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
        v2f* uvBuffer = (v2f*)m_allMeshData[uiAsset].m_vertexBufferData_UV;
        v3f* normalBuffer = (v3f*)m_allMeshData[uiAsset].m_vertexBufferData_Normal;
        uint32* indexBuffer = (uint32*)m_allMeshData[uiAsset].m_vertexBufferData_Index;
        memcpy(stagingBufferMemPtr_Pos, positionBuffer, numPositionBytes);
        memcpy(stagingBufferMemPtr_UV, uvBuffer, numUVBytes);
        //memcpy(stagingBufferMemPtr_Norm, normalBuffer, numNormalBytes);
        for (uint32 i = 0; i < m_allMeshData[uiAsset].m_numVertices; ++i)
        {
            memcpy(((uint8*)stagingBufferMemPtr_Norm) + sizeof(v4f) * i, normalBuffer + i, sizeof(v3f));
            memset(((uint8*)stagingBufferMemPtr_Norm) + sizeof(v4f) * i + sizeof(v3f), 0, 1);
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
}

void LoadAllAssets()
{
    // Models
    // TODO: this should come from a file eventually
    const uint32 numMeshAssets = 4;
    const char* meshFilePaths[numMeshAssets] =
    {
        ASSETS_PATH "UnitSphere\\sphere.obj",
        ASSETS_PATH "UnitCube\\cube.obj",
        ASSETS_PATH "FireElemental\\fire_elemental.obj",
        ASSETS_PATH "RTX3090\\rtx3090.obj"
    };

    g_AssetFileScratchMemory.Init(AssetFileMemorySize, CACHE_LINE);
    
    uint32 meshBufferSizes[numMeshAssets];
    memset(meshBufferSizes, 0, ARRAYCOUNT(meshBufferSizes) * sizeof(uint32));

    uint32 totalMeshFileBytes = 0;
    uint32 meshFileSizes[numMeshAssets] = {};
    for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
    {
        uint32 fileSize = Tk::Platform::GetEntireFileSize(meshFilePaths[uiAsset]);
        meshFileSizes[uiAsset] = fileSize;
        totalMeshFileBytes += fileSize;
    }

    // Allocate one large buffer to store a dump of all obj files.
    // Each obj file's data is separated by a single null byte to mark EOF
    g_ObjFileDataBuffer = (uint8*)g_AssetFileScratchMemory.Alloc(totalMeshFileBytes, CACHE_LINE);

    // Now precalculate the size of the vertex attribute buffers needed to contain the obj data
    uint32 totalMeshBufferSize = 0;

    uint32 accumFileOffset = 0;
    {
        TIMED_SCOPED_BLOCK("Load OBJ files from disk - multithreaded");

        if (ENABLE_MULTITHREADED_FILE_LOADING)
        {
            Tk::Platform::WorkerJobList jobs;
            jobs.Init(numMeshAssets);
            for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
            {
                uint8* currentObjFile = objFileDataBuffer + accumFileOffset;
                uint32 currentObjFileSize = meshFileSizes[uiAsset];
                accumFileOffset += currentObjFileSize;

                jobs.m_jobs[uiAsset] = Platform::CreateNewThreadJob([=]()
                    {
                        Tk::Platform::ReadEntireFile(meshFilePaths[uiAsset], currentObjFileSize, currentObjFile);
                    });
            }
            Tk::Platform::EnqueueWorkerThreadJobList_Assisted(&jobs);

            #ifndef ENABLE_ASSET_STREAMING
            jobs.WaitOnJobs();
            #else
            #endif
            jobs.FreeList();
        }
        else
        {
            for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
            {
                uint8* currentObjFile = objFileDataBuffer + accumFileOffset;
                uint32 currentObjFileSize = meshFileSizes[uiAsset];
                accumFileOffset += currentObjFileSize;

                Tk::Platform::ReadEntireFile(meshFilePaths[uiAsset], currentObjFileSize, currentObjFile);
            }
        }
    }

    // TODO: create mesh buffers on gpu, free allocator
    g_MeshBufferAllocator.ExplicitFree();

    // Textures
    const uint32 numTextureAssets = 2;
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
        uint32 fileSize = Tk::Platform::GetEntireFileSize(textureFilePaths[uiAsset]);
        textureFileSizes[uiAsset] = fileSize;
        totalTextureFileBytes += fileSize;
    }

    // Allocate one large buffer to store a dump of all texture files.
    g_TextureFileDataBuffer = (uint8*)g_AssetFileScratchMemory.Alloc(totalTextureFileBytes, CACHE_LINE);

    accumFileOffset = 0;

    if (ENABLE_MULTITHREADED_FILE_LOADING)
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
                    Tk::Platform::ReadEntireFile(textureFilePaths[uiAsset], currentTextureFileSize, currentTextureFile);
                });

            EnqueueWorkerThreadJob(jobs.m_jobs[uiAsset]);
        }
        #ifndef ENABLE_ASSET_STREAMING
        jobs.WaitOnJobs();
        #else
        #endif
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
            Tk::Platform::ReadEntireFile(textureFilePaths[uiTexture], currentTextureFileSize, currentTextureFile);
        }
    }

    // TODO: create textures on GPU
    g_TextureBufferAllocator.ExplicitFree();
}

}
}
}

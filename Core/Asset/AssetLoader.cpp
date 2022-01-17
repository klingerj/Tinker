#include "AssetLoader.h"
#include "Platform/PlatformGameAPI.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "CoreDefines.h"
#include "Allocators.h"
#include "DataStructures/HashMap.h"
#include "Utility/ScopedTimer.h"
#include "Asset/FileParsing.h"
#include "AssetLibrary.h"
#include "DataStructures/Vector.h"

#include <string.h>

#ifdef _ASSETS_DIR
#define ASSETS_PATH STRINGIFY(_ASSETS_DIR)
#else
//#define ASSETS_PATH "..\\Assets\\"
#endif

//#define ENABLE_ASSET_STREAMING
#define ENABLE_MULTITHREADED_FILE_LOADING 0

namespace Tk
{
namespace Core
{
namespace Asset
{

static const uint64 AssetFileMemorySize = 1024 * 1024 * 1024;
static Tk::Core::LinearAllocator g_AssetFileScratchMemory;

static uint8* g_ObjFileDataBuffer = nullptr;
static Tk::Core::HashMap<uint32, Tk::Core::Buffer, Hash32> g_MeshIDToFileBufferMap;

static uint8* g_TextureFileDataBuffer = nullptr;
static Tk::Core::HashMap<uint32, Tk::Core::Buffer, Hash32> g_TextureIDToFileBufferMap;

// Mesh graphics resources
#define TINKER_MAX_MESHES 64
static Tk::Core::Vector<Tk::Core::Graphics::StaticMeshData> g_allStaticMeshGraphicsHandles;
static uint32 g_MeshStreamCounter = 0;

// For storing dumping obj vertex data during parsing
static Tk::Core::Asset::OBJParseScratchBuffers ScratchBuffers;

// For storing final obj vertex data
static Tk::Core::LinearAllocator VertPosAllocator;
static Tk::Core::LinearAllocator VertUVAllocator;
static Tk::Core::LinearAllocator VertNormalAllocator;
static Tk::Core::LinearAllocator VertIndexAllocator;

static const Tk::Core::Buffer* GetMeshFileBuffer(uint32 uiMesh)
{
    uint32 index = g_MeshIDToFileBufferMap.FindIndex(uiMesh);
    if (index == g_MeshIDToFileBufferMap.eInvalidIndex)
        return nullptr;
    else
        return &g_MeshIDToFileBufferMap.DataAtIndex(index);
}

static void CreateVertexBufferDescriptor(uint32 meshID)
{
    Tk::Core::Graphics::StaticMeshData* data = &g_allStaticMeshGraphicsHandles[meshID];
    data->m_descriptor = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_ASSET_VBS);

    Graphics::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    descDataHandles[0].InitInvalid();
    descDataHandles[0].handles[0] = data->m_positionBuffer;
    descDataHandles[0].handles[1] = data->m_uvBuffer;
    descDataHandles[0].handles[2] = data->m_normalBuffer;
    descDataHandles[1].InitInvalid();
    descDataHandles[2].InitInvalid();

    Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_ASSET_VBS, data->m_descriptor, &descDataHandles[0], 1);
}

static void CreateMeshOnGPU(uint32 AssetID,
    const uint8* PosBuffer,
    const uint8* UVBuffer,
    const uint8* NormalBuffer,
    const uint8* IndexBuffer,
    uint32 NumVertices,
    Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    // Create buffer handles
    Graphics::ResourceDesc desc;
    desc.resourceType = Core::Graphics::ResourceType::eBuffer1D;

    Graphics::ResourceHandle stagingBufferHandle_Pos, stagingBufferHandle_UV, stagingBufferHandle_Norm, stagingBufferHandle_Idx;
    void* stagingBufferMemPtr_Pos, * stagingBufferMemPtr_UV, * stagingBufferMemPtr_Norm, * stagingBufferMemPtr_Idx;

    // Positions
    desc.dims = v3ui(NumVertices * sizeof(v4f), 0, 0);
    desc.bufferUsage = Graphics::BufferUsage::eVertex;
    g_allStaticMeshGraphicsHandles[AssetID].m_positionBuffer = Graphics::CreateResource(desc);

    desc.bufferUsage = Graphics::BufferUsage::eStaging;
    stagingBufferHandle_Pos = Graphics::CreateResource(desc);
    stagingBufferMemPtr_Pos = Graphics::MapResource(stagingBufferHandle_Pos);

    // UVs
    desc.dims = v3ui(NumVertices * sizeof(v2f), 0, 0);
    desc.bufferUsage = Graphics::BufferUsage::eVertex;
    g_allStaticMeshGraphicsHandles[AssetID].m_uvBuffer = Graphics::CreateResource(desc);

    desc.bufferUsage = Graphics::BufferUsage::eStaging;
    stagingBufferHandle_UV = Graphics::CreateResource(desc);
    stagingBufferMemPtr_UV = Graphics::MapResource(stagingBufferHandle_UV);

    // Normals
    desc.dims = v3ui(NumVertices * sizeof(v4f), 0, 0);
    desc.bufferUsage = Graphics::BufferUsage::eVertex;
    g_allStaticMeshGraphicsHandles[AssetID].m_normalBuffer = Graphics::CreateResource(desc);

    desc.bufferUsage = Graphics::BufferUsage::eStaging;
    stagingBufferHandle_Norm = Graphics::CreateResource(desc);
    stagingBufferMemPtr_Norm = Graphics::MapResource(stagingBufferHandle_Norm);

    // Indices
    desc.dims = v3ui(NumVertices * sizeof(uint32), 0, 0);
    desc.bufferUsage = Graphics::BufferUsage::eIndex;
    g_allStaticMeshGraphicsHandles[AssetID].m_indexBuffer = Graphics::CreateResource(desc);

    desc.bufferUsage = Graphics::BufferUsage::eStaging;
    stagingBufferHandle_Idx = Graphics::CreateResource(desc);
    stagingBufferMemPtr_Idx = Graphics::MapResource(stagingBufferHandle_Idx);

    g_allStaticMeshGraphicsHandles[AssetID].m_numIndices = NumVertices;

    // Descriptor
    CreateVertexBufferDescriptor(AssetID);

    // Memcpy data into staging buffer
    const uint32 numPositionBytes = NumVertices * sizeof(v4f);
    const uint32 numUVBytes = NumVertices * sizeof(v2f);
    const uint32 numNormalBytes = NumVertices * sizeof(v3f);
    const uint32 numIndexBytes = NumVertices * sizeof(uint32);

    v4f* positionBuffer = (v4f*)PosBuffer;
    v2f* uvBuffer = (v2f*)UVBuffer;
    v3f* normalBuffer = (v3f*)NormalBuffer;
    uint32* indexBuffer = (uint32*)IndexBuffer;
    memcpy(stagingBufferMemPtr_Pos, positionBuffer, numPositionBytes);
    memcpy(stagingBufferMemPtr_UV, uvBuffer, numUVBytes);
    //memcpy(stagingBufferMemPtr_Norm, normalBuffer, numNormalBytes);
    for (uint32 i = 0; i < NumVertices; ++i)
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
        command->m_sizeInBytes = NumVertices * sizeof(v4f);
        command->m_srcBufferHandle = stagingBufferHandle_Pos;
        command->m_dstBufferHandle = g_allStaticMeshGraphicsHandles[AssetID].m_positionBuffer;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // UV buffer copy
        command->m_commandType = Graphics::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Asset Vtx UV Buf";
        command->m_sizeInBytes = NumVertices * sizeof(v2f);
        command->m_srcBufferHandle = stagingBufferHandle_UV;
        command->m_dstBufferHandle = g_allStaticMeshGraphicsHandles[AssetID].m_uvBuffer;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Normal buffer copy
        command->m_commandType = Graphics::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Asset Vtx Norm Buf";
        command->m_sizeInBytes = NumVertices * sizeof(v4f);
        command->m_srcBufferHandle = stagingBufferHandle_Norm;
        command->m_dstBufferHandle = g_allStaticMeshGraphicsHandles[AssetID].m_normalBuffer;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Index buffer copy
        command->m_commandType = Graphics::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Asset Vtx Idx Buf";
        command->m_sizeInBytes = NumVertices * sizeof(uint32);
        command->m_srcBufferHandle = stagingBufferHandle_Idx;
        command->m_dstBufferHandle = g_allStaticMeshGraphicsHandles[AssetID].m_indexBuffer;
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

static void ParseAllMeshes(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream, uint32 NumMeshAssets)
{
    {
        TIMED_SCOPED_BLOCK("Parse OBJ Files - single threaded");

        const uint32 MAX_VERT_BUFFER_SIZE = 1024 * 1024 * 128;
        VertPosAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
        VertUVAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
        VertNormalAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
        VertIndexAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);

        ScratchBuffers = {};
        ScratchBuffers.VertPosAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
        ScratchBuffers.VertUVAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
        ScratchBuffers.VertNormalAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);

        uint32 accumNumVerts = 0;
        for (uint32 uiAsset = 0; uiAsset < NumMeshAssets; ++uiAsset)
        {
            const Tk::Core::Buffer* FileBuffer = Tk::Core::Asset::GetMeshFileBuffer(uiAsset);

            uint32 numObjVerts = 0;
            Tk::Core::Asset::ParseOBJ(VertPosAllocator, VertUVAllocator, VertNormalAllocator, VertIndexAllocator, ScratchBuffers, FileBuffer->m_data, FileBuffer->m_sizeInBytes, &numObjVerts);

            CreateMeshOnGPU(uiAsset,
                VertPosAllocator.m_ownedMemPtr + sizeof(v4f) * accumNumVerts,
                VertUVAllocator.m_ownedMemPtr + sizeof(v2f) * accumNumVerts,
                VertNormalAllocator.m_ownedMemPtr + sizeof(v3f) * accumNumVerts,
                VertIndexAllocator.m_ownedMemPtr + sizeof(uint32) * accumNumVerts,
                numObjVerts,
                graphicsCommandStream);

            ++g_MeshStreamCounter;

            ScratchBuffers.ResetState();
            accumNumVerts += numObjVerts;
        }
    }
}

void LoadAllAssets(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream)
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
    g_allStaticMeshGraphicsHandles.Resize(TINKER_MAX_MESHES);
    g_MeshIDToFileBufferMap.Reserve(64);
    g_TextureIDToFileBufferMap.Reserve(64);
    
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

    totalMeshFileBytes = 0;
    for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
    {
        uint32 fileSize = meshFileSizes[uiAsset];

        Buffer buffer;
        buffer.m_data = g_ObjFileDataBuffer + totalMeshFileBytes;
        buffer.m_sizeInBytes = fileSize;
        g_MeshIDToFileBufferMap.Insert(uiAsset, buffer);

        totalMeshFileBytes += fileSize;
    }
    
    uint32 accumFileOffset = 0;
    {
        TIMED_SCOPED_BLOCK("Load OBJ files from disk - multithreaded");

        if (ENABLE_MULTITHREADED_FILE_LOADING)
        {
            Tk::Platform::WorkerJobList jobs;
            jobs.Init(numMeshAssets);
            for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
            {
                uint8* currentObjFile = g_ObjFileDataBuffer + accumFileOffset;
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
                uint8* currentObjFile = g_ObjFileDataBuffer + accumFileOffset;
                uint32 currentObjFileSize = meshFileSizes[uiAsset];
                accumFileOffset += currentObjFileSize;

                Tk::Platform::ReadEntireFile(meshFilePaths[uiAsset], currentObjFileSize, currentObjFile);
            }
        }
    }

    ParseAllMeshes(graphicsCommandStream, numMeshAssets);

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

    totalTextureFileBytes = 0;
    for (uint32 uiAsset = 0; uiAsset < numTextureAssets; ++uiAsset)
    {
        uint32 fileSize = textureFileSizes[uiAsset];

        Buffer buffer;
        buffer.m_data = g_TextureFileDataBuffer + totalTextureFileBytes;
        buffer.m_sizeInBytes = fileSize;
        g_TextureIDToFileBufferMap.Insert(uiAsset, buffer);

        totalMeshFileBytes += fileSize;
    }

    accumFileOffset = 0;

    if (ENABLE_MULTITHREADED_FILE_LOADING)
    {
        Platform::WorkerJobList jobs;
        jobs.Init(numTextureAssets);
        for (uint32 uiAsset = 0; uiAsset < numTextureAssets; ++uiAsset)
        {
            uint8* currentTextureFile = g_TextureFileDataBuffer + accumFileOffset;
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
            uint8* currentTextureFile = g_TextureFileDataBuffer + accumFileOffset;
            uint32 currentTextureFileSize = textureFileSizes[uiAsset];
            accumFileOffset += currentTextureFileSize;

            // Read each file into the buffer
            Tk::Platform::ReadEntireFile(textureFilePaths[uiAsset], currentTextureFileSize, currentTextureFile);
        }
    }
}

void AddStreamedAssetsToAssetLibrary(AssetLibrary* AssetLib, uint32* NumAssetsStreamed)
{
    //TODO: make threadsafe

    uint32 NumAssetsStreamedSoFar = g_MeshStreamCounter;
    for (uint32 uiMesh = *NumAssetsStreamed; uiMesh < NumAssetsStreamedSoFar; ++uiMesh)
    {
        AssetLib->m_MeshLib.Insert(uiMesh, &g_allStaticMeshGraphicsHandles[uiMesh]);
    }
    *NumAssetsStreamed = NumAssetsStreamedSoFar;
}

}
}
}

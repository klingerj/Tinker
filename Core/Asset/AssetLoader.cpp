#include "AssetLoader.h"
#include "Platform/PlatformGameAPI.h"
#include "CoreDefines.h"
#include "Allocators.h"
#include "DataStructures/HashMap.h"
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
static Tk::Core::HashMap<uint32, Tk::Core::Buffer, Hash32> g_MeshIDToFileBufferMap;

static uint8* g_TextureFileDataBuffer = nullptr;
static Tk::Core::HashMap<uint32, Tk::Core::Buffer, Hash32> g_TextureIDToFileBufferMap;

inline const Tk::Core::Buffer* GetMeshFileBuffer(uint32 uiMesh)
{
    uint32 index = g_MeshIDToFileBufferMap.FindIndex(uiMesh);
    if (index == g_MeshIDToFileBufferMap.eInvalidIndex)
        return nullptr;
    else
        return &g_MeshIDToFileBufferMap.DataAtIndex(index);
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

}
}
}

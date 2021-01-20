#pragma once

#include "GameGraphicsTypes.h"
#include "../Include/Core/FileIO/FileLoading.h"
#include "../Include/Core/Allocators.h"

using namespace Tinker;
using namespace Core;
using namespace Math;

#define TINKER_MAX_MESHES 16
#define TINKER_MAX_TEXTURES 16

// TODO: probably heap-allocate all these arrays once things get large enough

class AssetManager
{
private:
    // Mesh Buffer data
    Memory::LinearAllocator<> m_meshBufferAllocator; // Persists after meshes are uploaded to the GPU
    MeshAttributeData m_allMeshData[TINKER_MAX_MESHES];

    // Mesh graphics resources
    StaticMeshData m_allStaticMeshGraphicsHandles[TINKER_MAX_MESHES];
    
    // Texture data
    Memory::LinearAllocator<> m_textureBufferAllocator; // Dealloc'd after all textures are uploaded to the GPU
    TextureMetadata m_allTextureMetadata[TINKER_MAX_TEXTURES];
    ResourceHandle m_allTextureGraphicsHandles[TINKER_MAX_TEXTURES];

public:
    uint32 m_numMeshAssets = 0;
    uint32 m_numTextureAssets = 0;

    AssetManager() {}
    ~AssetManager() {}

    void FreeMemory();

    void LoadAllAssets(const Tinker::Platform::PlatformAPIFuncs* platformFuncs);
    void InitAssetGraphicsResources(const Tinker::Platform::PlatformAPIFuncs* platformFuncs,
        Tinker::Platform::GraphicsCommandStream* graphicsCommandStream);
    // TODO: declare a mapping of ID to each asset file
    StaticMeshData* GetMeshGraphicsDataByID(uint32 meshID);
    ResourceHandle* GetTextureGraphicsDataByID(uint32 textureID);
};

extern AssetManager g_AssetManager;

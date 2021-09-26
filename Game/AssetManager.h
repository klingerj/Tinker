#pragma once

#include "GraphicsTypes.h"
#include "Core/Allocators.h"

#ifdef _ASSETS_DIR
#define ASSETS_PATH STRINGIFY(_ASSETS_DIR)
#else
//#define ASSETS_PATH "..\\Assets\\"
#endif

#define TINKER_MAX_MESHES 64
#define TINKER_MAX_TEXTURES 64

// TODO: probably heap-allocate all these arrays once things get large enough

class AssetManager
{
private:
    // Mesh Buffer data
    Tk::Memory::LinearAllocator<> m_meshBufferAllocator; // Persists after meshes are uploaded to the GPU
    MeshAttributeData m_allMeshData[TINKER_MAX_MESHES];

    // Mesh graphics resources
    StaticMeshData m_allStaticMeshGraphicsHandles[TINKER_MAX_MESHES];
    
    // Texture data
    Tk::Memory::LinearAllocator<> m_textureBufferAllocator; // Dealloc'd after all textures are uploaded to the GPU
    TextureMetadata m_allTextureMetadata[TINKER_MAX_TEXTURES];
    Tk::Platform::Graphics::ResourceHandle m_allTextureGraphicsHandles[TINKER_MAX_TEXTURES];

    void CreateVertexBufferDescriptor(uint32 meshID);

public:
    uint32 m_numMeshAssets = 0;
    uint32 m_numTextureAssets = 0;

    AssetManager() {}
    ~AssetManager() {}

    void FreeMemory();

    void LoadAllAssets(const Tk::Platform::PlatformAPIFuncs* platformFuncs);
    void InitAssetGraphicsResources(Tk::Platform::Graphics::GraphicsCommandStream* graphicsCommandStream);
    void DestroyAllMeshData();
    void DestroyAllTextureData();
    // TODO: declare a mapping of ID to each asset file
    StaticMeshData* GetMeshGraphicsDataByID(uint32 meshID);
    Tk::Platform::Graphics::ResourceHandle GetTextureGraphicsDataByID(uint32 textureID) const;
    const MeshAttributeData& GetMeshAttrDataByID(uint32 meshID) const;
};

extern AssetManager g_AssetManager;

#pragma once

#include "GraphicsTypes.h"

#define TINKER_MAX_MESHES 64
#define TINKER_MAX_TEXTURES 64

struct AssetManager
{
private:
    // Mesh Buffer data
    MeshAttributeData m_allMeshData[TINKER_MAX_MESHES];

    // Mesh graphics resources
    StaticMeshData m_allStaticMeshGraphicsHandles[TINKER_MAX_MESHES];
    
    // Texture data
    TextureMetadata m_allTextureMetadata[TINKER_MAX_TEXTURES];
    Tk::Core::Graphics::ResourceHandle m_allTextureGraphicsHandles[TINKER_MAX_TEXTURES];

    void CreateVertexBufferDescriptor(uint32 meshID);

public:
    uint32 m_numMeshAssets = 0;
    uint32 m_numTextureAssets = 0;

    AssetManager() {}
    ~AssetManager() {}

    void FreeMemory();

    void LoadAllAssets();
    void InitAssetGraphicsResources(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream);
    void DestroyAllMeshData();
    void DestroyAllTextureData();
    // TODO: declare a mapping of ID to each asset file
    StaticMeshData* GetMeshGraphicsDataByID(uint32 meshID);
    Tk::Core::Graphics::ResourceHandle GetTextureGraphicsDataByID(uint32 textureID) const;
    const MeshAttributeData& GetMeshAttrDataByID(uint32 meshID) const;
};

extern AssetManager g_AssetManager;

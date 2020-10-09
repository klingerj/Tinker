#pragma once

#include "GameGraphicsTypes.h"
#include "../Include/Core/FileIO/FileLoading.h"
#include "../Include/Core/Allocators.h"

using namespace Tinker;
using namespace Core;
using namespace Math;

#define TINKER_MAX_ASSETS 16

class AssetManager
{
private:
    // Mesh Buffer data
    Memory::LinearAllocator<> m_meshBufferAllocator;
    MeshAttributeData m_allMeshData[TINKER_MAX_ASSETS];

    // Mesh graphics resources
    DynamicMeshData m_allMeshGraphicsHandles[TINKER_MAX_ASSETS];

public:
    uint32 m_numMeshAssets = 0;

    AssetManager() {}
    ~AssetManager() {}

    void LoadAllAssets(const Tinker::Platform::PlatformAPIFuncs* platformFuncs);
    void InitAssetGraphicsResources(const Tinker::Platform::PlatformAPIFuncs* platformFuncs);
    // TODO: declare a mapping of ID to each asset file
    DynamicMeshData* GetMeshGraphicsDataByID(uint32 meshID);
};

extern AssetManager g_AssetManager;

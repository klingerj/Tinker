#include "AssetManager.h"

AssetManager g_AssetManager;


void AssetManager::LoadAllAssets(const Tinker::Platform::PlatformAPIFuncs* platformFuncs)
{
    const uint32 numMeshAssets = 1;
    TINKER_ASSERT(numMeshAssets <= TINKER_MAX_ASSETS);

    m_numMeshAssets = numMeshAssets;

    const char* meshFilePaths[numMeshAssets] =
    {
        //"..\\Assets\\UnitSphere\\sphere.obj",
        //"..\\Assets\\UnitCube\\cube.obj",
        "..\\Assets\\FireElemental\\fire_elemental.obj"
        //"..\\Assets\\RTX3090\\rtx3090.obj"
    };

    uint32 totalMeshFileBytes = 0;
    uint32 meshFileSizes[numMeshAssets] = {};
    for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
    {
        uint32 fileSize = platformFuncs->GetFileSize(meshFilePaths[uiAsset]); // + 1 byte for manual EOF byte
        meshFileSizes[uiAsset] = fileSize;
        totalMeshFileBytes += fileSize;
    }

    // Allocate one large buffer to store a dump of all obj files.
    // Each obj file's data is separated by a single null byte to mark EOF
    uint8* objFileDataBuffer = new uint8[totalMeshFileBytes + numMeshAssets]; // + 1 EOF byte for each obj file

    // Now precalculate the size of the vertex attribute buffers needed to contain the obj data
    uint32 meshBufferSizes[numMeshAssets] = {};
    uint32 totalMeshBufferSize = 0;

    uint32 accumFileOffset = 0;
    // Calculate the size of each mesh buffer - positions, normals, uvs, indices
    for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
    {
        uint8* currentObjFile = objFileDataBuffer + accumFileOffset;
        uint32 currentObjFileSize = meshFileSizes[uiAsset];
        accumFileOffset += currentObjFileSize + 1; // Account for manual EOF byte

        currentObjFile[currentObjFileSize] = '\0'; // Mark EOF
        platformFuncs->ReadEntireFile(meshFilePaths[uiAsset], currentObjFileSize, currentObjFile);

        uint32 numObjVerts = FileLoading::GetOBJVertCount(currentObjFile, currentObjFileSize);
        TINKER_ASSERT(numObjVerts > 0);

        m_allMeshData[uiAsset].m_numVertices = numObjVerts;
        uint32 numPositionBytes  = numObjVerts * sizeof(v4f);
        uint32 numNormalBytes    = numObjVerts * sizeof(v3f);
        uint32 numUVBytes        = numObjVerts * sizeof(v2f);
        uint32 numIndexBytes     = numObjVerts * sizeof(uint32);
        meshBufferSizes[uiAsset] = numPositionBytes + numNormalBytes + numUVBytes + numIndexBytes;
        totalMeshBufferSize += meshBufferSizes[uiAsset];
    }

    // Allocate exactly enough space for each mesh's vertex buffers, cache-line aligned
    m_meshBufferAllocator.Init(totalMeshBufferSize, 64);

    // Parse each OBJ and populate each mesh's buffer
    accumFileOffset = 0;
    for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
    {
        m_allMeshData[uiAsset].m_vertexBufferData = m_meshBufferAllocator.Alloc(meshBufferSizes[uiAsset], 1);

        uint32 numPositionBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v4f);
        uint32 numUVBytes       = m_allMeshData[uiAsset].m_numVertices * sizeof(v2f);
        uint32 numNormalBytes   = m_allMeshData[uiAsset].m_numVertices * sizeof(v3f);
        uint32 numIndexBytes    = m_allMeshData[uiAsset].m_numVertices * sizeof(uint32);

        v4f* positionBuffer = (v4f*) m_allMeshData[uiAsset].m_vertexBufferData;
        v2f* uvBuffer       = (v2f*)((uint8*)positionBuffer + numPositionBytes);
        v3f* normalBuffer   = (v3f*)((uint8*)uvBuffer + numUVBytes);
        uint32* indexBuffer = (uint32*)((uint8*)normalBuffer + numNormalBytes);

        uint8* currentObjFile = objFileDataBuffer + accumFileOffset;
        uint32 currentObjFileSize = meshFileSizes[uiAsset];
        accumFileOffset += currentObjFileSize + 1; // Account for manual EOF byte
        FileLoading::ParseOBJ(positionBuffer, uvBuffer, normalBuffer, indexBuffer, currentObjFile, currentObjFileSize);
    }

    delete objFileDataBuffer;
}

void AssetManager::InitAssetGraphicsResources(const Tinker::Platform::PlatformAPIFuncs* platformFuncs)
{
    for (uint32 uiAsset = 0; uiAsset < m_numMeshAssets; ++uiAsset)
    {
        // Create buffer handles
        m_allMeshGraphicsHandles[uiAsset].m_positionBuffer.gpuBufferHandle = platformFuncs->CreateBuffer(m_allMeshData[uiAsset].m_numVertices * sizeof(v4f), Platform::eBufferUsageVertex).handle;
        Platform::BufferData data = platformFuncs->CreateBuffer(m_allMeshData[uiAsset].m_numVertices * sizeof(v4f), Platform::eBufferUsageStaging);
        m_allMeshGraphicsHandles[uiAsset].m_positionBuffer.stagingBufferHandle = data.handle;
        m_allMeshGraphicsHandles[uiAsset].m_positionBuffer.stagingBufferMemPtr = data.memory;

        m_allMeshGraphicsHandles[uiAsset].m_uvBuffer.gpuBufferHandle = platformFuncs->CreateBuffer(m_allMeshData[uiAsset].m_numVertices * sizeof(v2f), Platform::eBufferUsageVertex).handle;
        data = platformFuncs->CreateBuffer(m_allMeshData[uiAsset].m_numVertices * sizeof(v2f), Platform::eBufferUsageStaging);
        m_allMeshGraphicsHandles[uiAsset].m_uvBuffer.stagingBufferHandle = data.handle;
        m_allMeshGraphicsHandles[uiAsset].m_uvBuffer.stagingBufferMemPtr = data.memory;

        m_allMeshGraphicsHandles[uiAsset].m_normalBuffer.gpuBufferHandle = platformFuncs->CreateBuffer(m_allMeshData[uiAsset].m_numVertices * sizeof(v3f), Platform::eBufferUsageVertex).handle;
        data = platformFuncs->CreateBuffer(m_allMeshData[uiAsset].m_numVertices * sizeof(v3f), Platform::eBufferUsageStaging);
        m_allMeshGraphicsHandles[uiAsset].m_normalBuffer.stagingBufferHandle = data.handle;
        m_allMeshGraphicsHandles[uiAsset].m_normalBuffer.stagingBufferMemPtr = data.memory;

        m_allMeshGraphicsHandles[uiAsset].m_indexBuffer.gpuBufferHandle = platformFuncs->CreateBuffer(m_allMeshData[uiAsset].m_numVertices * sizeof(uint32), Platform::eBufferUsageIndex).handle;
        data = platformFuncs->CreateBuffer(m_allMeshData[uiAsset].m_numVertices * sizeof(uint32), Platform::eBufferUsageStaging);
        m_allMeshGraphicsHandles[uiAsset].m_indexBuffer.stagingBufferHandle = data.handle;
        m_allMeshGraphicsHandles[uiAsset].m_indexBuffer.stagingBufferMemPtr = data.memory;

        m_allMeshGraphicsHandles[uiAsset].m_numIndices = m_allMeshData[uiAsset].m_numVertices;

        // Memcpy data into staging buffer
        uint32 numPositionBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v4f);
        uint32 numUVBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v2f);
        uint32 numNormalBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v3f);
        uint32 numIndexBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(uint32);

        v4f* positionBuffer = (v4f*)m_allMeshData[uiAsset].m_vertexBufferData;
        v2f* uvBuffer       = (v2f*)((uint8*)positionBuffer + numPositionBytes);
        v3f* normalBuffer   = (v3f*)((uint8*)uvBuffer + numUVBytes);
        uint32* indexBuffer = (uint32*)((uint8*)normalBuffer + numNormalBytes);
        memcpy(m_allMeshGraphicsHandles[uiAsset].m_positionBuffer.stagingBufferMemPtr, positionBuffer, numPositionBytes);
        memcpy(m_allMeshGraphicsHandles[uiAsset].m_uvBuffer.stagingBufferMemPtr, uvBuffer, numNormalBytes);
        memcpy(m_allMeshGraphicsHandles[uiAsset].m_normalBuffer.stagingBufferMemPtr, normalBuffer, numNormalBytes);
        memcpy(m_allMeshGraphicsHandles[uiAsset].m_indexBuffer.stagingBufferMemPtr, indexBuffer, numIndexBytes);
    }
}

DynamicMeshData* AssetManager::GetMeshGraphicsDataByID(uint32 meshID)
{
    TINKER_ASSERT(meshID != TINKER_INVALID_HANDLE);
    TINKER_ASSERT(meshID < TINKER_MAX_ASSETS);
    TINKER_ASSERT(meshID >= 0);
    return &m_allMeshGraphicsHandles[meshID];
}

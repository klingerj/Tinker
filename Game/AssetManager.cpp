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

void AssetManager::InitAssetGraphicsResources(const Tinker::Platform::PlatformAPIFuncs* platformFuncs,
    Tinker::Platform::GraphicsCommandStream* graphicsCommandStream)
{
    for (uint32 uiAsset = 0; uiAsset < m_numMeshAssets; ++uiAsset)
    {
        // Create buffer handles
        ResourceDesc desc;
        desc.resourceType = Platform::eResourceTypeBuffer1D;

        ResourceHandle stagingBufferHandle_Pos, stagingBufferHandle_UV, stagingBufferHandle_Norm, stagingBufferHandle_Idx;
        void* stagingBufferMemPtr_Pos, *stagingBufferMemPtr_UV, *stagingBufferMemPtr_Norm, *stagingBufferMemPtr_Idx;

        // Positions
        desc.dims = v3ui(m_allMeshData[uiAsset].m_numVertices * sizeof(v4f), 0, 0);
        desc.bufferUsage = Platform::eBufferUsageVertex;
        m_allStaticMeshGraphicsHandles[uiAsset].m_positionBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::eBufferUsageStaging;
        stagingBufferHandle_Pos = platformFuncs->CreateResource(desc);
        stagingBufferMemPtr_Pos = platformFuncs->MapResource(stagingBufferHandle_Pos);

        // UVs
        desc.dims = v3ui(m_allMeshData[uiAsset].m_numVertices * sizeof(v2f), 0, 0);
        desc.bufferUsage = Platform::eBufferUsageVertex;
        m_allStaticMeshGraphicsHandles[uiAsset].m_uvBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);
        
        desc.bufferUsage = Platform::eBufferUsageStaging;
        stagingBufferHandle_UV = platformFuncs->CreateResource(desc);
        stagingBufferMemPtr_UV = platformFuncs->MapResource(stagingBufferHandle_UV);

        // Normals
        desc.dims = v3ui(m_allMeshData[uiAsset].m_numVertices * sizeof(v3f), 0, 0);
        desc.bufferUsage = Platform::eBufferUsageVertex;
        m_allStaticMeshGraphicsHandles[uiAsset].m_normalBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);
        
        desc.bufferUsage = Platform::eBufferUsageStaging;
        stagingBufferHandle_Norm = platformFuncs->CreateResource(desc);
        stagingBufferMemPtr_Norm = platformFuncs->MapResource(stagingBufferHandle_Norm);

        // Indices
        desc.dims = v3ui(m_allMeshData[uiAsset].m_numVertices * sizeof(uint32), 0, 0);
        desc.bufferUsage = Platform::eBufferUsageIndex;
        m_allStaticMeshGraphicsHandles[uiAsset].m_indexBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);
        
        desc.bufferUsage = Platform::eBufferUsageStaging;
        stagingBufferHandle_Idx = platformFuncs->CreateResource(desc);
        stagingBufferMemPtr_Idx = platformFuncs->MapResource(stagingBufferHandle_Idx);

        m_allStaticMeshGraphicsHandles[uiAsset].m_numIndices = m_allMeshData[uiAsset].m_numVertices;
        //-----

        // Memcpy data into staging buffer
        uint32 numPositionBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v4f);
        uint32 numUVBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v2f);
        uint32 numNormalBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v3f);
        uint32 numIndexBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(uint32);

        v4f* positionBuffer = (v4f*)m_allMeshData[uiAsset].m_vertexBufferData;
        v2f* uvBuffer       = (v2f*)((uint8*)positionBuffer + numPositionBytes);
        v3f* normalBuffer   = (v3f*)((uint8*)uvBuffer + numUVBytes);
        uint32* indexBuffer = (uint32*)((uint8*)normalBuffer + numNormalBytes);
        memcpy(stagingBufferMemPtr_Pos, positionBuffer, numPositionBytes);
        memcpy(stagingBufferMemPtr_UV, uvBuffer, numUVBytes);
        memcpy(stagingBufferMemPtr_Norm, normalBuffer, numNormalBytes);
        memcpy(stagingBufferMemPtr_Idx, indexBuffer, numIndexBytes);
        //-----

        // Create, submit, and execute the buffer copy commands
        {
            // Graphics command to copy from staging buffer to gpu local buffer
            Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

            // Position buffer copy
            command->m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;
            command->debugLabel = "Update Asset Vtx Pos Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v4f);
            command->m_srcBufferHandle = stagingBufferHandle_Pos;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_positionBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // UV buffer copy
            command->m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;
            command->debugLabel = "Update Asset Vtx UV Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v2f);
            command->m_srcBufferHandle = stagingBufferHandle_UV;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_uvBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // Normal buffer copy
            command->m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;
            command->debugLabel = "Update Asset Vtx Norm Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v3f);
            command->m_srcBufferHandle = stagingBufferHandle_Norm;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_normalBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // Index buffer copy
            command->m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;
            command->debugLabel = "Update Asset Vtx Idx Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(uint32);
            command->m_srcBufferHandle = stagingBufferHandle_Idx;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_indexBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // Perform the copies
            platformFuncs->SubmitCmdsImmediate(graphicsCommandStream);
            graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream
        }

        // Unmap the buffer resource
        platformFuncs->UnmapResource(stagingBufferHandle_Pos);
        platformFuncs->UnmapResource(stagingBufferHandle_UV);
        platformFuncs->UnmapResource(stagingBufferHandle_Norm);
        platformFuncs->UnmapResource(stagingBufferHandle_Idx);

        // Destroy the staging buffer resources
        platformFuncs->DestroyResource(stagingBufferHandle_Pos);
        platformFuncs->DestroyResource(stagingBufferHandle_UV);
        platformFuncs->DestroyResource(stagingBufferHandle_Norm);
        platformFuncs->DestroyResource(stagingBufferHandle_Idx);
    }
}

StaticMeshData* AssetManager::GetMeshGraphicsDataByID(uint32 meshID)
{
    TINKER_ASSERT(meshID != TINKER_INVALID_HANDLE);
    TINKER_ASSERT(meshID < TINKER_MAX_ASSETS);
    TINKER_ASSERT(meshID >= 0);
    return &m_allStaticMeshGraphicsHandles[meshID];
}

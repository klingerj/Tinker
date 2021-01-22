#include "AssetManager.h"

AssetManager g_AssetManager;

void AssetManager::FreeMemory()
{
    m_meshBufferAllocator.ExplicitFree();
}

void AssetManager::LoadAllAssets(const Tinker::Platform::PlatformAPIFuncs* platformFuncs)
{
    // Meshes
    const uint32 numMeshAssets = 4;
    TINKER_ASSERT(numMeshAssets <= TINKER_MAX_MESHES);

    m_numMeshAssets = numMeshAssets;

    const char* meshFilePaths[numMeshAssets] =
    {
        "..\\Assets\\UnitSphere\\sphere.obj",
        "..\\Assets\\UnitCube\\cube.obj",
        "..\\Assets\\FireElemental\\fire_elemental.obj",
        "..\\Assets\\RTX3090\\rtx3090.obj"
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

    bool multithreadObjLoading = true;
    if (multithreadObjLoading)
    {
        Platform::WorkerJob* jobs[numMeshAssets] = {};
        for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
        {
            uint8* currentObjFile = objFileDataBuffer + accumFileOffset;
            uint32 currentObjFileSize = meshFileSizes[uiAsset];
            accumFileOffset += currentObjFileSize + 1; // Account for manual EOF byte

            jobs[uiAsset] = Platform::CreateNewThreadJob([=]()
                {
                    platformFuncs->ReadEntireFile(meshFilePaths[uiAsset], currentObjFileSize, currentObjFile);
                    currentObjFile[currentObjFileSize] = '\0'; // Mark EOF
                });

            platformFuncs->EnqueueWorkerThreadJob(jobs[uiAsset]);
        }
        for (uint32 i = 0; i < numMeshAssets; ++i)
        {
            platformFuncs->WaitOnThreadJob(jobs[i]);
        }
        for (uint32 i = 0; i < numMeshAssets; ++i)
        {
            delete jobs[i];
        }
    }
    else
    {
        for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
        {
            uint8* currentObjFile = objFileDataBuffer + accumFileOffset;
            uint32 currentObjFileSize = meshFileSizes[uiAsset];
            accumFileOffset += currentObjFileSize + 1; // Account for manual EOF byte

            currentObjFile[currentObjFileSize] = '\0'; // Mark EOF
            platformFuncs->ReadEntireFile(meshFilePaths[uiAsset], currentObjFileSize, currentObjFile);
        }
    }

    accumFileOffset = 0;
    // Calculate the size of each mesh buffer - positions, normals, uvs, indices
    for (uint32 uiAsset = 0; uiAsset < numMeshAssets; ++uiAsset)
    {
        uint8* currentObjFile = objFileDataBuffer + accumFileOffset;
        uint32 currentObjFileSize = meshFileSizes[uiAsset];
        accumFileOffset += currentObjFileSize + 1; // Account for manual EOF byte

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

    accumFileOffset = 0;
    // Parse each OBJ and populate each mesh's buffer
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
    //-----

    // Textures
    const uint32 numTextureAssets = 2;
    TINKER_ASSERT(numTextureAssets <= TINKER_MAX_TEXTURES);

    m_numTextureAssets = numTextureAssets;

    const char* textureFilePaths[numTextureAssets] =
    {
        "..\\Assets\\checkerboard512.bmp",
        "..\\Assets\\checkerboardRGB512.bmp"
    };
    // Compute the actual size of the texture data to be allocated and copied to the GPU
    // TODO: get file extension from string
    const char* fileExt = "bmp";

    uint32 totalTextureFileBytes = 0;
    uint32 textureFileSizes[numTextureAssets] = {};
    for (uint32 uiAsset = 0; uiAsset < numTextureAssets; ++uiAsset)
    {
        uint32 fileSize = platformFuncs->GetFileSize(textureFilePaths[uiAsset]);
        textureFileSizes[uiAsset] = fileSize;
        totalTextureFileBytes += fileSize;
    }

    // Allocate one large buffer to store a dump of all texture files.
    uint8* textureFileDataBuffer = new uint8[totalTextureFileBytes];

    accumFileOffset = 0;

    bool multithreadTextureLoading = true;
    if (multithreadTextureLoading)
    {
        Platform::WorkerJob* jobs[numTextureAssets] = {};
        for (uint32 uiAsset = 0; uiAsset < numTextureAssets; ++uiAsset)
        {
            uint8* currentTextureFile = textureFileDataBuffer + accumFileOffset;
            uint32 currentTextureFileSize = textureFileSizes[uiAsset];
            accumFileOffset += currentTextureFileSize;

            jobs[uiAsset] = Platform::CreateNewThreadJob([=]()
                {
                    platformFuncs->ReadEntireFile(textureFilePaths[uiAsset], currentTextureFileSize, currentTextureFile);
                });

            platformFuncs->EnqueueWorkerThreadJob(jobs[uiAsset]);
        }
        for (uint32 i = 0; i < numTextureAssets; ++i)
        {
            platformFuncs->WaitOnThreadJob(jobs[i]);
        }
        for (uint32 i = 0; i < numTextureAssets; ++i)
        {
            delete jobs[i];
        }
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
            platformFuncs->ReadEntireFile(textureFilePaths[uiAsset], currentTextureFileSize, currentTextureFile);
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
            FileLoading::BMPInfo info = FileLoading::GetBMPInfo(currentTextureFile);

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
    m_textureBufferAllocator.Init(totalActualTextureSize, 64);

    accumFileOffset = 0;
    for (uint32 uiAsset = 0; uiAsset < numTextureAssets; ++uiAsset)
    {
        uint8* currentTextureFile = textureFileDataBuffer + accumFileOffset;
        uint32 currentTextureFileSize = textureFileSizes[uiAsset];
        accumFileOffset += currentTextureFileSize;

        uint8* textureBuffer = m_textureBufferAllocator.Alloc(actualTextureSizes[uiAsset], 1);

        if (strncmp(fileExt, "bmp", 3) == 0)
        {
            uint8* textureBytesStart = currentTextureFile + sizeof(FileLoading::BMPHeader) + sizeof(FileLoading::BMPInfo);

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

    delete textureFileDataBuffer;
}

void AssetManager::InitAssetGraphicsResources(const Tinker::Platform::PlatformAPIFuncs* platformFuncs,
    Tinker::Platform::GraphicsCommandStream* graphicsCommandStream)
{
    // Meshes
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
            command->m_commandType = Platform::eGraphicsCmdMemTransfer;
            command->debugLabel = "Update Asset Vtx Pos Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v4f);
            command->m_srcBufferHandle = stagingBufferHandle_Pos;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_positionBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // UV buffer copy
            command->m_commandType = Platform::eGraphicsCmdMemTransfer;
            command->debugLabel = "Update Asset Vtx UV Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v2f);
            command->m_srcBufferHandle = stagingBufferHandle_UV;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_uvBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // Normal buffer copy
            command->m_commandType = Platform::eGraphicsCmdMemTransfer;
            command->debugLabel = "Update Asset Vtx Norm Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v3f);
            command->m_srcBufferHandle = stagingBufferHandle_Norm;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_normalBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // Index buffer copy
            command->m_commandType = Platform::eGraphicsCmdMemTransfer;
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

    // Textures
    ResourceHandle imageStagingBufferHandles[TINKER_MAX_TEXTURES] = {};

    uint32 accumTextureOffset = 0;
    for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
    {
        // Create texture handles
        ResourceDesc desc;
        desc.resourceType = Platform::eResourceTypeImage2D;

        desc.imageFormat = Platform::eImageFormat_RGBA8_SRGB; // TODO: don't hard code this
        desc.dims = m_allTextureMetadata[uiAsset].m_dims;
        m_allTextureGraphicsHandles[uiAsset] = platformFuncs->CreateResource(desc);

        uint32 textureSizeInBytes = m_allTextureMetadata[uiAsset].m_dims.x * m_allTextureMetadata[uiAsset].m_dims.y * 4; // 4 bytes per pixel since RGBA8
        desc.dims = v3ui(textureSizeInBytes, 0, 0);
        desc.resourceType = Platform::eResourceTypeBuffer1D; // staging buffer is just a 1D buffer
        desc.bufferUsage = Platform::eBufferUsageStaging;
        imageStagingBufferHandles[uiAsset] = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr = platformFuncs->MapResource(imageStagingBufferHandles[uiAsset]);

        // Copy texture data into the staging buffer
        uint8* currentTextureFile = m_textureBufferAllocator.m_ownedMemPtr + accumTextureOffset;
        accumTextureOffset += textureSizeInBytes;
        memcpy(stagingBufferMemPtr, currentTextureFile, textureSizeInBytes);
    }

    // Graphics command to copy from staging buffer to gpu local buffer
    Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    // Create, submit, and execute the buffer copy commands
    for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
    {
        uint32 textureSizeInBytes = m_allTextureMetadata[uiAsset].m_dims.x * m_allTextureMetadata[uiAsset].m_dims.y * 4; // 4 bytes per pixel since RGBA8

        // Transition to transfer dst optimal layout
        command->m_commandType = Platform::eGraphicsCmdLayoutTransition;
        command->debugLabel = "Transition image layout to transfer dst optimal";
        command->m_imageHandle = m_allTextureGraphicsHandles[uiAsset];
        command->m_startLayout = Platform::eImageLayoutUndefined;
        command->m_endLayout = Platform::eImageLayoutTransferDst;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Texture buffer copy
        command->m_commandType = Platform::eGraphicsCmdMemTransfer;
        command->debugLabel = "Update Asset Texture Data";
        command->m_sizeInBytes = textureSizeInBytes;
        command->m_srcBufferHandle = imageStagingBufferHandles[uiAsset];
        command->m_dstBufferHandle = m_allTextureGraphicsHandles[uiAsset];
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // TODO: transition image to shader read optimal?
    }

    // Perform the copies
    platformFuncs->SubmitCmdsImmediate(graphicsCommandStream);
    graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream

    // Destroy the staging buffers
    for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
    {
        platformFuncs->UnmapResource(imageStagingBufferHandles[uiAsset]);
        platformFuncs->DestroyResource(imageStagingBufferHandles[uiAsset]);
    }

    m_textureBufferAllocator.ExplicitFree();
}

StaticMeshData* AssetManager::GetMeshGraphicsDataByID(uint32 meshID)
{
    TINKER_ASSERT(meshID != TINKER_INVALID_HANDLE);
    TINKER_ASSERT(meshID < TINKER_MAX_MESHES);
    TINKER_ASSERT(meshID >= 0);
    return &m_allStaticMeshGraphicsHandles[meshID];
}

ResourceHandle* AssetManager::GetTextureGraphicsDataByID(uint32 textureID)
{
    TINKER_ASSERT(textureID != TINKER_INVALID_HANDLE);
    TINKER_ASSERT(textureID < TINKER_MAX_TEXTURES);
    TINKER_ASSERT(textureID >= 0);
    return &m_allTextureGraphicsHandles[textureID];
}

#include "AssetManager.h"
#include "Utility/Logging.h"
#include "AssetFileParsing.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "Platform/PlatformGameAPI.h"
#include "Mem.h"
#include "Utility/ScopedTimer.h"

#include <string.h>

using namespace Tk;
using namespace Platform;

AssetManager g_AssetManager;

static const uint64 AssetFileMemorySize = 1024 * 1024 * 1024;
static Tk::Core::LinearAllocator g_AssetFileScratchMemory;

// For storing dumping obj vertex data during parsing
static Tk::Core::Asset::OBJParseScratchBuffers ScratchBuffers;

// For storing final obj vertex data
static Tk::Core::LinearAllocator VertPosAllocator;
static Tk::Core::LinearAllocator VertUVAllocator;
static Tk::Core::LinearAllocator VertNormalAllocator;
static Tk::Core::LinearAllocator VertIndexAllocator;

// For storing cooked asset data
static const uint32 MAX_COOKED_BUFFER_SIZE = 1024 * 1024 * 64;
static Tk::Core::LinearAllocator CookedDataAllocator;

static const uint32 numDemoMeshAssets = 4;
static const char* demoMeshAssetNames[numDemoMeshAssets] =
{
    "UnitSphere",
    "UnitCube",
    "FireElemental",
    "RTX3090"
};
static const char* demoMeshFilePaths[numDemoMeshAssets] =
{
    ASSETS_PATH "UnitSphere\\sphere.obj",
    ASSETS_PATH "UnitCube\\cube.obj",
    ASSETS_PATH "FireElemental\\fire_elemental.obj",
    ASSETS_PATH "RTX3090\\rtx3090.obj"
};

static const uint32 numDemoTextureAssets = 2;
static const char* demoTextureFilePaths[numDemoTextureAssets] =
{
    ASSETS_PATH "checkerboard512.bmp",
    ASSETS_PATH "checkerboardRGB512.bmp"
};


#include "StringTypes.h"

#ifdef _COOKED_ASSETS_DIR
#define COOKED_ASSETS_PATH STRINGIFY(_COOKED_ASSETS_DIR)
#else
//#define COOKED_ASSETS_PATH ".\\CookedAssets\\"
#endif

namespace AssetCooker
{
    static const char* fileSuffix = "ckMsh";

    typedef struct file_header
    {
        uint32 numVertices;
        uint32 pad[3];
    } FileHeader;

    typedef struct parsed_file_data
    {
        FileHeader header;
        const uint8* dataPtr;
    } ParsedFileData;

    void Init()
    {
        // TODO: create the COOKED_ASSETS_PATH dir if not present
    }

    ParsedFileData ParseFile(const uint8* fileBuffer)
    {
        ParsedFileData FileData = {};
        FileData.header = *(FileHeader*)fileBuffer;
        FileData.dataPtr = fileBuffer + sizeof(FileHeader);
        return FileData;
    }

    void GetCookedDataFileName(const char* inAssetFileName, char* outCookedFileName, uint32 outNameMaxLen)
    {
        Tk::Core::StrFixedBuffer<2048> MeshNameStr;
        MeshNameStr.Clear();

        MeshNameStr.Append(COOKED_ASSETS_PATH);
        MeshNameStr.Append("Mesh_");
        const uint32 nameHash = MurmurHash3_x86_32(inAssetFileName, (int)strlen(inAssetFileName), DEFAULT_STRING_HASH_SEED);
        _ultoa_s(nameHash, MeshNameStr.EndOfStrPtr(), MeshNameStr.LenRemaining(), 10);
        MeshNameStr.UpdateLen();
        MeshNameStr.Append(".");
        MeshNameStr.Append(fileSuffix);
        MeshNameStr.NullTerminate();

        if (MeshNameStr.m_len > outNameMaxLen)
        {
            // TODO error
        }
        else
        {
            memcpy(outCookedFileName, MeshNameStr.m_data, MeshNameStr.m_len);
        }
    }

    bool IsCookedDataFilePresentForAsset(const char* assetName)
    {
        Tk::Core::StrFixedBuffer<2048> MeshNameStr;
        MeshNameStr.Clear();
        GetCookedDataFileName(assetName, MeshNameStr.EndOfStrPtr(), MeshNameStr.LenRemaining());
        MeshNameStr.UpdateLen();

        return Tk::Platform::CheckFileExists(MeshNameStr.m_data);
    }
}

void AssetManager::FreeMemory()
{
    VertPosAllocator.ExplicitFree();
    VertUVAllocator.ExplicitFree();
    VertNormalAllocator.ExplicitFree();
    VertIndexAllocator.ExplicitFree();
}

void AssetManager::LoadAllAssets()
{
    g_AssetFileScratchMemory.Init(AssetFileMemorySize, CACHE_LINE);
    AssetCooker::Init();

    // Meshes

    m_numMeshAssets = numDemoMeshAssets;
    TINKER_ASSERT(m_numMeshAssets <= TINKER_MAX_MESHES);
    const char** meshFilePaths = &demoMeshFilePaths[0];
    const char** meshFileNames = &demoMeshAssetNames[0];

    if (m_numMeshAssets > 0)
    {
        uint32 totalMeshFileBytes = 0;
        uint32 meshFileSizes[TINKER_MAX_MESHES] = {};
        uint32 meshFileAlreadyCooked[TINKER_MAX_MESHES] = {}; // TODO needs to get replaced with a system that makes more sense

        for (uint32 uiAsset = 0; uiAsset < m_numMeshAssets; ++uiAsset)
        {
            static const uint32 filepathMax = 2048;
            Tk::Core::StrFixedBuffer<2048> AssetLoadPath;
            AssetLoadPath.Clear();

            // Check if cooked asset data file exists and use that size instead
            meshFileAlreadyCooked[uiAsset] = (uint32)AssetCooker::IsCookedDataFilePresentForAsset(meshFileNames[uiAsset]);
            if (meshFileAlreadyCooked[uiAsset])
            {
                AssetCooker::GetCookedDataFileName(meshFileNames[uiAsset], AssetLoadPath.EndOfStrPtr(), AssetLoadPath.LenRemaining());
                AssetLoadPath.UpdateLen();
            }
            else
            {
                AssetLoadPath.Append(meshFilePaths[uiAsset]);
            }

            const uint32 fileSize = Tk::Platform::GetEntireFileSize(AssetLoadPath.m_data);
            meshFileSizes[uiAsset] = fileSize;
            totalMeshFileBytes += fileSize;
        }

        // Allocate one large buffer to store a dump of all obj files.
        // Each obj file's data is separated by a single null byte to mark EOF
        uint8* meshFileDataBuffer = (uint8*)g_AssetFileScratchMemory.Alloc(totalMeshFileBytes + m_numMeshAssets, CACHE_LINE); // + 1 EOF byte for each obj file -> + m_numMeshAssets

        // Now precalculate the size of the vertex attribute buffers needed to contain the obj data
        uint32 meshBufferSizes[TINKER_MAX_MESHES] = {};
        uint32 totalMeshBufferSize = 0;

        uint32 accumFileOffset = 0;

        // Load cooked and/or obj files from disk
        {
            TIMED_SCOPED_BLOCK("Load mesh files from disk");

            bool multithreadLoading = true;
            if (multithreadLoading)
            {
                Tk::Platform::WorkerJobList jobs;
                jobs.Init(m_numMeshAssets);
                for (uint32 uiAsset = 0; uiAsset < m_numMeshAssets; ++uiAsset)
                {
                    uint8* currentMeshFile = meshFileDataBuffer + accumFileOffset;
                    uint32 currentMeshFileSize = meshFileSizes[uiAsset];
                    accumFileOffset += currentMeshFileSize + 1; // Account for manual EOF byte


                    // Duplicated logic from above
                    static const uint32 filepathMax = 2048;
                    Tk::Core::StrFixedBuffer<2048> AssetLoadPath;
                    AssetLoadPath.Clear();

                    // Check if cooked asset data file exists and use that size instead
                    meshFileAlreadyCooked[uiAsset] = (uint32)AssetCooker::IsCookedDataFilePresentForAsset(meshFileNames[uiAsset]);
                    if (meshFileAlreadyCooked[uiAsset])
                    {
                        AssetCooker::GetCookedDataFileName(meshFileNames[uiAsset], AssetLoadPath.EndOfStrPtr(), AssetLoadPath.LenRemaining());
                        AssetLoadPath.UpdateLen();
                    }
                    else
                    {
                        AssetLoadPath.Append(meshFilePaths[uiAsset]);
                    }

                    jobs.m_jobs[uiAsset] = Platform::CreateNewThreadJob([=]()
                        {
                            Tk::Platform::ReadEntireFile(AssetLoadPath.m_data, currentMeshFileSize, currentMeshFile);
                            currentMeshFile[currentMeshFileSize] = '\0'; // Mark EOF
                        });

                }
                Tk::Platform::EnqueueWorkerThreadJobList_Assisted(&jobs);
                jobs.WaitOnJobs();
                jobs.FreeList();
            }
            else
            {
                for (uint32 uiAsset = 0; uiAsset < m_numMeshAssets; ++uiAsset)
                {
                    uint8* currentMeshFile = meshFileDataBuffer + accumFileOffset;
                    uint32 currentMeshFileSize = meshFileSizes[uiAsset];
                    accumFileOffset += currentMeshFileSize + 1; // Account for manual EOF byte

                    // Duplicated logic from above
                    static const uint32 filepathMax = 2048;
                    Tk::Core::StrFixedBuffer<2048> AssetLoadPath;
                    AssetLoadPath.Clear();

                    // Check if cooked asset data file exists and use that size instead
                    meshFileAlreadyCooked[uiAsset] = (uint32)AssetCooker::IsCookedDataFilePresentForAsset(meshFileNames[uiAsset]);
                    if (meshFileAlreadyCooked[uiAsset])
                    {
                        AssetCooker::GetCookedDataFileName(meshFileNames[uiAsset], AssetLoadPath.EndOfStrPtr(), AssetLoadPath.LenRemaining());
                        AssetLoadPath.UpdateLen();
                    }
                    else
                    {
                        AssetLoadPath.Append(meshFilePaths[uiAsset]);
                    }

                    currentMeshFile[currentMeshFileSize] = '\0'; // Mark EOF
                    ReadEntireFile(meshFilePaths[uiAsset], currentMeshFileSize, currentMeshFile);
                }
            }
        }

        // Parse mesh files
        {
            TIMED_SCOPED_BLOCK("Parse mesh Files - single threaded");

            const uint32 MAX_VERT_BUFFER_SIZE = 1024 * 1024 * 128; // TODO: reduce this when we don't have to store all the attr buffers in the same linear allocator by doing graphics right here
            VertPosAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
            VertUVAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
            VertNormalAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
            VertIndexAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);

            ScratchBuffers = {};
            ScratchBuffers.VertPosAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
            ScratchBuffers.VertUVAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);
            ScratchBuffers.VertNormalAllocator.Init(MAX_VERT_BUFFER_SIZE, CACHE_LINE);

            CookedDataAllocator.Init(MAX_COOKED_BUFFER_SIZE, CACHE_LINE);

            accumFileOffset = 0;
            uint32 accumNumVerts = 0;
            for (uint32 uiAsset = 0; uiAsset < m_numMeshAssets; ++uiAsset)
            {
                const uint8* currentMeshFile = meshFileDataBuffer + accumFileOffset;
                uint32 currentMeshFileSize = meshFileSizes[uiAsset];
                accumFileOffset += currentMeshFileSize + 1; // Account for manual EOF byte

                // Parse obj file
                if (!meshFileAlreadyCooked[uiAsset])
                {
                    uint32 numObjVerts = 0;
                    Tk::Core::Asset::ParseOBJ(VertPosAllocator, VertUVAllocator, VertNormalAllocator, VertIndexAllocator, ScratchBuffers, currentMeshFile, currentMeshFileSize, &numObjVerts);
                    TINKER_ASSERT(numObjVerts); // TODO: log instead
                    m_allMeshData[uiAsset].m_numVertices = numObjVerts;
                    m_allMeshData[uiAsset].m_vertexBufferData_Pos = VertPosAllocator.m_ownedMemPtr + sizeof(v4f) * accumNumVerts;
                    m_allMeshData[uiAsset].m_vertexBufferData_UV = VertUVAllocator.m_ownedMemPtr + sizeof(v2f) * accumNumVerts;
                    m_allMeshData[uiAsset].m_vertexBufferData_Normal = VertNormalAllocator.m_ownedMemPtr + sizeof(v4f) * accumNumVerts;
                    m_allMeshData[uiAsset].m_vertexBufferData_Index = VertIndexAllocator.m_ownedMemPtr + sizeof(uint32) * accumNumVerts;
                    // TODO: create the graphics buffers right away and don't bother storing m_allMeshData at all

                    {
                        // Duplicated logic from above
                        static const uint32 filepathMax = 2048;
                        Tk::Core::StrFixedBuffer<2048> AssetLoadPath;
                        AssetLoadPath.Clear();

                        // NOTE: This is exactly how the graphics buffer will be written to. That code should unified with this logic 

                        AssetCooker::FileHeader header = {};
                        header.numVertices = m_allMeshData[uiAsset].m_numVertices;

                        const uint32 cookedDataBufferSize =
                            sizeof(header) +
                            sizeof(v4f) * m_allMeshData[uiAsset].m_numVertices +
                            sizeof(v2f) * m_allMeshData[uiAsset].m_numVertices +
                            sizeof(v4f) * m_allMeshData[uiAsset].m_numVertices +
                            sizeof(uint32) * m_allMeshData[uiAsset].m_numVertices;
                        uint8* cookedDataBufferHeader = CookedDataAllocator.Alloc(cookedDataBufferSize, CACHE_LINE);
                        uint8* cookedDataBufferPos = cookedDataBufferHeader + sizeof(header);
                        uint8* cookedDataBufferUV = cookedDataBufferPos + sizeof(v4f) * m_allMeshData[uiAsset].m_numVertices;
                        uint8* cookedDataBufferNormal = cookedDataBufferUV + sizeof(v2f) * m_allMeshData[uiAsset].m_numVertices;
                        uint8* cookedDataBufferIndex = cookedDataBufferNormal + sizeof(v4f) * m_allMeshData[uiAsset].m_numVertices;
                        TINKER_ASSERT(cookedDataBufferSize == (uint32)(cookedDataBufferIndex - cookedDataBufferHeader) + sizeof(uint32) * m_allMeshData[uiAsset].m_numVertices);
                        memcpy(cookedDataBufferHeader, &header, sizeof(header));
                        memcpy(cookedDataBufferPos, m_allMeshData[uiAsset].m_vertexBufferData_Pos, sizeof(v4f) * header.numVertices);
                        memcpy(cookedDataBufferUV, m_allMeshData[uiAsset].m_vertexBufferData_UV, sizeof(v2f) * header.numVertices);
                        memcpy(cookedDataBufferNormal, m_allMeshData[uiAsset].m_vertexBufferData_Normal, sizeof(v4f) * header.numVertices);
                        memcpy(cookedDataBufferIndex, m_allMeshData[uiAsset].m_vertexBufferData_Index, sizeof(uint32) * header.numVertices);

                        AssetCooker::GetCookedDataFileName(meshFileNames[uiAsset], AssetLoadPath.EndOfStrPtr(), AssetLoadPath.LenRemaining());
                        AssetLoadPath.UpdateLen();
                        Tk::Platform::WriteEntireFile(AssetLoadPath.m_data, cookedDataBufferSize, cookedDataBufferHeader);
                    }

                    ScratchBuffers.ResetState();
                    accumNumVerts += numObjVerts;
                }
                else
                {
                    // Move the cooked file data into persistent mem
                    // TODO: this is only to store it until we create the graphics resource. This should get refactored and not happen
                    uint8* cookedDataPtr = CookedDataAllocator.Alloc(currentMeshFileSize, CACHE_LINE);
                    memcpy(cookedDataPtr, currentMeshFile, currentMeshFileSize);

                    AssetCooker::ParsedFileData data = AssetCooker::ParseFile(cookedDataPtr);
                    m_allMeshData[uiAsset].m_numVertices = data.header.numVertices;
                    m_allMeshData[uiAsset].m_vertexBufferData_Pos = data.dataPtr;
                    m_allMeshData[uiAsset].m_vertexBufferData_UV = m_allMeshData[uiAsset].m_vertexBufferData_Pos + sizeof(v4f) * data.header.numVertices; // offset from previous buffer end location
                    m_allMeshData[uiAsset].m_vertexBufferData_Normal = m_allMeshData[uiAsset].m_vertexBufferData_UV + sizeof(v2f) * data.header.numVertices;
                    m_allMeshData[uiAsset].m_vertexBufferData_Index = m_allMeshData[uiAsset].m_vertexBufferData_Normal + sizeof(v4f) * data.header.numVertices;
                }
            }
        }
    }

    // Textures
    g_AssetFileScratchMemory.ResetState();

    m_numTextureAssets = numDemoTextureAssets;
    TINKER_ASSERT(m_numTextureAssets <= TINKER_MAX_TEXTURES);
    const char** textureFilePaths = &demoTextureFilePaths[0];
    
    if (m_numTextureAssets > 0)
    {
        // Compute the actual size of the texture data to be allocated and copied to the GPU
        // TODO: get file extension from string
        const char* fileExt = "bmp";

        uint32 totalTextureFileBytes = 0;
        uint32 textureFileSizes[TINKER_MAX_TEXTURES] = {};
        for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
        {
            uint32 fileSize = GetEntireFileSize(textureFilePaths[uiAsset]);
            textureFileSizes[uiAsset] = fileSize;
            totalTextureFileBytes += fileSize;
        }

        // Allocate one large buffer to store a dump of all texture files.
        uint8* textureFileDataBuffer = (uint8*)g_AssetFileScratchMemory.Alloc(totalTextureFileBytes, CACHE_LINE);

        uint32 accumFileOffset = 0;

        bool multithreadTextureLoading = true;
        if (multithreadTextureLoading)
        {
            Platform::WorkerJobList jobs;
            jobs.Init(m_numTextureAssets);
            for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
            {
                uint8* currentTextureFile = textureFileDataBuffer + accumFileOffset;
                uint32 currentTextureFileSize = textureFileSizes[uiAsset];
                accumFileOffset += currentTextureFileSize;

                jobs.m_jobs[uiAsset] = Platform::CreateNewThreadJob([=]()
                    {
                        ReadEntireFile(textureFilePaths[uiAsset], currentTextureFileSize, currentTextureFile);
                    });

                EnqueueWorkerThreadJob(jobs.m_jobs[uiAsset]);
            }
            jobs.WaitOnJobs();
            jobs.FreeList();
        }
        else
        {
            // Dump each file into the one big buffer
            for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
            {
                uint8* currentTextureFile = textureFileDataBuffer + accumFileOffset;
                uint32 currentTextureFileSize = textureFileSizes[uiAsset];
                accumFileOffset += currentTextureFileSize;

                // Read each file into the buffer
                ReadEntireFile(textureFilePaths[uiAsset], currentTextureFileSize, currentTextureFile);
            }
        }

        accumFileOffset = 0;
        uint32 totalActualTextureSize = 0;
        uint32 actualTextureSizes[TINKER_MAX_TEXTURES] = {};
        // Parse each texture file and dump the actual texture contents into the allocator
        for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
        {
            uint8* currentTextureFile = textureFileDataBuffer + accumFileOffset;
            uint32 currentTextureFileSize = textureFileSizes[uiAsset];
            accumFileOffset += currentTextureFileSize;

            uint32 actualTextureSizeInBytes = 0;

            if (strncmp(fileExt, "bmp", 3) == 0)
            {
                Tk::Core::Asset::BMPInfo info = Tk::Core::Asset::GetBMPInfo(currentTextureFile);

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
        m_textureBufferAllocator.Init(totalActualTextureSize, CACHE_LINE);

        accumFileOffset = 0;
        for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
        {
            uint8* currentTextureFile = textureFileDataBuffer + accumFileOffset;
            uint32 currentTextureFileSize = textureFileSizes[uiAsset];
            accumFileOffset += currentTextureFileSize;

            uint8* textureBuffer = m_textureBufferAllocator.Alloc(actualTextureSizes[uiAsset], 1);

            if (strncmp(fileExt, "bmp", 3) == 0)
            {
                uint8* textureBytesStart = currentTextureFile + sizeof(Tk::Core::Asset::BMPHeader) + sizeof(Tk::Core::Asset::BMPInfo);

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
    }

    g_AssetFileScratchMemory.ExplicitFree();
}

void AssetManager::InitAssetGraphicsResources(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    // Meshes
    for (uint32 uiAsset = 0; uiAsset < m_numMeshAssets; ++uiAsset)
    {
        // Create buffer handles
        Graphics::ResourceDesc desc;
        desc.resourceType = Graphics::ResourceType::eBuffer1D;

        Graphics::ResourceHandle stagingBufferHandle_Pos, stagingBufferHandle_UV, stagingBufferHandle_Norm, stagingBufferHandle_Idx;
        void* stagingBufferMemPtr_Pos, *stagingBufferMemPtr_UV, *stagingBufferMemPtr_Norm, *stagingBufferMemPtr_Idx;

        // Positions
        desc.dims = v3ui(m_allMeshData[uiAsset].m_numVertices * sizeof(v4f), 0, 0);
        desc.bufferUsage = Graphics::BufferUsage::eVertex;
        desc.debugLabel = "Asset Vtx Attr Buffer";
        m_allStaticMeshGraphicsHandles[uiAsset].m_positionBuffer.gpuBufferHandle = Graphics::CreateResource(desc);

        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        desc.debugLabel = "Asset Vtx Attr Staging Buffer";
        stagingBufferHandle_Pos = Graphics::CreateResource(desc);
        stagingBufferMemPtr_Pos = Graphics::MapResource(stagingBufferHandle_Pos);

        // UVs
        desc.dims = v3ui(m_allMeshData[uiAsset].m_numVertices * sizeof(v2f), 0, 0);
        desc.bufferUsage = Graphics::BufferUsage::eVertex;
        desc.debugLabel = "Asset Vtx Attr Buffer";
        m_allStaticMeshGraphicsHandles[uiAsset].m_uvBuffer.gpuBufferHandle = Graphics::CreateResource(desc);
        
        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        desc.debugLabel = "Asset Vtx Attr Staging Buffer";
        stagingBufferHandle_UV = Graphics::CreateResource(desc);
        stagingBufferMemPtr_UV = Graphics::MapResource(stagingBufferHandle_UV);

        // Normals
        desc.dims = v3ui(m_allMeshData[uiAsset].m_numVertices * sizeof(v4f), 0, 0);
        desc.bufferUsage = Graphics::BufferUsage::eVertex;
        desc.debugLabel = "Asset Vtx Attr Buffer";
        m_allStaticMeshGraphicsHandles[uiAsset].m_normalBuffer.gpuBufferHandle = Graphics::CreateResource(desc);
        
        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        desc.debugLabel = "Asset Vtx Attr Staging Buffer";
        stagingBufferHandle_Norm = Graphics::CreateResource(desc);
        stagingBufferMemPtr_Norm = Graphics::MapResource(stagingBufferHandle_Norm);

        // Indices
        desc.dims = v3ui(m_allMeshData[uiAsset].m_numVertices * sizeof(uint32), 0, 0);
        desc.bufferUsage = Graphics::BufferUsage::eIndex;
        desc.debugLabel = "Asset Idx Buffer";
        m_allStaticMeshGraphicsHandles[uiAsset].m_indexBuffer.gpuBufferHandle = Graphics::CreateResource(desc);
        
        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        desc.debugLabel = "Asset Idx Staging Buffer";
        stagingBufferHandle_Idx = Graphics::CreateResource(desc);
        stagingBufferMemPtr_Idx = Graphics::MapResource(stagingBufferHandle_Idx);

        m_allStaticMeshGraphicsHandles[uiAsset].m_numIndices = m_allMeshData[uiAsset].m_numVertices;

        // Descriptor
        CreateVertexBufferDescriptor(uiAsset);

        // Memcpy data into staging buffer
        const uint32 numPositionBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v4f);
        const uint32 numUVBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v2f);
        const uint32 numNormalBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v4f);
        const uint32 numIndexBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(uint32);

        v4f* positionBuffer = (v4f*)m_allMeshData[uiAsset].m_vertexBufferData_Pos;
        v2f* uvBuffer       = (v2f*)m_allMeshData[uiAsset].m_vertexBufferData_UV;
        v4f* normalBuffer   = (v4f*)m_allMeshData[uiAsset].m_vertexBufferData_Normal;
        uint32* indexBuffer = (uint32*)m_allMeshData[uiAsset].m_vertexBufferData_Index;
        memcpy(stagingBufferMemPtr_Pos, positionBuffer, numPositionBytes);
        memcpy(stagingBufferMemPtr_UV, uvBuffer, numUVBytes);
        memcpy(stagingBufferMemPtr_Norm, normalBuffer, numNormalBytes);
        memcpy(stagingBufferMemPtr_Idx, indexBuffer, numIndexBytes);
        //-----

        // Create, submit, and execute the buffer copy commands
        {
            // Graphics command to copy from staging buffer to gpu local buffer
            Tk::Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

            // Position buffer copy
            command->m_commandType = Graphics::GraphicsCommand::eMemTransfer;
            command->debugLabel = "Update Asset Vtx Pos Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v4f);
            command->m_srcBufferHandle = stagingBufferHandle_Pos;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_positionBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // UV buffer copy
            command->m_commandType = Graphics::GraphicsCommand::eMemTransfer;
            command->debugLabel = "Update Asset Vtx UV Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v2f);
            command->m_srcBufferHandle = stagingBufferHandle_UV;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_uvBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // Normal buffer copy
            command->m_commandType = Graphics::GraphicsCommand::eMemTransfer;
            command->debugLabel = "Update Asset Vtx Norm Buf";
            command->m_sizeInBytes = m_allMeshData[uiAsset].m_numVertices * sizeof(v4f);
            command->m_srcBufferHandle = stagingBufferHandle_Norm;
            command->m_dstBufferHandle = m_allStaticMeshGraphicsHandles[uiAsset].m_normalBuffer.gpuBufferHandle;
            ++graphicsCommandStream->m_numCommands;
            ++command;

            // Index buffer copy
            command->m_commandType = Graphics::GraphicsCommand::eMemTransfer;
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

    // Textures
    Graphics::ResourceHandle imageStagingBufferHandles[TINKER_MAX_TEXTURES] = {};

    uint32 accumTextureOffset = 0;
    for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
    {
        // Create texture handles
        Graphics::ResourceDesc desc;
        desc.resourceType = Graphics::ResourceType::eImage2D;
        desc.arrayEles = 1;

        desc.imageFormat = Graphics::ImageFormat::RGBA8_SRGB; // TODO: don't hard code this
        desc.dims = m_allTextureMetadata[uiAsset].m_dims;
        m_allTextureGraphicsHandles[uiAsset] = Graphics::CreateResource(desc);

        uint32 textureSizeInBytes = m_allTextureMetadata[uiAsset].m_dims.x * m_allTextureMetadata[uiAsset].m_dims.y * 4; // 4 bytes per pixel since RGBA8
        desc.dims = v3ui(textureSizeInBytes, 0, 0);
        desc.resourceType = Graphics::ResourceType::eBuffer1D; // staging buffer is just a 1D buffer
        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        imageStagingBufferHandles[uiAsset] = Graphics::CreateResource(desc);
        void* stagingBufferMemPtr = Graphics::MapResource(imageStagingBufferHandles[uiAsset]);

        // Copy texture data into the staging buffer
        uint8* currentTextureFile = m_textureBufferAllocator.m_ownedMemPtr + accumTextureOffset;
        accumTextureOffset += textureSizeInBytes;
        memcpy(stagingBufferMemPtr, currentTextureFile, textureSizeInBytes);
    }

    // Graphics command to copy from staging buffer to gpu local buffer
    Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    // Create, submit, and execute the buffer copy commands
    for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
    {
        uint32 textureSizeInBytes = m_allTextureMetadata[uiAsset].m_dims.x * m_allTextureMetadata[uiAsset].m_dims.y * 4; // 4 bytes per pixel since RGBA8

        // Transition to transfer dst optimal layout
        command->m_commandType = Graphics::GraphicsCommand::eLayoutTransition;
        command->debugLabel = "Transition image layout to transfer dst optimal";
        command->m_imageHandle = m_allTextureGraphicsHandles[uiAsset];
        command->m_startLayout = Graphics::ImageLayout::eUndefined;
        command->m_endLayout = Graphics::ImageLayout::eTransferDst;
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // Texture buffer copy
        command->m_commandType = Graphics::GraphicsCommand::eMemTransfer;
        command->debugLabel = "Update Asset Texture Data";
        command->m_sizeInBytes = textureSizeInBytes;
        command->m_srcBufferHandle = imageStagingBufferHandles[uiAsset];
        command->m_dstBufferHandle = m_allTextureGraphicsHandles[uiAsset];
        ++command;
        ++graphicsCommandStream->m_numCommands;

        // TODO: transition image to shader read optimal?
    }

    // Perform the copies
    Graphics::SubmitCmdsImmediate(graphicsCommandStream);
    graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream

    // Destroy the staging buffers
    for (uint32 uiAsset = 0; uiAsset < m_numTextureAssets; ++uiAsset)
    {
        Graphics::UnmapResource(imageStagingBufferHandles[uiAsset]);
        Graphics::DestroyResource(imageStagingBufferHandles[uiAsset]);
    }

    m_textureBufferAllocator.ExplicitFree();
}

void AssetManager::DestroyAllMeshData()
{
    for (uint32 uiAssetID = 0; uiAssetID < m_numMeshAssets; ++uiAssetID)
    {
        StaticMeshData* meshData = GetMeshGraphicsDataByID(uiAssetID);

        Graphics::DestroyResource(meshData->m_positionBuffer.gpuBufferHandle);
        meshData->m_positionBuffer.gpuBufferHandle = Graphics::DefaultResHandle_Invalid;
        Graphics::DestroyResource(meshData->m_uvBuffer.gpuBufferHandle);
        meshData->m_uvBuffer.gpuBufferHandle = Graphics::DefaultResHandle_Invalid;
        Graphics::DestroyResource(meshData->m_normalBuffer.gpuBufferHandle);
        meshData->m_normalBuffer.gpuBufferHandle = Graphics::DefaultResHandle_Invalid;
        Graphics::DestroyResource(meshData->m_indexBuffer.gpuBufferHandle);
        meshData->m_indexBuffer.gpuBufferHandle = Graphics::DefaultResHandle_Invalid;

        Graphics::DestroyDescriptor(meshData->m_descriptor);
        meshData->m_descriptor = Graphics::DefaultDescHandle_Invalid;
    }
}

void AssetManager::DestroyAllTextureData()
{
    for (uint32 uiAssetID = 0; uiAssetID < m_numTextureAssets; ++uiAssetID)
    {
        Graphics::ResourceHandle textureHandle = GetTextureGraphicsDataByID(uiAssetID);
        Graphics::DestroyResource(textureHandle);
    }
}

void AssetManager::CreateVertexBufferDescriptor(uint32 meshID)
{
    StaticMeshData* data = g_AssetManager.GetMeshGraphicsDataByID(meshID);
    data->m_descriptor = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_ASSET_VBS);

    Graphics::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    descDataHandles[0].InitInvalid();
    descDataHandles[0].handles[0] = data->m_positionBuffer.gpuBufferHandle;
    descDataHandles[0].handles[1] = data->m_uvBuffer.gpuBufferHandle;
    descDataHandles[0].handles[2] = data->m_normalBuffer.gpuBufferHandle;
    descDataHandles[1].InitInvalid();
    descDataHandles[2].InitInvalid();

    Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_ASSET_VBS, data->m_descriptor, &descDataHandles[0]);
}

StaticMeshData* AssetManager::GetMeshGraphicsDataByID(uint32 meshID)
{
    TINKER_ASSERT(meshID != TINKER_INVALID_HANDLE);
    TINKER_ASSERT(meshID < TINKER_MAX_MESHES);
    return &m_allStaticMeshGraphicsHandles[meshID];
}

Graphics::ResourceHandle AssetManager::GetTextureGraphicsDataByID(uint32 textureID) const
{
    TINKER_ASSERT(textureID != TINKER_INVALID_HANDLE);
    TINKER_ASSERT(textureID < TINKER_MAX_TEXTURES);
    return m_allTextureGraphicsHandles[textureID];
}

const MeshAttributeData& AssetManager::GetMeshAttrDataByID(uint32 meshID) const
{
    TINKER_ASSERT(meshID != TINKER_INVALID_HANDLE);
    TINKER_ASSERT(meshID < TINKER_MAX_MESHES);
    return m_allMeshData[meshID];
}


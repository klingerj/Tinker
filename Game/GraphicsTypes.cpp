#include "GraphicsTypes.h"

#include <string.h>

using namespace Tk;
using namespace Platform;

void CreateDefaultGeometry(const Tk::Platform::PlatformAPIFuncs* platformFuncs, Tk::Platform::GraphicsCommandStream* graphicsCommandStream)
{
    // Default Quad
    {
        ResourceDesc desc;
        desc.resourceType = Platform::ResourceType::eBuffer1D;

        // Positions
        desc.dims = v3ui(sizeof(defaultQuad.m_points), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eVertex;
        defaultQuad.m_positionBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::BufferUsage::eStaging;
        ResourceHandle stagingBufferHandle_Pos = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr_Pos = platformFuncs->MapResource(stagingBufferHandle_Pos);

        // UVs
        desc.dims = v3ui(sizeof(defaultQuad.m_uvs), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eVertex;
        defaultQuad.m_uvBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::BufferUsage::eStaging;
        ResourceHandle stagingBufferHandle_UV = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr_UV = platformFuncs->MapResource(stagingBufferHandle_UV);

        // Normals
        desc.dims = v3ui(sizeof(defaultQuad.m_normals), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eVertex;
        defaultQuad.m_normalBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::BufferUsage::eStaging;
        ResourceHandle stagingBufferHandle_Norm = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr_Norm = platformFuncs->MapResource(stagingBufferHandle_Norm);

        // Indices
        desc.dims = v3ui(sizeof(defaultQuad.m_indices), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eIndex;
        defaultQuad.m_indexBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::BufferUsage::eStaging;
        ResourceHandle stagingBufferHandle_Idx = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr_Idx = platformFuncs->MapResource(stagingBufferHandle_Idx);

        // Descriptor
        CreateDefaultGeometryVertexBufferDescriptor(defaultQuad, platformFuncs);

        // Memcpy into staging buffers
        memcpy(stagingBufferMemPtr_Pos, defaultQuad.m_points, sizeof(defaultQuad.m_points));
        memcpy(stagingBufferMemPtr_UV, defaultQuad.m_uvs, sizeof(defaultQuad.m_uvs));
        memcpy(stagingBufferMemPtr_Norm, defaultQuad.m_normals, sizeof(defaultQuad.m_normals));
        memcpy(stagingBufferMemPtr_Idx, defaultQuad.m_indices, sizeof(defaultQuad.m_indices));

        // Do GPU buffer copies
        Tk::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];
        command->m_commandType = (uint32)Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Default Quad Vtx Pos Buf";
        command->m_sizeInBytes = sizeof(defaultQuad.m_points);
        command->m_dstBufferHandle = defaultQuad.m_positionBuffer.gpuBufferHandle;
        command->m_srcBufferHandle = stagingBufferHandle_Pos;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        command->m_commandType = (uint32)Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Default Quad Vtx UV Buf";
        command->m_sizeInBytes = sizeof(defaultQuad.m_uvs);
        command->m_dstBufferHandle = defaultQuad.m_uvBuffer.gpuBufferHandle;
        command->m_srcBufferHandle = stagingBufferHandle_UV;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        command->m_commandType = (uint32)Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Default Quad Vtx Norm Buf";
        command->m_sizeInBytes = sizeof(defaultQuad.m_normals);
        command->m_dstBufferHandle = defaultQuad.m_normalBuffer.gpuBufferHandle;
        command->m_srcBufferHandle = stagingBufferHandle_Norm;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        command->m_commandType = (uint32)Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Default Quad Vtx Idx Buf";
        command->m_sizeInBytes = sizeof(defaultQuad.m_indices);
        command->m_dstBufferHandle = defaultQuad.m_indexBuffer.gpuBufferHandle;
        command->m_srcBufferHandle = stagingBufferHandle_Idx;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        platformFuncs->SubmitCmdsImmediate(graphicsCommandStream);
        graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream

        // Unmap + destroy resources
        platformFuncs->UnmapResource(stagingBufferHandle_Pos);
        platformFuncs->UnmapResource(stagingBufferHandle_UV);
        platformFuncs->UnmapResource(stagingBufferHandle_Norm);
        platformFuncs->UnmapResource(stagingBufferHandle_Idx);

        platformFuncs->DestroyResource(stagingBufferHandle_Pos);
        platformFuncs->DestroyResource(stagingBufferHandle_UV);
        platformFuncs->DestroyResource(stagingBufferHandle_Norm);
        platformFuncs->DestroyResource(stagingBufferHandle_Idx);
    }
}

void DestroyDefaultGeometry(const Tk::Platform::PlatformAPIFuncs* platformFuncs)
{
    // Default quad
    platformFuncs->DestroyResource(defaultQuad.m_positionBuffer.gpuBufferHandle);
    defaultQuad.m_positionBuffer.gpuBufferHandle = Platform::DefaultResHandle_Invalid;
    platformFuncs->DestroyResource(defaultQuad.m_uvBuffer.gpuBufferHandle);
    defaultQuad.m_uvBuffer.gpuBufferHandle = Platform::DefaultResHandle_Invalid;
    platformFuncs->DestroyResource(defaultQuad.m_normalBuffer.gpuBufferHandle);
    defaultQuad.m_normalBuffer.gpuBufferHandle = Platform::DefaultResHandle_Invalid;
    platformFuncs->DestroyResource(defaultQuad.m_indexBuffer.gpuBufferHandle);
    defaultQuad.m_indexBuffer.gpuBufferHandle = Platform::DefaultResHandle_Invalid;

    //DestroyDefaultGeometryVertexBufferDescriptor(defaultQuad, platformFuncs);
}

DefaultGeometry<DEFAULT_QUAD_NUM_VERTICES, DEFAULT_QUAD_NUM_INDICES> defaultQuad = {
    // buffer handles
    { DefaultResHandle_Invalid },
    { DefaultResHandle_Invalid },
    { DefaultResHandle_Invalid },
    { DefaultResHandle_Invalid },
    // descriptor
    { DefaultDescHandle_Invalid },
    // positions
    v4f(-1.0f, -1.0f, 0.0f, 1.0f),
    v4f(1.0f, -1.0f, 0.0f, 1.0f),
    v4f(-1.0f, 1.0f, 0.0f, 1.0f),
    v4f(1.0f, 1.0f, 0.0f, 1.0f),
    //uvs
    v2f(0.0f, 0.0f),
    v2f(0.0f, 1.0f),
    v2f(0.0f, 1.0f),
    v2f(1.0f, 1.0f),
    // normals
    v4f(0.0f, 0.0f, 1.0f, 0.0f),
    v4f(0.0f, 0.0f, 1.0f, 0.0f),
    v4f(0.0f, 0.0f, 1.0f, 0.0f),
    v4f(0.0f, 0.0f, 1.0f, 0.0f),
    // indices
    0, 1, 2, 2, 1, 3
};

void CreateAnimatedPoly(const Tk::Platform::PlatformAPIFuncs* platformFuncs, TransientPrim* prim)
{
    Platform::ResourceDesc desc;
    desc.resourceType = Platform::ResourceType::eBuffer1D;

    prim->numVertices = 150;

    desc.dims = v3ui(prim->numVertices * sizeof(v4f), 0, 0);
    desc.bufferUsage = Platform::BufferUsage::eTransientVertex;
    prim->vertexBufferHandle = platformFuncs->CreateResource(desc);
    desc.bufferUsage = Platform::BufferUsage::eTransientIndex;
    desc.dims = v3ui((prim->numVertices - 1) * 3 * sizeof(uint32), 0, 0);
    prim->indexBufferHandle = platformFuncs->CreateResource(desc);

    // Descriptor - vertex buffer
    Platform::DescriptorLayout descriptorLayout = {};
    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;

    prim->descriptor = platformFuncs->CreateDescriptor(&descriptorLayout);

    Platform::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    descDataHandles[0].InitInvalid();
    descDataHandles[1].InitInvalid();
    descDataHandles[1].handles[0] = prim->vertexBufferHandle;
    descDataHandles[2].InitInvalid();

    Platform::DescriptorHandle descHandles[MAX_BINDINGS_PER_SET] = { Platform::DefaultDescHandle_Invalid, prim->descriptor, Platform::DefaultDescHandle_Invalid };
    platformFuncs->WriteDescriptor(&descriptorLayout, &descHandles[0], &descDataHandles[0]);
}

void DestroyAnimatedPoly(const Tk::Platform::PlatformAPIFuncs* platformFuncs, TransientPrim* prim)
{
    platformFuncs->DestroyResource(prim->indexBufferHandle);
    prim->indexBufferHandle = Platform::DefaultResHandle_Invalid;
    platformFuncs->DestroyResource(prim->vertexBufferHandle);
    prim->vertexBufferHandle = Platform::DefaultResHandle_Invalid;

    platformFuncs->DestroyDescriptor(prim->descriptor);
    prim->descriptor = Platform::DefaultDescHandle_Invalid;
}

void UpdateAnimatedPoly(const Tk::Platform::PlatformAPIFuncs* platformFuncs, TransientPrim* prim)
{
    // Map
    void* indexBuf = platformFuncs->MapResource(prim->indexBufferHandle);
    void* vertexBuf = platformFuncs->MapResource(prim->vertexBufferHandle);

    // Update
    const uint32 numIndices = ((prim->numVertices - 1) * 3);
    for (uint32 idx = 0; idx < numIndices; idx += 3)
    {
        ((uint32*)indexBuf)[idx + 0] = 0;
        ((uint32*)indexBuf)[idx + 1] = idx / 3 + 1;
        ((uint32*)indexBuf)[idx + 2] = idx < numIndices - 3 ? idx / 3 + 2 : 1;
    }

    static uint32 frameCtr = 0;
    ++frameCtr;
    for (uint32 vtx = 0; vtx < prim->numVertices; ++vtx)
    {
        if (vtx == 0)
        {
            ((v4f*)vertexBuf)[vtx] = v4f(0.0f, 0.0f, 0.0f, 1.0f);
        }
        else
        {
            const float f = ((float)(vtx - 1) / (prim->numVertices - 1));
            const float amt = f * (3.14159f * 2.0f);
            const float scale = 2.0f * (cosf((float)frameCtr * 0.2f * f) * 0.5f + 0.5f);
            ((v4f*)vertexBuf)[vtx] = v4f(cosf(amt) * scale, sinf(amt) * scale, 0.0f, 1.0f);
        }
    }

    // Unmap
    platformFuncs->UnmapResource(prim->indexBufferHandle);
    platformFuncs->UnmapResource(prim->vertexBufferHandle);
}

void DrawAnimatedPoly(TransientPrim* prim, Tk::Platform::DescriptorHandle globalData, uint32 shaderID, uint32 blendState, uint32 depthState, Tk::Platform::GraphicsCommandStream* graphicsCommandStream)
{
    Tk::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Platform::GraphicsCmd::eDrawCall;
    command->debugLabel = "Draw animated poly";
    command->m_numIndices = (prim->numVertices - 1) * 3;
    command->m_numInstances = 1;
    command->m_indexBufferHandle = prim->indexBufferHandle;
    command->m_shader = shaderID;
    command->m_blendState = blendState;
    command->m_depthState = depthState;

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        command->m_descriptors[i] = Platform::DefaultDescHandle_Invalid;
    }
    command->m_descriptors[0] = globalData;
    command->m_descriptors[1] = prim->descriptor;
    ++graphicsCommandStream->m_numCommands;
}

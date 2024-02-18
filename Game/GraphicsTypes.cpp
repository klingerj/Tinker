#include "GraphicsTypes.h"
#include "BindlessSystem.h"

#include <string.h>

using namespace Tk;

void CreateDefaultGeometry(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    Tk::Graphics::CommandBuffer commandBuffer = Tk::Graphics::CreateCommandBuffer();

    // Default Quad
    {
        Graphics::ResourceDesc desc;
        desc.resourceType = Graphics::ResourceType::eBuffer1D;
        desc.debugLabel = "Default Quad Vtx Attr Buffer";

        const uint32 numBytesPos = sizeof(defaultQuad.m_points);
        const uint32 numBytesUV = sizeof(defaultQuad.m_uvs);
        const uint32 numBytesNorm = sizeof(defaultQuad.m_normals);
        const uint32 numBytesIdx = sizeof(defaultQuad.m_indices);

        // Positions
        desc.dims = v3ui(numBytesPos, 0, 0);
        desc.bufferUsage = Graphics::BufferUsage::eVertex;
        defaultQuad.m_positionBuffer.gpuBufferHandle = Graphics::CreateResource(desc);

        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        Graphics::ResourceHandle stagingBufferPos = Graphics::CreateResource(desc);
        Graphics::MemoryMappedBufferPtr stagingBufferPtrPos = Graphics::MapResource(stagingBufferPos);

        // UVs
        desc.dims = v3ui(numBytesUV, 0, 0);
        desc.bufferUsage = Graphics::BufferUsage::eVertex;
        defaultQuad.m_uvBuffer.gpuBufferHandle = Graphics::CreateResource(desc);

        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        Graphics::ResourceHandle stagingBufferUV = Graphics::CreateResource(desc);
        Graphics::MemoryMappedBufferPtr stagingBufferPtrUV = Graphics::MapResource(stagingBufferUV);

        // Normals
        desc.dims = v3ui(numBytesNorm, 0, 0);
        desc.bufferUsage = Graphics::BufferUsage::eVertex;
        defaultQuad.m_normalBuffer.gpuBufferHandle = Graphics::CreateResource(desc);

        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        Graphics::ResourceHandle stagingBufferNorm = Graphics::CreateResource(desc);
        Graphics::MemoryMappedBufferPtr stagingBufferPtrNorm = Graphics::MapResource(stagingBufferNorm);

        // Indices
        desc.dims = v3ui(numBytesIdx, 0, 0);
        desc.bufferUsage = Graphics::BufferUsage::eIndex;
        defaultQuad.m_indexBuffer.gpuBufferHandle = Graphics::CreateResource(desc);

        desc.bufferUsage = Graphics::BufferUsage::eStaging;
        Graphics::ResourceHandle stagingBufferIdx = Graphics::CreateResource(desc);
        Graphics::MemoryMappedBufferPtr stagingBufferPtrIdx = Graphics::MapResource(stagingBufferIdx);

        // Descriptor
        CreateDefaultGeometryVertexBufferDescriptor(defaultQuad);
        
        // Memcpy into staging buffers
        stagingBufferPtrPos.MemcpyInto(defaultQuad.m_points, numBytesPos);
        stagingBufferPtrUV.MemcpyInto(defaultQuad.m_uvs, numBytesUV);
        stagingBufferPtrNorm.MemcpyInto(defaultQuad.m_normals, numBytesNorm);
        stagingBufferPtrIdx.MemcpyInto(defaultQuad.m_indices, numBytesIdx);
        
        // Do GPU buffer copies
        graphicsCommandStream->CmdCommandBufferBegin(commandBuffer, "Default geometry creation cmd buffer");

        graphicsCommandStream->CmdCopy(stagingBufferPos,
            defaultQuad.m_positionBuffer.gpuBufferHandle,
            numBytesPos,
            "Update Default Quad Vtx Pos Buf");
        graphicsCommandStream->CmdCopy(stagingBufferUV,
            defaultQuad.m_uvBuffer.gpuBufferHandle,
            numBytesUV,
            "Update Default Quad Vtx UV Buf");
        graphicsCommandStream->CmdCopy(stagingBufferNorm,
            defaultQuad.m_normalBuffer.gpuBufferHandle,
            numBytesNorm,
            "Update Default Quad Vtx Norm Buf");
        graphicsCommandStream->CmdCopy(stagingBufferIdx,
            defaultQuad.m_indexBuffer.gpuBufferHandle,
            numBytesIdx,
            "Update Default Quad Vtx Idx Buf");

        graphicsCommandStream->CmdCommandBufferEnd(commandBuffer);
        Graphics::SubmitCmdsImmediate(graphicsCommandStream, commandBuffer);
        graphicsCommandStream->Clear();

        // Unmap + destroy resources
        Graphics::UnmapResource(stagingBufferPos);
        Graphics::UnmapResource(stagingBufferUV);
        Graphics::UnmapResource(stagingBufferNorm);
        Graphics::UnmapResource(stagingBufferIdx);

        Graphics::DestroyResource(stagingBufferPos);
        Graphics::DestroyResource(stagingBufferUV);
        Graphics::DestroyResource(stagingBufferNorm);
        Graphics::DestroyResource(stagingBufferIdx);
    }
}

void DestroyDefaultGeometry()
{
    // Default quad
    Graphics::DestroyResource(defaultQuad.m_positionBuffer.gpuBufferHandle);
    defaultQuad.m_positionBuffer.gpuBufferHandle = Graphics::DefaultResHandle_Invalid;
    Graphics::DestroyResource(defaultQuad.m_uvBuffer.gpuBufferHandle);
    defaultQuad.m_uvBuffer.gpuBufferHandle = Graphics::DefaultResHandle_Invalid;
    Graphics::DestroyResource(defaultQuad.m_normalBuffer.gpuBufferHandle);
    defaultQuad.m_normalBuffer.gpuBufferHandle = Graphics::DefaultResHandle_Invalid;
    Graphics::DestroyResource(defaultQuad.m_indexBuffer.gpuBufferHandle);
    defaultQuad.m_indexBuffer.gpuBufferHandle = Graphics::DefaultResHandle_Invalid;

    DestroyDefaultGeometryVertexBufferDescriptor(defaultQuad);
}

DefaultGeometry<DEFAULT_QUAD_NUM_VERTICES, DEFAULT_QUAD_NUM_INDICES> defaultQuad = {
    // buffer handles
    { Graphics::DefaultResHandle_Invalid },
    { Graphics::DefaultResHandle_Invalid },
    { Graphics::DefaultResHandle_Invalid },
    { Graphics::DefaultResHandle_Invalid },
    // descriptor
    { Graphics::DefaultDescHandle_Invalid },
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

void CreateAnimatedPoly(TransientPrim* prim)
{
    Graphics::ResourceDesc desc;
    desc.resourceType = Graphics::ResourceType::eBuffer1D;

    prim->numVertices = 150;

    desc.dims = v3ui(prim->numVertices * sizeof(v4f), 0, 0);
    desc.bufferUsage = Graphics::BufferUsage::eTransient;
    desc.debugLabel = "AnimatedPoly Transient Vtx Buf";
    prim->vertexBufferHandle = Graphics::CreateResource(desc);
    desc.bufferUsage = Graphics::BufferUsage::eTransientIndex;
    desc.dims = v3ui((prim->numVertices - 1) * 3 * sizeof(uint32), 0, 0);
    desc.debugLabel = "AnimatedPoly Transient Idx Buf";
    
    prim->indexBufferHandle = Graphics::CreateResource(desc);

    // Descriptor - vertex buffer
    prim->descriptor = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_POSONLY_VBS);

    Graphics::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    descDataHandles[0].InitInvalid();
    descDataHandles[0].handles[0] = prim->vertexBufferHandle;
    descDataHandles[1].InitInvalid();
    descDataHandles[2].InitInvalid();
    Graphics::WriteDescriptorSimple(prim->descriptor, &descDataHandles[0]);
}

void DestroyAnimatedPoly(TransientPrim* prim)
{
    Graphics::DestroyResource(prim->indexBufferHandle);
    prim->indexBufferHandle = Graphics::DefaultResHandle_Invalid;
    Graphics::DestroyResource(prim->vertexBufferHandle);
    prim->vertexBufferHandle = Graphics::DefaultResHandle_Invalid;

    Graphics::DestroyDescriptor(prim->descriptor);
    prim->descriptor = Graphics::DefaultDescHandle_Invalid;
}

void UpdateAnimatedPoly(TransientPrim* prim)
{
    // Map
    Graphics::MemoryMappedBufferPtr indexBuf = Graphics::MapResource(prim->indexBufferHandle);
    Graphics::MemoryMappedBufferPtr vertexBuf = Graphics::MapResource(prim->vertexBufferHandle);

    // Update
    const uint32 numIndices = ((prim->numVertices - 1) * 3);
    for (uint32 idx = 0; idx < numIndices; idx += 3)
    {
        ((uint32*)indexBuf.m_ptr)[idx + 0] = 0;
        ((uint32*)indexBuf.m_ptr)[idx + 1] = idx / 3 + 1;
        ((uint32*)indexBuf.m_ptr)[idx + 2] = idx < numIndices - 3 ? idx / 3 + 2 : 1;
    }

    static uint32 frameCtr = 0;
    ++frameCtr;
    for (uint32 vtx = 0; vtx < prim->numVertices; ++vtx)
    {
        if (vtx == 0)
        {
            ((v4f*)vertexBuf.m_ptr)[vtx] = v4f(0.0f, 0.0f, 0.0f, 1.0f);
        }
        else
        {
            const float f = ((float)(vtx - 1) / (prim->numVertices - 1));
            const float amt = f * (3.14159f * 2.0f);
            const float scale = 2.0f * (cosf((float)frameCtr * 0.2f * f) * 0.5f + 0.5f);
            ((v4f*)vertexBuf.m_ptr)[vtx] = v4f(cosf(amt) * scale, sinf(amt) * scale, 0.0f, 1.0f);
        }
    }

    // Unmap
    Graphics::UnmapResource(prim->indexBufferHandle);
    Graphics::UnmapResource(prim->vertexBufferHandle);
}

void DrawAnimatedPoly(TransientPrim* prim, uint32 shaderID, uint32 blendState, uint32 depthState, Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    Graphics::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        descriptors[i] = Graphics::DefaultDescHandle_Invalid;
    }
    descriptors[0] = BindlessSystem::GetBindlessConstantBufferDescriptor();
    descriptors[1] = prim->descriptor;
    graphicsCommandStream->CmdDraw((prim->numVertices - 1) * 3,
        1, 0, 0,
        shaderID,
        blendState,
        depthState,
        prim->indexBufferHandle,
        MAX_DESCRIPTOR_SETS_PER_SHADER, descriptors,
        "Draw animated poly");
}

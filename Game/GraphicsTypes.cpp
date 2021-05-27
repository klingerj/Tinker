#include "GraphicsTypes.h"

#include <string.h>

using namespace Tk;
using namespace Platform;

/*
void CopyStagingBufferToGPUBufferCommand(Tk::Platform::GraphicsCommandStream* graphicsCommandStream,
    ResourceHandle stagingBufferHandle, ResourceHandle gpuBufferHandle, uint32 bufferSizeInBytes,
    const char* debugLabel)
{
    Tk::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];
    command->m_commandType = Platform::GraphicsCmd::eMemTransfer;
    command->debugLabel = debugLabel;
    command->m_sizeInBytes = bufferSizeInBytes;
    command->m_srcBufferHandle = stagingBufferHandle;
    command->m_dstBufferHandle = gpuBufferHandle;
    ++graphicsCommandStream->m_numCommands;
}
*/

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
    platformFuncs->DestroyResource(defaultQuad.m_uvBuffer.gpuBufferHandle);
    platformFuncs->DestroyResource(defaultQuad.m_normalBuffer.gpuBufferHandle);
    platformFuncs->DestroyResource(defaultQuad.m_indexBuffer.gpuBufferHandle);
}

DefaultGeometry<DEFAULT_QUAD_NUM_VERTICES, DEFAULT_QUAD_NUM_INDICES> defaultQuad = {
    // buffer handles
    { DefaultResHandle_Invalid },
    { DefaultResHandle_Invalid },
    { DefaultResHandle_Invalid },
    { DefaultResHandle_Invalid },
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
    v3f(0.0f, 0.0f, 1.0f),
    v3f(0.0f, 0.0f, 1.0f),
    v3f(0.0f, 0.0f, 1.0f),
    v3f(0.0f, 0.0f, 1.0f),
    // indices
    0, 1, 2, 2, 1, 3
};


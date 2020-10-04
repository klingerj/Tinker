#pragma once

#include "../Include/Core/CoreDefines.h"
#include "../Include/PlatformGameAPI.h"

#include <vector> // TODO: remove me

using namespace Tinker;

// Buffer that has no persistent staging buffer
// Meant to updated once with a staging buffer which should
// then be destroyed.
typedef struct static_buffer_data
{
    uint32 gpuBufferHandle;
} StaticBuffer;

// Buffer that has a constantly mapped staging buffer
// Likely is updated frame-to-frame
typedef struct dynamic_buffer_data
{
    uint32 gpuBufferHandle;
    uint32 stagingBufferHandle;
    void* stagingBufferMemPtr;
} DynamicBuffer;

typedef struct static_mesh_data
{
    StaticBuffer m_positionBuffer;
    StaticBuffer m_normalBuffer;
    StaticBuffer m_indexBuffer;
} StaticMeshData;

typedef struct dynamic_mesh_data
{
    DynamicBuffer m_positionBuffer;
    DynamicBuffer m_normalBuffer;
    DynamicBuffer m_indexBuffer;
} DynamicMeshData;

void UpdateDynamicBufferCommand(std::vector<Platform::GraphicsCommand>& graphicsCommands, DynamicBuffer* dynamicBuffer, uint32 bufferSizeInBytes)
{
    Platform::GraphicsCommand command;
    command.m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;

    command.m_sizeInBytes = bufferSizeInBytes;
    command.m_srcBufferHandle = dynamicBuffer->stagingBufferHandle;
    command.m_dstBufferHandle = dynamicBuffer->gpuBufferHandle;
    graphicsCommands.push_back(command);
}

void DrawMeshDataCommand(std::vector<Platform::GraphicsCommand>& graphicsCommands, uint32 numIndices,
        uint32 indexBufferHandle, uint32 positionBufferHandle, uint32 normalBufferHandle,
        uint32 shaderHandle, Platform::DescriptorSetDataHandles* descriptors)
{
    Platform::GraphicsCommand command;
    command.m_commandType = (uint32)Platform::eGraphicsCmdDrawCall;

    command.m_numIndices = numIndices;
    command.m_indexBufferHandle = indexBufferHandle;
    command.m_positionBufferHandle = positionBufferHandle;
    command.m_normalBufferHandle = normalBufferHandle;
    //command.m_uvBufferHandle = TINKER_INVALID_HANDLE;
    command.m_shaderHandle = shaderHandle;
    memcpy(command.m_descriptors, descriptors, sizeof(command.m_descriptors));
    graphicsCommands.push_back(command);
}

typedef struct game_graphic_data
{
    uint32 m_imageHandle;
    uint32 m_imageViewHandle;
    uint32 m_framebufferHandle;

    uint32 m_shaderHandle;
    uint32 m_blitShaderHandle;
    uint32 m_swapChainBlitDescHandle;
    uint32 m_modelMatrixDescHandle1;
    uint32 m_modelMatrixBufferHandle1;
    void* m_modelMatrixBufferMemPtr1;
    uint32 m_mainRenderPassHandle;
} GameGraphicsData;

template <uint32 numPoints, uint32 numIndices>
struct default_geometry
{
    DynamicBuffer m_positionBuffer;
    DynamicBuffer m_normalBuffer;
    DynamicBuffer m_indexBuffer;
    Core::Math::v4f m_points[numPoints];
    Core::Math::v3f m_normals[numPoints];
    uint32 m_indices[numIndices];
};

template <uint32 numPoints, uint32 numIndices>
using DefaultGeometry = struct default_geometry<numPoints, numIndices>;

extern DefaultGeometry<4, 6> defaultQuad;


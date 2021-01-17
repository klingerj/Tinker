#pragma once

#include "../Include/Core/CoreDefines.h"
#include "../Include/Core/Math/VectorTypes.h"
#include "../Include/PlatformGameAPI.h"

#include <vector> // TODO: remove me

using namespace Tinker;
using namespace Platform;

// Buffer that has no persistent staging buffer
// Meant to updated once with a staging buffer which should
// then be destroyed.
typedef struct static_buffer_data
{
    ResourceHandle gpuBufferHandle;
} StaticBuffer;

// Buffer that has a constantly mapped staging buffer
// Likely is updated frame-to-frame
typedef struct dynamic_buffer_data
{
    ResourceHandle gpuBufferHandle;
    ResourceHandle stagingBufferHandle;
    void* stagingBufferMemPtr;
} DynamicBuffer;

typedef struct static_mesh_data
{
    StaticBuffer m_positionBuffer;
    StaticBuffer m_uvBuffer;
    StaticBuffer m_normalBuffer;
    StaticBuffer m_indexBuffer;
    uint32 m_numIndices;
} StaticMeshData;

typedef struct dynamic_mesh_data
{
    DynamicBuffer m_positionBuffer;
    DynamicBuffer m_uvBuffer;
    DynamicBuffer m_normalBuffer;
    DynamicBuffer m_indexBuffer;
    uint32 m_numIndices;
} DynamicMeshData;

void CopyStagingBufferToGPUBufferCommand(std::vector<Platform::GraphicsCommand>& graphicsCommands,
    ResourceHandle stagingBufferHandle, ResourceHandle gpuBufferHandle, uint32 bufferSizeInBytes,
    const char* debugLabel);

void DrawMeshDataCommand(std::vector<Platform::GraphicsCommand>& graphicsCommands, uint32 numIndices,
    ResourceHandle indexBufferHandle, ResourceHandle positionBufferHandle, ResourceHandle uvBufferHandle,
    ResourceHandle normalBufferHandle, ShaderHandle shaderHandle, Platform::DescriptorSetDescHandles* descriptors,
    const char* debugLabel);

typedef struct meshTriangles
{
    uint32 m_numVertices;
    uint8* m_vertexBufferData; // positions, uvs, normals, indices
} MeshAttributeData;

typedef struct game_graphic_data
{
    ResourceHandle m_rtColorHandle;
    ResourceHandle m_rtDepthHandle;
    FramebufferHandle m_framebufferHandle;
    //ResourceHandle m_mainRenderPassHandle;

    ShaderHandle m_shaderHandle;
    DescriptorHandle m_modelMatrixDescHandle1;
    ResourceHandle m_modelMatrixBufferHandle1;
    void* m_modelMatrixBufferMemPtr1;

    ShaderHandle m_blitShaderHandle;
    DescriptorHandle m_swapChainBlitDescHandle;
} GameGraphicsData;

template <uint32 numPoints, uint32 numIndices>
struct default_geometry
{
    StaticBuffer m_positionBuffer;
    StaticBuffer m_uvBuffer;
    StaticBuffer m_normalBuffer;
    StaticBuffer m_indexBuffer;
    Core::Math::v4f m_points[numPoints];
    Core::Math::v2f m_uvs[numPoints];
    Core::Math::v3f m_normals[numPoints];
    uint32 m_indices[numIndices];
};

template <uint32 numPoints, uint32 numIndices>
using DefaultGeometry = struct default_geometry<numPoints, numIndices>;

extern DefaultGeometry<4, 6> defaultQuad;

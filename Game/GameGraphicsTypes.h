#pragma once

#include "../Include/Core/CoreDefines.h"

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
    StaticBuffer m_vertexBuffer;
    StaticBuffer m_indexBuffer;
} StaticMeshData;

typedef struct dynamic_mesh_data
{
    DynamicBuffer m_vertexBuffer;
    DynamicBuffer m_indexBuffer;
} DynamicMeshData;

// TODO: make a function that automatically records graphics commands for a mesh
// UpdateBufferCmd(), DrawCmd()
// Take a GraphicsCommandStream* as a parameter

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
    uint32 m_vertexBufferHandle;
    uint32 m_indexBufferHandle;
    uint32 m_stagingBufferHandle_vert;
    uint32 m_stagingBufferHandle_idx;
    Core::Math::v4f m_points[numPoints];
    uint32 m_indices[numIndices];
};

template <uint32 numPoints, uint32 numIndices>
using DefaultGeometry = struct default_geometry<numPoints, numIndices>;

extern DefaultGeometry<4, 6> defaultQuad;


#pragma once

#include "Core/CoreDefines.h"
#include "Core/Math/VectorTypes.h"
#include "PlatformGameAPI.h"

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

/*
void CopyStagingBufferToGPUBufferCommand(std::vector<Platform::GraphicsCommand>& graphicsCommands,
    ResourceHandle stagingBufferHandle, ResourceHandle gpuBufferHandle, uint32 bufferSizeInBytes,
    const char* debugLabel);
*/

typedef struct mesh_attribute_data
{
    uint32 m_numVertices;
    uint8* m_vertexBufferData; // positions, uvs, normals, indices
} MeshAttributeData;

typedef struct texture_metadata
{
    Core::Math::v3ui m_dims;
    uint16 m_bitsPerPixel;
    // TODO: file type/ext?
} TextureMetadata;

enum
{
    eRenderPass_ZPrePass = 0,
    eRenderPass_MainView,
    eRenderPass_Max
};

typedef struct game_graphics_data
{
    ResourceHandle m_rtColorHandle;
    ResourceHandle m_rtDepthHandle;
    FramebufferHandle m_framebufferHandles[eRenderPass_Max];

    ShaderHandle m_shaderHandles[eRenderPass_Max];
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

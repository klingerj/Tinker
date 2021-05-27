#pragma once

#include "Core/CoreDefines.h"
#include "Core/Math/VectorTypes.h"
#include "PlatformGameAPI.h"

// Buffer that has no persistent staging buffer
// Meant to updated once with a staging buffer which should
// then be destroyed.
typedef struct static_buffer_data
{
    Tk::Platform::ResourceHandle gpuBufferHandle;
} StaticBuffer;

// Buffer that has a constantly mapped staging buffer
// Likely is updated frame-to-frame
typedef struct dynamic_buffer_data
{
    Tk::Platform::ResourceHandle gpuBufferHandle;
    Tk::Platform::ResourceHandle stagingBufferHandle;
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
    v3ui m_dims;
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
    Tk::Platform::ResourceHandle m_rtColorHandle;
    Tk::Platform::ResourceHandle m_rtDepthHandle;
    Tk::Platform::FramebufferHandle m_framebufferHandles[eRenderPass_Max];

    Tk::Platform::ShaderHandle m_shaderHandles[eRenderPass_Max];

    Tk::Platform::DescriptorHandle m_DescData_Instance;
    Tk::Platform::ResourceHandle m_DescDataBufferHandle_Instance;
    void* m_DescDataBufferMemPtr_Instance;

    Tk::Platform::DescriptorHandle m_DescData_Global;
    Tk::Platform::ResourceHandle m_DescDataBufferHandle_Global;
    void* m_DescDataBufferMemPtr_Global;

    Tk::Platform::ShaderHandle m_blitShaderHandle;
    Tk::Platform::DescriptorHandle m_swapChainBlitDescHandle;
} GameGraphicsData;

typedef struct descriptor_instance_data
{
    alignas(16) m4f modelMatrix;
} DescriptorData_Instance;

typedef struct descriptor_global_data
{
    alignas(16) m4f viewProj;
} DescriptorData_Global;

template <uint32 numPoints, uint32 numIndices>
struct default_geometry
{
    StaticBuffer m_positionBuffer;
    StaticBuffer m_uvBuffer;
    StaticBuffer m_normalBuffer;
    StaticBuffer m_indexBuffer;
    v4f m_points[numPoints];
    v2f m_uvs[numPoints];
    v3f m_normals[numPoints];
    uint32 m_indices[numIndices];
};

// Default geometry
template <uint32 numPoints, uint32 numIndices>
using DefaultGeometry = struct default_geometry<numPoints, numIndices>;

#define DEFAULT_QUAD_NUM_VERTICES 4
#define DEFAULT_QUAD_NUM_INDICES 6
extern DefaultGeometry<DEFAULT_QUAD_NUM_VERTICES, DEFAULT_QUAD_NUM_INDICES> defaultQuad;

void CreateDefaultGeometry(const Tk::Platform::PlatformAPIFuncs* platformFuncs, Tk::Platform::GraphicsCommandStream* graphicsCommandStream);
void DestroyDefaultGeometry(const Tk::Platform::PlatformAPIFuncs* platformFuncs);


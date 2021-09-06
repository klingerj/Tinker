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

typedef struct static_mesh_data
{
    Tk::Platform::DescriptorHandle m_descriptor;
    StaticBuffer m_positionBuffer;
    StaticBuffer m_uvBuffer;
    StaticBuffer m_normalBuffer;
    StaticBuffer m_indexBuffer;
    uint32 m_numIndices;
} StaticMeshData;

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

struct TransientPrim
{
    Tk::Platform::ResourceHandle indexBufferHandle;
    Tk::Platform::ResourceHandle vertexBufferHandle;
    Tk::Platform::DescriptorHandle descriptor;
    uint32 numVertices;
};

void CreateAnimatedPoly(const Tk::Platform::PlatformAPIFuncs* platformFuncs, TransientPrim* prim);
void DestroyAnimatedPoly(const Tk::Platform::PlatformAPIFuncs* platformFuncs, TransientPrim* prim);
void UpdateAnimatedPoly(const Tk::Platform::PlatformAPIFuncs* platformFuncs, TransientPrim* prim);
void DrawAnimatedPoly(TransientPrim* prim, Tk::Platform::DescriptorHandle globalData, uint32 shaderID, uint32 blendState, uint32 depthState, Tk::Platform::GraphicsCommandStream* graphicsCommandStream);

typedef struct game_graphics_data
{
    Tk::Platform::ResourceHandle m_rtColorHandle;
    Tk::Platform::ResourceHandle m_rtDepthHandle;
    Tk::Platform::FramebufferHandle m_framebufferHandles[eRenderPass_Max];

    Tk::Platform::DescriptorHandle m_DescData_Instance;
    Tk::Platform::ResourceHandle m_DescDataBufferHandle_Instance;
    void* m_DescDataBufferMemPtr_Instance;

    Tk::Platform::DescriptorHandle m_DescData_Global;
    Tk::Platform::ResourceHandle m_DescDataBufferHandle_Global;
    void* m_DescDataBufferMemPtr_Global;

    Tk::Platform::DescriptorHandle m_swapChainBlitDescHandle;

    TransientPrim m_animatedPolygon;
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
    Tk::Platform::DescriptorHandle m_descriptor;
    v4f m_points[numPoints];
    v2f m_uvs[numPoints];
    v4f m_normals[numPoints];
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

template <typename DefGeom>
void DestroyDefaultGeometryVertexBufferDescriptor(DefGeom& geom, const Tk::Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyDescriptor(geom.m_descriptor);
    geom.m_descriptor = Platform::DefaultDescHandle_Invalid;
}

template <typename DefGeom>
void CreateDefaultGeometryVertexBufferDescriptor(DefGeom& geom, const Tk::Platform::PlatformAPIFuncs* platformFuncs)
{
    geom.m_descriptor = platformFuncs->CreateDescriptor(DESCLAYOUT_ID_ASSET_VBS);

    Platform::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    descDataHandles[0].InitInvalid();
    descDataHandles[1].InitInvalid();
    descDataHandles[1].handles[0] = geom.m_positionBuffer.gpuBufferHandle;
    descDataHandles[1].handles[1] = geom.m_uvBuffer.gpuBufferHandle;
    descDataHandles[1].handles[2] = geom.m_normalBuffer.gpuBufferHandle;
    descDataHandles[2].InitInvalid();

    Platform::DescriptorHandle descHandles[MAX_BINDINGS_PER_SET] = { Platform::DefaultDescHandle_Invalid, geom.m_descriptor, Platform::DefaultDescHandle_Invalid };
    platformFuncs->WriteDescriptor(DESCLAYOUT_ID_ASSET_VBS, &descHandles[0], &descDataHandles[0]);
}


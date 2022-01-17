#pragma once

#include "CoreDefines.h"
#include "Math/VectorTypes.h"
#include "Platform/PlatformGameAPI.h"
#include "Graphics/Common/GraphicsCommon.h"

typedef struct mesh_attribute_data
{
    uint32 m_numVertices;
    uint8* m_vertexBufferData_Pos;
    uint8* m_vertexBufferData_UV;
    uint8* m_vertexBufferData_Normal;
    uint8* m_vertexBufferData_Index;
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
    Tk::Core::Graphics::ResourceHandle indexBufferHandle;
    Tk::Core::Graphics::ResourceHandle vertexBufferHandle;
    Tk::Core::Graphics::DescriptorHandle descriptor;
    uint32 numVertices;
};

void CreateAnimatedPoly(TransientPrim* prim);
void DestroyAnimatedPoly(TransientPrim* prim);
void UpdateAnimatedPoly(TransientPrim* prim);
void DrawAnimatedPoly(TransientPrim* prim, Tk::Core::Graphics::DescriptorHandle globalData, uint32 shaderID, uint32 blendState, uint32 depthState, Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream);

typedef struct game_graphics_data
{
    Tk::Core::Graphics::ResourceHandle m_rtColorHandle;
    Tk::Core::Graphics::ResourceHandle m_rtDepthHandle;
    Tk::Core::Graphics::FramebufferHandle m_framebufferHandles[eRenderPass_Max];

    Tk::Core::Graphics::DescriptorHandle m_DescData_Instance;
    Tk::Core::Graphics::ResourceHandle m_DescDataBufferHandle_Instance;
    void* m_DescDataBufferMemPtr_Instance;

    Tk::Core::Graphics::DescriptorHandle m_DescData_Global;
    Tk::Core::Graphics::ResourceHandle m_DescDataBufferHandle_Global;
    void* m_DescDataBufferMemPtr_Global;

    Tk::Core::Graphics::DescriptorHandle m_swapChainBlitDescHandle;

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
    Tk::Core::Graphics::ResourceHandle m_positionBuffer;
    Tk::Core::Graphics::ResourceHandle m_uvBuffer;
    Tk::Core::Graphics::ResourceHandle m_normalBuffer;
    Tk::Core::Graphics::ResourceHandle m_indexBuffer;
    Tk::Core::Graphics::DescriptorHandle m_descriptor;
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

void CreateDefaultGeometry(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream);
void DestroyDefaultGeometry();

template <typename DefGeom>
void DestroyDefaultGeometryVertexBufferDescriptor(DefGeom& geom)
{
    Graphics::DestroyDescriptor(geom.m_descriptor);
    geom.m_descriptor = Core::Graphics::DefaultDescHandle_Invalid;
}

template <typename DefGeom>
void CreateDefaultGeometryVertexBufferDescriptor(DefGeom& geom)
{
    geom.m_descriptor = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_ASSET_VBS);

    Core::Graphics::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    descDataHandles[0].InitInvalid();
    descDataHandles[0].handles[0] = geom.m_positionBuffer;
    descDataHandles[0].handles[1] = geom.m_uvBuffer;
    descDataHandles[0].handles[2] = geom.m_normalBuffer;
    descDataHandles[1].InitInvalid();
    descDataHandles[2].InitInvalid();

    Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_ASSET_VBS, geom.m_descriptor, &descDataHandles[0], 1);
}


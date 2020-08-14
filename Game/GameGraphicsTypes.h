#pragma once

typedef struct game_graphic_data
{
    uint32 m_vertexBufferHandle;
    uint32 m_stagingBufferHandle;
    void* m_stagingBufferMemPtr;
    uint32 m_indexBufferHandle;
    uint32 m_stagingBufferHandle3;
    void* m_stagingBufferMemPtr3;

    uint32 m_vertexBufferHandle2;
    uint32 m_stagingBufferHandle2;
    void* m_stagingBufferMemPtr2;
    uint32 m_indexBufferHandle2;
    uint32 m_stagingBufferHandle4;
    void* m_stagingBufferMemPtr4;

    uint32 m_imageHandle;
    uint32 m_imageViewHandle;
    uint32 m_framebufferHandle;

    uint32 m_shaderHandle;
    uint32 m_mainRenderPassHandle;
} GameGraphicsData;

template <uint32 numPoints, uint32 numIndices>
struct default_geometry
{
    uint32 m_vertexBufferHandle;
    uint32 m_indexBufferHandle;
    uint32 m_stagingBufferHandle_vert;
    uint32 m_stagingBufferHandle_idx;
    v4f m_points[numPoints];
    uint32 m_indices[numIndices];
};

template <uint32 numPoints, uint32 numIndices>
using DefaultGeometry = struct default_geometry<numPoints, numIndices>;

extern DefaultGeometry<4, 6> defaultQuad;


#pragma once

#include "Core/Math/VectorTypes.h"
using namespace Tinker;
using namespace Core;
using namespace Math;

typedef struct virtual_camera_data
{
    v3f m_eye;
    v3f m_ref;
} VirtualCamera;

static const v3f WORLD_UP = v3f(0, 0, 1);
static float fovy = 0.785398f;
static const float nearPlane = 0.1f;
static const float farPlane = 1000.0f;
static m4f g_projMat = m4f(1.0f);

inline m4f CameraViewMatrix(const VirtualCamera* camera)
{
    v3f forward = camera->m_ref - camera->m_eye;
    Normalize(forward);

    v3f right = Cross(forward, WORLD_UP);
    Normalize(right);
    v3f up = Cross(right, forward);
    Normalize(up);

    m4f view;
    view[0] = v4f(right.x, up.x, forward.x, 0.0f);
    view[1] = v4f(right.y, up.y, forward.y, 0.0f);
    view[2] = v4f(right.z, up.z, forward.z, 0.0f);
    view[3] = v4f(-Dot(camera->m_eye, right), -Dot(camera->m_eye, up), -Dot(camera->m_eye, forward), 1.0f);
    return view;
}

inline m4f PerspectiveProjectionMatrix(float aspect)
{
    m4f proj;

    const float tanFov = tanf(fovy * 0.5f);

    proj[0][0] = 1.0f / (aspect * tanFov); proj[1][0] = 0.0f;          proj[2][0] = 0.0f;                                             proj[3][0] = 0.0f;
    proj[0][1] = 0.0f;                     proj[1][1] = 1.0f / tanFov; proj[2][1] = 0.0f;                                             proj[3][1] = 0.0f;
    proj[0][2] = 0.0f;                     proj[1][2] = 0.0f;          proj[2][2] = (-nearPlane - farPlane) / (nearPlane - farPlane); proj[3][2] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    proj[0][3] = 0.0f;                     proj[1][3] = 0.0f;          proj[2][3] = 1.0f;                                             proj[3][3] = 0.0f;

    return proj;
}

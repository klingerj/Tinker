#pragma once

#include "Core/Math/VectorTypes.h"

typedef struct camera_data
{
    v3f m_eye;
    v3f m_ref;
} Camera;

static const v3f WORLD_UP = v3f(0, 0, 1);
static const float fovy = 0.785398f;
static const float nearPlane = 0.1f;
static const float farPlane = 1000.0f;
static m4f g_projMat = m4f(1.0f);

m4f CameraViewMatrix(const Camera* camera);
m4f PerspectiveProjectionMatrix(float aspect);

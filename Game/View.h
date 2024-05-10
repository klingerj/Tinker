#pragma once

#include "Platform/PlatformGameAPI.h"
#include "Math/VectorTypes.h"
#include "GraphicsTypes.h"

struct View
{
    alignas(CACHE_LINE) m4f m_viewMatrix;
    alignas(CACHE_LINE) m4f m_projMatrix;
    alignas(CACHE_LINE) m4f m_viewProjMatrix;

    void Init() {}
    void Update();
};


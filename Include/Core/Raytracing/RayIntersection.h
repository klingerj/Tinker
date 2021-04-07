#pragma once

#include "Core/Math/VectorTypes.h"

namespace Tinker
{
namespace Core
{
using namespace Math;

namespace Raytracing
{

struct Ray
{
    v3f origin;
    v3f dir;
};

struct Intersection
{
    uint32 hitTri;
    float t;
    float bary;

    void InitInvalid()
    {
        hitTri = MAX_UINT32;
        t = -1.0f;
        bary = 0.0f;
    }
};

float IntersectRayTriangle(const Ray& ray, const v3f* triangle);
//float IntersectRayCube(const Ray& ray, const Box& box);

}
}
}

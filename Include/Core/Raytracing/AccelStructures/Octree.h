#pragma once

#include "Core/CoreDefines.h"
#include "Core/Math/VectorTypes.h"
#include "Core/Raytracing/RayIntersection.h"

namespace Tinker
{
namespace Core
{
using namespace Math;

namespace Raytracing
{

struct Octree;
Octree* CreateEmptyOctree();
void BuildOctree(Octree* octreeToBuild, const v3f* triangleData, uint32 numTris);
Intersection IntersectRay(Octree* octree, const Ray& ray);


}
}
}


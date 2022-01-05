#pragma once

#include "CoreDefines.h"
#include "Math/VectorTypes.h"
#include "Raytracing/RayIntersection.h"

namespace Tk
{
namespace Core
{
namespace Raytracing
{

struct Octree;
TINKER_API Octree* CreateEmptyOctree();
TINKER_API void DestroyOctree(Octree* octree);
TINKER_API void BuildOctree(Octree* octreeToBuild, const v3f* triangleData, uint32 numTris);
TINKER_API Intersection IntersectRay(Octree* octree, const Ray& ray);

}
}
}


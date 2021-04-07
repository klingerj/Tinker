#pragma once

#include "Core/CoreDefines.h"
#include "Core/Math/VectorTypes.h"

namespace Tinker
{
namespace Core
{
using namespace Math;

namespace Raytracing
{

struct Octree;
Octree* CreateEmptyOctree();
void BuildOctree(Octree* octreeToBuild, v3f* triangleData, uint32 numTris);
// TODO: functions to intersect with octree


}
}
}


#pragma once

#include "Core/Raytracing/RayIntersection.h"
#include "PlatformGameAPI.h"

using namespace Tinker;
using namespace Core;
using namespace Math;

void RayMeshIntersectNaive(Raytracing::Intersection& isx);
void RaytraceTest(const Tinker::Platform::PlatformAPIFuncs* platformFuncs);

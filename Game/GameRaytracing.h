#pragma once

#include "Core/Raytracing/RayIntersection.h"
#include "PlatformGameAPI.h"

using namespace Tk;
using namespace Core;
using namespace Math;

void RayMeshIntersectNaive(Raytracing::Intersection& isx);
void RaytraceTest(const Tk::Platform::PlatformAPIFuncs* platformFuncs);

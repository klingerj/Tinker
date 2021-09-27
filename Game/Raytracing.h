#pragma once

#include "Raytracing/RayIntersection.h"
#include "Platform/PlatformGameAPI.h"

void RayMeshIntersectNaive(Tk::Core::Raytracing::Intersection& isx);
void RaytraceTest(const Tk::Platform::PlatformAPIFuncs* platformFuncs);

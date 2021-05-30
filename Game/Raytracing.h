#pragma once

#include "Core/Raytracing/RayIntersection.h"
#include "PlatformGameAPI.h"

void RayMeshIntersectNaive(Tk::Core::Raytracing::Intersection& isx);
void RaytraceTest(const Tk::Platform::PlatformAPIFuncs* platformFuncs);
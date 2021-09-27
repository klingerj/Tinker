#include "Raytracing.h"
#include "Mem.h"
#include "FileLoading.h"
#include "Raytracing/AccelStructures/Octree.h"
#include "Utility/ScopedTimer.h"
#include "GraphicsTypes.h"
#include "Camera.h"
#include "AssetManager.h"

#include <string.h>

void RayMeshIntersectNaive(const Tk::Core::Raytracing::Ray& ray, v3f* triData, uint32 numTris, Tk::Core::Raytracing::Intersection& isx)
{
    Tk::Core::Raytracing::Intersection newIsx;
    for (uint32 uiTri = 0; uiTri < numTris; ++uiTri)
    {
        newIsx.InitInvalid();
        Tk::Core::Raytracing::IntersectRayTriangle(ray, &triData[uiTri * 3], newIsx);

        if (newIsx.t > 0.0f)
        {
            if (isx.t < 0.0f || (isx.t > 0.0f && newIsx.t < isx.t))
            {
                memcpy(&isx, &newIsx, sizeof(Tk::Core::Raytracing::Intersection));
                isx.hitTri = uiTri;
            }
        }
    }
}

void RaytraceTest(const Tk::Platform::PlatformAPIFuncs* platformFuncs)
{
    TIMED_SCOPED_BLOCK("Raytrace test");

    const bool useOctree = false;

    const MeshAttributeData& data = g_AssetManager.GetMeshAttrDataByID(2);
    uint32 numVerts =  data.m_numVertices;
    v3f* triData = (v3f*)Tk::Core::CoreMalloc(numVerts * sizeof(v3f));
    for (uint32 i = 0; i < numVerts; ++i)
    {
        const v4f& ptVec4 = ((v4f*)data.m_vertexBufferData)[i];
        triData[i] = v3f(ptVec4.x, ptVec4.y, ptVec4.z);
    }
    
    Tk::Core::Raytracing::Octree* octree = nullptr;
    if (useOctree)
    {
        //octree = Tk::Core::Raytracing::CreateEmptyOctree();
        //Tk::Core::Raytracing::BuildOctree(octree, triData, numVerts/3);
    }

    const uint32 width = 256;
    const uint32 height = 256;
    uint32* img = (uint32*)Tk::Core::CoreMalloc(sizeof(uint32) * width * height);
    memset(img, 0, width * height * sizeof(uint32));

    v3f camEye = v3f(27, 27, 27);
    v3f camRef = v3f(0, 0, 0);
    float aspect = (float)width / height;
    float tanFov = tanf(fovy * 0.5f);

    for (uint32 px = 0; px < width; ++px)
    {
        for (uint32 py = 0; py < height; ++py)
        {
            v3f rayOrigin = v3f();
            v3f rayDir = v3f();
            
            // Cast ray from camera
            // TODO: move this
            {
                v4f coord = v4f((float)px, (float)py, 1.0, 1.0);
                coord.x /= width;
                coord.y /= height;

                // shift by half a pixel
                coord.x += 0.5f / width;
                coord.y += 0.5f / height;
                coord.x = coord.x * 2 - 1;
                coord.y = coord.y * 2 - 1;

                rayOrigin = camEye;
                v3f look = camRef - camEye;
                float len = Length(look);
                Normalize(look);
                v3f right = Cross(look, v3f(0, 0, 1));
                Normalize(right);
                v3f up = Cross(right, look);
                Normalize(up);

                v3f H = right * len * tanFov * aspect;
                v3f V = up * len * tanFov;

                v3f screenPt = camRef + H * coord.x + V * coord.y;
                rayDir = screenPt - camEye;
                Normalize(rayDir);
            }
            
            Tk::Core::Raytracing::Ray ray;
            ray.origin = rayOrigin;
            ray.dir = rayDir;

            uint8 channel[4] = {};
            channel[3] = 255;

            Tk::Core::Raytracing::Intersection isx;
            isx.InitInvalid();
            if (useOctree)
            {
                // intersect with octree
                isx = Tk::Core::Raytracing::IntersectRay(octree, ray);
            }
            else
            {
                RayMeshIntersectNaive(ray, triData, numVerts / 3, isx);
            }

            if (isx.t > 0.0f)
            {
                v3f* meshNormals = (v3f*)(data.m_vertexBufferData + numVerts * (sizeof(v4f) + sizeof(v2f))); // skip pos and uvs in buffer
                v3f interpNormal = meshNormals[isx.hitTri * 3] * isx.bary[0] +
                    meshNormals[isx.hitTri * 3 + 1] * isx.bary[1] +
                    meshNormals[isx.hitTri * 3 + 2] * isx.bary[2];
                Normalize(interpNormal);

                v3f lightDir = v3f(-1, -1, 1);
                Normalize(lightDir);
                float lambert = Dot(interpNormal, lightDir);
                lambert *= 0.7f;
                lambert = max(0.05f, lambert);
                // Linear to SRGB
                if (lambert > 0.0031308f)
                {
                    lambert = 1.055f * (powf(lambert, 1.0f/2.4f)) - 0.055f;
                }
                else
                {
                    lambert = 12.92f * lambert;
                }

                uint8 asUnorm = (uint8)(lambert * 255);
                channel[0] = asUnorm;
                channel[1] = asUnorm;
                channel[2] = asUnorm;

                // BGRA
                img[py * width + px] = (uint32)(channel[2] | (channel[1] << 8) | (channel[0] << 16) | (channel[3] << 24));
            }
            else
            {
                // No intersection
            }
        }
    }

    // Output image
    Buffer imgBuffer = {};
    Tk::Core::FileLoading::SaveBMP(&imgBuffer, (uint8*)img, width, height, 32);
    platformFuncs->WriteEntireFile("..\\Output\\TestImages\\raytraceOutput.bmp", imgBuffer.m_sizeInBytes, imgBuffer.m_data);
    imgBuffer.Dealloc();
    Tk::Core::CoreFree(img);

    Tk::Core::CoreFree(triData);
}

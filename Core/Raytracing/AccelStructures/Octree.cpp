#include "Raytracing/AccelStructures/Octree.h"

#include <string.h>

namespace Tk
{
namespace Core
{
namespace Raytracing
{

#define MAX_TRIS_PER_NODE 14
struct OctreeNode
{
    enum : uint8
    {
        eIsLeafNode = 0x1,
    };

    union
    {
        // Intermediate node
        struct
        {
            uint16 childrenIndex;
        };

        // Leaf node
        struct
        {
            uint32 tris[MAX_TRIS_PER_NODE];
            uint8 numTris;
            uint8 leafPad[3];
        };
    };

    uint8 flags;
    uint8 nodePad[3];
};
// TODO: add a static assert that sizeof(OctreeNode) == CACHE_LINE (or a multiple)

// TODO: allocate mem more better

struct Octree
{
    enum { eRootNode = 0 };

    uint16 numNodes;
    OctreeNode nodes[65536];
    AABB3D aabbs[65536];
    const v3f* triangleData;
};

void InitLeafNode(OctreeNode& node)
{
    node.flags = 0;
    node.flags |= OctreeNode::eIsLeafNode;
    node.numTris = 0;
}

Octree* CreateEmptyOctree()
{
    Octree* newOctree = new Octree();
    memset(newOctree->nodes, 255, 65536 * sizeof(OctreeNode));
    for (uint32 i = 0; i < 65536; ++i)
    {
        newOctree->aabbs[i].InitInvalidMinMax();
    }
    InitLeafNode(newOctree->nodes[newOctree->numNodes++]);
    newOctree->numNodes = 1;
    newOctree->triangleData = nullptr;

    return newOctree;
}

void SubdivideNode(Octree* octree, uint32 nodeIndex)
{
    OctreeNode& node = octree->nodes[nodeIndex];
    node.childrenIndex = octree->numNodes;
    TINKER_ASSERT(node.childrenIndex + 7 < 65536);
    octree->numNodes += 8; // claim 8 children nodes at once

    const AABB3D& aabb = octree->aabbs[nodeIndex];
    const v3f centerDisp = 0.5f * (aabb.maxExt - aabb.minExt);

    for (uint8 i = 0; i < 8; ++i)
    {
        InitLeafNode(octree->nodes[node.childrenIndex + i]);
        v3ui octant = v3ui();
        octant.x = i & 0x1;
        octant.y = (i >> 1) & 0x1;
        octant.z = (i >> 2) & 0x1;

        // Calculate new aabbs for children
        v3f disp = centerDisp;
        disp.x *= octant.x ? -1.0f : 1.0f;
        disp.y *= octant.y ? -1.0f : 1.0f;
        disp.z *= octant.z ? -1.0f : 1.0f;
        v3f corner = centerDisp + disp; // this point is the outer corner of each child node

        AABB3D& aabbChild = octree->aabbs[node.childrenIndex + i];
        aabbChild.ExpandTo(corner); // include corner
        aabbChild.ExpandTo(centerDisp); // include parent center
    }
}

void InsertTriangleIntoNode(Octree* octree, uint32 nodeIndex, uint32 triIndex)
{
    OctreeNode& node = octree->nodes[nodeIndex];

    if (node.flags & OctreeNode::eIsLeafNode)
    {
        if (node.numTris < MAX_TRIS_PER_NODE)
        {
            node.tris[node.numTris++] = triIndex;
        }
        else
        {
            node.flags &= ~OctreeNode::eIsLeafNode;
            SubdivideNode(octree, nodeIndex);
            InsertTriangleIntoNode(octree, nodeIndex, triIndex);
        }
    }
    else
    {
        // Intermediate node - already been subdivided

        // 1. figure out which children node(s) the triangle lies in
        /* Do this by getting the plane equation for each separating plane and evaluating it with
         * each triangle point. This could probably be fast with SIMD, so I guess we can just accept v4fs
         * into these functions. Additionally, i think the data might need to be laid out like:
         * x1 x2 x3 x4 y1 y2 y3 y4 z1 z2 z3 z4
         * which will require additional preprocessing, probably by the user I guess.
         * Alternatively, we could evaluate the cuboid SDF, but that would probably be slower, and doesn't seem
         * as friendly to SIMD-ify.
         * Can side-by-side that with the non-SIMD version of the above as a test though.
        */

        if (1)
        {
            // Add triangle to child octants that the tri intersects
            // NOTE: this is done by checking if the triangle's aabb intersects the octant AABB. This can add a triangle
            // to an octant that is does not actually lie in, which is wasteful, but shouldn't affect correctness ultimately.
            AABB3D triAABB;
            triAABB.InitInvalidMinMax();
            for (uint32 i = 0; i < 3; ++i)
            {
                triAABB.ExpandTo(octree->triangleData[triIndex + i]);
            }

            for (uint32 i = 0; i < 8; ++i)
            {
                if (triAABB.Intersects(octree->aabbs[node.childrenIndex + i]))
                {
                    InsertTriangleIntoNode(octree, node.childrenIndex + i, triIndex);
                }
            }
        }
        else
        {
            // Determine octant of each triangle point
            v3ui octants[3] = {};
            for (uint32 i = 0; i < 3; ++i)
            {
                TINKER_ASSERT(octree->triangleData);
                const v3f& triPt = octree->triangleData[triIndex + i];
                v3f aabbCenter = 0.5f * (octree->aabbs[nodeIndex].minExt + octree->aabbs[nodeIndex].maxExt);
                v3f disp = triPt - aabbCenter;
                octants[i].x = (uint32)signbit(Dot(disp, v3f(1, 0, 0)) - aabbCenter.x);
                octants[i].y = (uint32)signbit(Dot(disp, v3f(0, 1, 0)) - aabbCenter.y);
                octants[i].z = (uint32)signbit(Dot(disp, v3f(0, 0, 1)) - aabbCenter.z);
            }


            // Insert this triangle into each octant
            uint16 octant0 = (uint16)(octants[0].x + (octants[0].y << 1) + (octants[0].z << 2));
            InsertTriangleIntoNode(octree, node.childrenIndex + octant0, triIndex);

            uint16 octant1 = (uint16)(octants[1].x + (octants[1].y << 1) + (octants[1].z << 2));
            if (octant1 != octant0)
            {
                InsertTriangleIntoNode(octree, node.childrenIndex + octant1, triIndex);
            }

            uint16 octant2 = (uint16)(octants[2].x + (octants[2].y << 1) + (octants[2].z << 2));
            if (octant2 != octant0 && octant2 != octant1)
            {
                InsertTriangleIntoNode(octree, node.childrenIndex + octant2, triIndex);
            }
        }
    }
}

void BuildOctree(Octree* octreeToBuild, const v3f* triangleData, uint32 numTris)
{
    octreeToBuild->triangleData = triangleData;

    // Init root node to be large enough to contain all triangle points
    for (uint32 i = 0; i < numTris; i += 3)
    {
        octreeToBuild->aabbs[Octree::eRootNode].ExpandTo(triangleData[i]);
        octreeToBuild->aabbs[Octree::eRootNode].ExpandTo(triangleData[i + 1]);
        octreeToBuild->aabbs[Octree::eRootNode].ExpandTo(triangleData[i + 2]);
    }

    // Insert all triangles into octree
    for (uint32 i = 0; i < numTris; i += 3)
    {
        InsertTriangleIntoNode(octreeToBuild, Octree::eRootNode, i);
    }
}

Intersection IntersectRayWithNode(const Octree* octree, uint32 nodeIndex, const Ray& ray)
{
    Intersection isx;
    isx.InitInvalid();

    const OctreeNode& node = octree->nodes[nodeIndex];
    const AABB3D& aabb = octree->aabbs[nodeIndex];

    // TODO: ray-cuboid intersection using aabb data

    return isx;
}

Intersection IntersectRay(Octree* octree, const Ray& ray)
{
    Intersection isx = IntersectRayWithNode(octree, Octree::eRootNode, ray);
    return isx;
}

}
}
}


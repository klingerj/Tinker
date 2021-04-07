#include "Core/Raytracing/AccelStructures/Octree.h"

#include <string.h>

namespace Tinker
{
namespace Core
{
using namespace Math;

namespace Raytracing
{

#define MAX_TRIS_PER_NODE 2
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
        };
    };

    uint8 flags;
};

// TODO: allocate mem more better

struct Octree
{
    enum { eRootNode = 0 };

    uint16 numNodes;
    OctreeNode nodes[65536];
    AABB3D aabbs[65536];
};

void InitLeafNode(OctreeNode& node)
{
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

    return newOctree;
}

void SubdivideNode(Octree* octree, uint32 nodeIndex)
{
    OctreeNode& node = octree->nodes[nodeIndex];
    node.childrenIndex = octree->numNodes;
    octree->numNodes += 8; // claim 8 children nodes at once

    for (uint8 i = 0; i < 8; ++i)
    {
        InitLeafNode(octree->nodes[i]);
        v3ui octant = v3ui();
        octant.x = i & 0x1;
        octant.y = (i >> 1) & 0x1;
        octant.z = (i >> 2) & 0x1;

        // Calculate new aabbs for children
        const AABB3D& aabb = octree->aabbs[nodeIndex];
        v3f disp = 0.5f * (aabb.maxExt - aabb.minExt);
        disp.x *= octant.x ? -1.0f : 1.0f;
        disp.y *= octant.y ? -1.0f : 1.0f;
        disp.z *= octant.z ? -1.0f : 1.0f;

        AABB3D& aabbChild = octree->aabbs[node.childrenIndex + i];
        aabbChild.minExt = aabb.minExt + disp;
        aabbChild.maxExt = aabbChild.minExt + disp;
    }
}

void InsertTriangleIntoNode(Octree* octree, uint32 nodeIndex, const v3f* triangleData, uint32 triIndex)
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
        }
    }
    else
    {
        // Intermediate node - already been subdivided
        // TODO:

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

        // Determine octant of each triangle point
        v3ui octants[3] = {};
        for (uint32 i = 0; i < 3; ++i)
        {
            const v3f& triPt = triangleData[triIndex + i];
            v3f aabbCenter = 0.5f * (octree->aabbs[nodeIndex].minExt + octree->aabbs[nodeIndex].maxExt);
            v3f disp = triPt - aabbCenter;
            octants[i].x = (uint32)signbit(Dot(disp, v3f(1, 0, 0)) - aabbCenter.x);
            octants[i].y = (uint32)signbit(Dot(disp, v3f(0, 1, 0)) - aabbCenter.y);
            octants[i].z = (uint32)signbit(Dot(disp, v3f(0, 0, 1)) - aabbCenter.z);
        }
        
        // Insert this triangle into each octant
        uint16 octant0 = (uint16)(octants[0].x + (octants[0].y << 1) + (octants[0].z << 2));
        InsertTriangleIntoNode(octree, node.childrenIndex + octant0, triangleData, triIndex);

        uint16 octant1 = (uint16)(octants[1].x + (octants[1].y << 1) + (octants[1].z << 2));
        if (octant1 != octant0)
        {
            InsertTriangleIntoNode(octree, node.childrenIndex + octant1, triangleData, triIndex);
        }

        uint16 octant2 = (uint16)(octants[2].x + (octants[2].y << 1) + (octants[2].z << 2));
        if (octant2 != octant0 && octant2 != octant1)
        {
            InsertTriangleIntoNode(octree, node.childrenIndex + octant2, triangleData, triIndex);
        }
    }
}

void BuildOctree(Octree* octreeToBuild, v3f* triangleData, uint32 numTris)
{
    // Init root node to be large enough to contain all triangle points
    for (uint32 i = 0; i < numTris; ++i)
    {
        octreeToBuild->aabbs[Octree::eRootNode].ExpandTo(triangleData[i]);
    }

    // Insert all triangles into octree
    for (uint32 i = 0; i < numTris; i += 3)
    {
        InsertTriangleIntoNode(octreeToBuild, Octree::eRootNode, triangleData, i);
    }
}

}
}
}


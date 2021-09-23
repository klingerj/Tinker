#version 450

// Virtual texturing resources
layout(set = 2, binding = 0) uniform sampler2DArray PageTextures;

// NOTE: These definitions must match the definitions in the engine.
#define MAX_VIRTUAL_PAGES 128
struct PageTableEntry
{
    uint frameIndex;
};
layout(set = 2, binding = 1) uniform DescPageTable
{
    // -1 == invalid / not in physical memory
    // -2 == use fallback
    uvec4 data[MAX_VIRTUAL_PAGES];
} PageTable;

layout(set = 2, binding = 2) uniform sampler2D Fallback;

// Plane/terrain tiles data
layout(set = 3, binding = 0) uniform DescData
{
    uvec2 numTerrainTiles;
} TerrainData;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

#define LIGHT_DIR normalize(vec3(-1, -1, 1))
#define AMBIENT 0.05

void main()
{
    float lambert = dot(LIGHT_DIR, normalize(vec3(0, 0, 1)));
    float brightness = clamp(lambert, AMBIENT, 1.0);
    //outColor = vec4(inUV, 0, 1); return;
    //vec4 color = texture(Fallback, inUV);
    //outColor = vec4((color.rgb * brightness), color.a);
    //return;

    // Read from virtual page table
    // 1. determine which page to read from
    uvec2 pageIndex2D;
    pageIndex2D.x = uint(floor(inUV.x * TerrainData.numTerrainTiles.x));
    pageIndex2D.y = uint(floor(inUV.y * TerrainData.numTerrainTiles.y));
    uint pageIndex1D = pageIndex2D.x + pageIndex2D.y * TerrainData.numTerrainTiles.x;

    uint frame = PageTable.data[pageIndex1D].x;
    if (frame == -1)
    {
        outColor = vec4(1, 0, 1, 1); // invalid frame, not in memory
    }
    else if (frame == -2)
    {
        // read from fallback
        vec4 color = texture(Fallback, inUV);
        outColor = vec4((color.rgb * brightness), color.a);
    }
    else
    {
        // read from valid frame

        vec4 color = texture(PageTextures, vec3(inUV, frame));
        outColor = vec4(color.rgb * brightness, color.a);
    }

    // debug:
    //outColor = vec4(frame, 0, 0, 1.0);
}

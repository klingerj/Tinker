#version 450

// Virtual texturing resources
layout(set = 3, binding = 0) uniform sampler2D PageTextures;

// NOTE: These definitions must match the definitions in the engine.
#define MAX_VIRTUAL_PAGES 128
struct PageTableEntry
{
    // -1 == invalid / not in physical memory
    // -2 == use fallback
    uint frameIndex;
};
layout(set = 3, binding = 0) uniform DescPageTable
{
    PageTableEntry data[MAX_VIRTUAL_PAGES];
    vec4 PageDims; // xy is single page dims, zw is total page texture dims
} PageTable;

layout(set = 3, binding = 2) uniform sampler2D Fallback;

// Plane/terrain tiles data
layout(set = 4, binding = 0) uniform DescData
{
    uvec2 numTerrainTiles;
} TerrainData;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 0) out vec4 outColor;

#define LIGHT_DIR normalize(vec3(-1, -1, 1))
#define AMBIENT 0.05

void main()
{
    float lambert = dot(LIGHT_DIR, normalize(inNormal));
    float brightness = clamp(lambert, AMBIENT, 1.0);

    // Read from virtual page table
    // 1. determine which page to read from
    uvec2 pageIndex2D;
    pageIndex2D.x = uint(floor(inUV.x * TerrainData.numTerrainTiles.x));
    pageIndex2D.y = uint(floor(inUV.y * TerrainData.numTerrainTiles.y));
    uint pageIndex1D = pageIndex2D.x + pageIndex2D.y * TerrainData.numTerrainTiles.x;

    uint frame = PageTable.data[pageIndex1D].frameIndex;
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

        // Calculate offset into texture of pages
        vec2 uvScaled = inUV * (1.0 / PageTable.PageDims.xy);
        vec2 uvOffset = uvScaled + PageTable.PageDims.xy * pageIndex2D / PageTable.PageDims.zw;

        vec4 color = texture(PageTextures, uvOffset);
        outColor = vec4(color.rgb * brightness, color.a);
    }

    // debug:
    //outColor = vec4(frame, 0, 0, 1.0);
}

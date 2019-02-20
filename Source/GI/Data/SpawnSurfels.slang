import GICommon;

StructuredBuffer<uint2> gSurfelSpawnCoords;
RWStructuredBuffer<uint> gIndices;
RWStructuredBuffer<uint> gNewSurfelsCount;
Buffer<uint> gSurfelCount;

[numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint dim;
    uint stride;
    Data.Surfels.Storage.GetDimensions(dim, stride);

    uint currentCount = Data.Surfels.Count[0];

    if (currentCount + tid.x >= dim)
        return;

    uint2 screenPos = gSurfelSpawnCoords[tid.x];

    // Create new Surfel
    Surfel surfel;
    surfel.Position = GetWorldPosition(screenPos);
    surfel.Normal = GetNormal(screenPos);
    surfel.Color = float3(0.0f, 0.0f, 0.0f);

    Data.Surfels.Storage[currentCount + tid.x] = surfel;

    uint3 index = GetWorldStructureIndex(surfel.Position);
    uint flatIndex = FlattenWorldIndex(index);
    uint oldValue;
    InterlockedAdd(gNewSurfelsCount[flatIndex], -1, oldValue);
    gIndices[Data.Surfels.WorldStructure[flatIndex].StartIndex + oldValue - 1] = currentCount + tid.x;

    // Check surrounding chunks
    const float3 chunkCenter = GetChunkCenter(index);
    const float3 posInChunk = surfel.Position - chunkCenter;
    const float d = WORLD_STRUCTURE_CHUNK_SIZE / 2.0f;
    // first 8 check for diagonal cornered chunks
    // We need to check the center chunk corners
    float3 corners[8] =
    {
        float3( 1,  1,  1),
        float3( 1,  1, -1),
        float3( 1, -1, -1),
        float3( 1, -1,  1),
        float3(-1, -1,  1),
        float3(-1, -1, -1),
        float3(-1,  1, -1),
        float3(-1,  1,  1)
    };
    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        float3 v = corners[i] * d - posInChunk;
        if (dot(v, v) < SurfelRadiusSquared)
        {
            uint flatIndex = FlattenWorldIndex(index + corners[i]);
            InterlockedAdd(gNewSurfelsCount[flatIndex], -1, oldValue);
            gIndices[Data.Surfels.WorldStructure[flatIndex].StartIndex + oldValue - 1] = currentCount + tid.x;
        }
    }

    // Check 6 planes of the cube for intersection
    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        float v = d - posInChunk[i];
        uint3 checkedChunkIndex = uint3(0, 0, 0);
        checkedChunkIndex[i] = 1;
        if (v < SurfelRadius)
        {
            uint flatIndex = FlattenWorldIndex(index + checkedChunkIndex);
            InterlockedAdd(gNewSurfelsCount[flatIndex], -1, oldValue);
            gIndices[Data.Surfels.WorldStructure[flatIndex].StartIndex + oldValue - 1] = currentCount + tid.x;
        }


        v = -d - posInChunk[i];
        checkedChunkIndex[i] = -1;
        if (v < SurfelRadius)
        {
            uint flatIndex = FlattenWorldIndex(index + checkedChunkIndex);
            InterlockedAdd(gNewSurfelsCount[flatIndex], -1, oldValue);
            gIndices[Data.Surfels.WorldStructure[flatIndex].StartIndex + oldValue - 1] = currentCount + tid.x;
        }
    }

    // Toghether with 12 edges of the cubes
    float3 edges[12] =
    {
            float3( 0,  1,  1),
            float3( 0,  1, -1),
            float3( 0, -1, -1),
            float3( 0, -1,  1),
            float3( 1,  0,  1),
            float3( 1,  0, -1),
            float3(-1,  0, -1),
            float3(-1,  0,  1),
            float3( 1,  1,  0),
            float3( 1, -1,  0),
            float3(-1, -1,  0),
            float3(-1,  1,  0)
    };
    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        float3 relevantPos = posInChunk;
        relevantPos[i] = 0;
        [unroll]
        for (int j = 0; j < 4; ++j)
        {
            float3 v = edges[i * 4 + j] * d - relevantPos;
            if (dot(v, v) < SurfelRadiusSquared)
            {
                uint flatIndex = FlattenWorldIndex(index + edges[i * 4 + j]);
                InterlockedAdd(gNewSurfelsCount[flatIndex], -1, oldValue);
                gIndices[Data.Surfels.WorldStructure[flatIndex].StartIndex + oldValue - 1] = currentCount + tid.x;
            }
        }
    }

    if (tid.x == 0)
    {
        Data.Surfels.Count[0] = min(Data.Surfels.Count[0] + gSurfelCount[0], dim);
    }
}


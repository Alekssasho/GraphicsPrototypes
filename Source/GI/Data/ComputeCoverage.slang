__import GICommon;

RWTexture2D<float> gCoverage;

float computeCoverage(float3 posW, float3 normal, Surfel surfel)
{
    float NdotSN = dot(normal, surfel.Normal);

    // If normals are not in the same direction bail out
    if(NdotSN <= 0)
        return 0.0f;

    // Every pixel inside he surfel radius gets equal coverage
    float3 WtoS = surfel.Position - posW;
    float distanceSquared = dot(WtoS, WtoS);
    if (distanceSquared < SurfelRadiusSquared)
    {
        float coeff = 1.0f - (distanceSquared / SurfelRadiusSquared);
        return coeff * NdotSN;
    }

    return 0.0f;
}

#define BLOCK_SIZE_X 16
#define BLOCK_SIZE_Y 16

groupshared uint2 groupScreenPos[BLOCK_SIZE_X * BLOCK_SIZE_Y];
groupshared float groupCoverage[BLOCK_SIZE_X * BLOCK_SIZE_Y];

[numthreads(BLOCK_SIZE_X, BLOCK_SIZE_Y, 1)]
void main(uint3 tid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    uint numSurfels = 0;
    uint stride = 0;
    Data.surfels.SurfelStorage.GetDimensions(numSurfels, stride);

    float3 posW = GetWorldPosition(tid.xy, Data);
    float3 normal = GetNormal(tid.xy, Data);

    float coverage = 0.0f;
    for (uint i = 0; i < numSurfels; ++i)
    {
        coverage += computeCoverage(posW, normal, Data.surfels.SurfelStorage[i]);
    }

    groupCoverage[groupIndex] = coverage;
    groupScreenPos[groupIndex] = tid.xy;

    GroupMemoryBarrierWithGroupSync();

    gCoverage[tid.xy] = coverage;

    // TODO: write with Wave Instructions when we support SM 6.0
    uint threadCount = BLOCK_SIZE_X * BLOCK_SIZE_Y;
    [unroll]
    for (uint step = threadCount / 2; step > 0; step >>= 1)
    {
        if (groupIndex < step
            && (groupCoverage[groupIndex] > groupCoverage[groupIndex + step]))
        {
            groupCoverage[groupIndex] = groupCoverage[groupIndex + step];
            groupScreenPos[groupIndex] = groupScreenPos[groupIndex + step];
        }

        if(step > 32)
        {
            GroupMemoryBarrierWithGroupSync();
        }
    }

    if(groupIndex == 0)
    {
        gCoverage[groupScreenPos[0]] = 1000.0f;
    }
}

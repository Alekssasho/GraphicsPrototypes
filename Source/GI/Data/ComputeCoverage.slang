__import GICommon;
__import Random;

#define COVERAGE_THRESHOLD 0.5

RWTexture2D<float2> gCoverage;

cbuffer GlobalState
{
    float globalTime;
    float globalSpawnChance;
}

float computeCoverage(float3 posW, float3 normal, Surfel surfel)
{
    float NdotSN = dot(normal, surfel.Normal);

    // If normals are not in the same direction bail out
    if(NdotSN <= 0)
        return 0.0f;

    // Every pixel inside he surfel radius gets equal coverage
    float3 WtoS = surfel.Position - posW;
    float distanceSquared = dot(WtoS, WtoS);
	// TODO: possible add double the surfel radius, in order to push new surfels as far as possible from other surfels
    if (distanceSquared < SurfelRadiusSquared * 4)
    {
        float coeff = 1.0f - (distanceSquared / (SurfelRadiusSquared * 4));
        return coeff * NdotSN;
    }

    return 0.0f;
}

float GetPixelProjectedArea(uint2 loc)
{
    uint2 textureDim;
    Data.GBuffer.Depth.GetDimensions(textureDim.x, textureDim.y);
	if(loc.x + 1 > textureDim.x
		|| loc.y + 1 > textureDim.y)
    {
        return 0.0f;
    }

    float depth = Data.GBuffer.Depth[loc].r;
    if (depth == 1.0f)
    {
        return 0.0f;
    }

	// a --- b
	// |     |
	// c --- d
    float3 a = GetWorldPosition(loc);
    float3 b = GetWorldPosition(loc + uint2(1, 0));
    float3 c = GetWorldPosition(loc + uint2(0, 1));
    float3 d = GetWorldPosition(loc + uint2(1, 1));
    float area = 0.5f * (length(cross(b - a, d - a)) + length(cross(d - a, c - a)));
    return area * 5000.0f;
}

#define BLOCK_SIZE_X 16
#define BLOCK_SIZE_Y 16

groupshared uint2 groupScreenPos[BLOCK_SIZE_X * BLOCK_SIZE_Y];
groupshared float groupCoverage[BLOCK_SIZE_X * BLOCK_SIZE_Y];

[numthreads(BLOCK_SIZE_X, BLOCK_SIZE_Y, 1)]
void main(uint3 tid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    float3 posW = GetWorldPosition(tid.xy);
    float3 normal = GetNormal(tid.xy);

    float coverage = 0.0f;
    for (uint i = 0; i < Data.Surfels.Count[0]; ++i)
    {
        coverage += computeCoverage(posW, normal, Data.Surfels.Storage[i]);
    }

    groupCoverage[groupIndex] = coverage;
    groupScreenPos[groupIndex] = tid.xy;

    GroupMemoryBarrierWithGroupSync();

    gCoverage[tid.xy] = float2(coverage, 0.0f);

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

    if (groupIndex == 0 && groupCoverage[0] < COVERAGE_THRESHOLD)
    {
        uint seedState = RandomSeed(globalTime * 1000 + tid.x * 4096 + tid.y);
        float chance = RandomFloat(seedState);
        float pixArea = GetPixelProjectedArea(groupScreenPos[0]);
        if (chance > globalSpawnChance)
        {
			//gCoverage[groupScreenPos[0]] = 1000.0f;
            gCoverage[groupScreenPos[0]] = float2(groupCoverage[0], pixArea);

			// Create new Surfel
            uint dim;
            uint stride;
            Data.Surfels.Storage.GetDimensions(dim, stride);
            if (Data.Surfels.Count[0] >= dim)
                return;

            uint index;
            InterlockedAdd(Data.Surfels.Count[0], 1, index);
            Surfel surfel;
            surfel.Position = GetWorldPosition(groupScreenPos[0]);
            surfel.Normal = GetNormal(groupScreenPos[0]);
            surfel.Color.r = RandomFloat(seedState);
            surfel.Color.g = RandomFloat(seedState);
            surfel.Color.b = RandomFloat(seedState);

            Data.Surfels.Storage[index] = surfel;
        }
    }
}

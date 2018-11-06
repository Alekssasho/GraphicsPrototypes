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

[numthreads(16, 16, 1)]
void main(uint3 tid : SV_DispatchThreadID)
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

    gCoverage[tid.xy] = coverage;
}

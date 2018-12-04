import GICommon;
import Random;

RWTexture2D<float4> gGIMap;

float K(float dist)
{
	if(dist > 1)
        return 0;
    return (2 * dist * dist * dist) - (3 * dist * dist) + 1;
}

float dist(float3 pos, float3 surfelCenter, float3 surfelNormal)
{
    float3 v = (pos - surfelCenter + (2 * (dot(pos - surfelCenter, surfelNormal)) * surfelNormal));
    return sqrt(dot(v, v));
}

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) : SV_TARGET0
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);

    float3 posW = GetWorldPosition(tid.xy);
    float3 normal = GetNormal(tid.xy);

    uint worldIndex = FlattenWorldIndex(GetWorldStructureIndex(posW));
    uint startIndex = Data.Surfels.WorldStructure[worldIndex].StartIndex;
    for (uint i = 0; i < Data.Surfels.WorldStructure[worldIndex].Count; ++i)
    {
        uint surfelIndex = Data.Surfels.Indices[startIndex + i];
#ifndef VISUALIZE
        float3 surfelNormal = Data.Surfels.Storage[surfelIndex].Normal;
        float3 surfelCenter = Data.Surfels.Storage[surfelIndex].Position;
        float3 surfelColor = Data.Surfels.Storage[surfelIndex].Color;
        float weight = K(dist(posW, surfelCenter, surfelNormal) / SurfelRadius)
				* pow(max(0, dot(normal, surfelNormal)), 2);
        color.rgb += weight * surfelColor;
#else
        float3 distanceVec = posW - Data.Surfels.Storage[surfelIndex].Position;
        if ((SurfelRadius * SurfelRadius) >= dot(distanceVec, distanceVec))
        {
            // Check normals direction
            if (dot(normal, Data.Surfels.Storage[surfelIndex].Normal) > 0)
            {
                uint seedState = RandomSeed(surfelIndex);
                color.r = RandomFloat(seedState);
                color.g = RandomFloat(seedState);
                color.b = RandomFloat(seedState);
                color.a = 1.0f;
                break;
            }
        }
#endif
    }

    gGIMap[tid.xy] = color;
}

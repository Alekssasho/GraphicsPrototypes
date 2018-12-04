import GICommon;
import Random;

RWTexture2D<float4> gGIMap;

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) : SV_TARGET0
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float3 posW = GetWorldPosition(tid.xy);
    float3 normal = GetNormal(tid.xy);

    uint worldIndex = FlattenWorldIndex(GetWorldStructureIndex(posW));
    uint startIndex = Data.Surfels.WorldStructure[worldIndex].StartIndex;
    for (uint i = 0; i < Data.Surfels.WorldStructure[worldIndex].Count; ++i)
    {
        uint surfelIndex = Data.Surfels.Indices[startIndex + i];
        float3 distanceVec = posW - Data.Surfels.Storage[surfelIndex].Position;
        if ((SurfelRadius * SurfelRadius) >= dot(distanceVec, distanceVec))
        {
#ifndef VISUALIZE

#else
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
#endif
        }
    }

    gGIMap[tid.xy] = color;
}

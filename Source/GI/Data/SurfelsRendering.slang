import GICommon;
import Random;
#include "HostDeviceSharedMacros.h"

RWTexture2D<float4> gGIMap;
RWTexture2D<float4> gIrradiance;

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) : SV_TARGET0
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);

    float3 posW = GetWorldPosition(tid.xy);
    float3 normal = GetNormal(tid.xy);
    float3 albedo = GetAlbedo(tid.xy);

#ifdef VISUALIZE
    uint worldIndex = FlattenWorldIndex(GetWorldStructureIndex(posW));
    uint startIndex = Data.Surfels.WorldStructure[worldIndex].StartIndex;
    for (uint i = 0; i < Data.Surfels.WorldStructure[worldIndex].Count; ++i)
    {
		uint surfelIndex = Data.Surfels.Indices[startIndex + i];
		float distance = dist(posW, Data.Surfels.Storage[surfelIndex].Position, Data.Surfels.Storage[surfelIndex].Normal);
        if (distance <= SurfelRadius)
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
    }
#else
    float3 irradiance = GetIrradianceAtPoint(posW, normal);
    gIrradiance[tid.xy] = float4(irradiance, 1.0f);

    color.rgb = (albedo / M_PI) * irradiance;

    float4 colorext = float4(0.0f, 0.0f, 0.0f, 1.0f);
    uint worldIndex = FlattenWorldIndex(GetWorldStructureIndex(posW));
    uint startIndex = Data.Surfels.WorldStructure[worldIndex].StartIndex;
    for (uint i = 0; i < Data.Surfels.WorldStructure[worldIndex].Count; ++i)
    {
        uint surfelIndex = Data.Surfels.Indices[startIndex + i];
        float distance = dist(posW, Data.Surfels.Storage[surfelIndex].Position, Data.Surfels.Storage[surfelIndex].Normal);
        if (distance <= SurfelRadius)
        {
            // Check normals direction
            if (dot(normal, Data.Surfels.Storage[surfelIndex].Normal) > 0)
            {
                colorext = Data.Surfels.Storage[surfelIndex].DebugData;
                //colorext = float4(albedo, 0.0f);
                break;
            }
        }
    }
    Data.DebugTexture[tid.xy] = colorext;
#endif

    gGIMap[tid.xy] = color;
}

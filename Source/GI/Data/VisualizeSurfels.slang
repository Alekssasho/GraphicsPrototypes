import GICommon;

float4 main(float2 texC : TEXCOORD, float4 pos : SV_POSITION) : SV_TARGET0
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float3 posW = GetWorldPosition(pos.xy);
    float3 normal = GetNormal(pos.xy);

    uint worldIndex = FlattenWorldIndex(GetWorldStructureIndex(posW));
    uint startIndex = Data.Surfels.WorldStructure[worldIndex].StartIndex;
    for (uint i = 0; i < Data.Surfels.WorldStructure[worldIndex].Count; ++i)
    {
        uint surfelIndex = Data.Surfels.Indices[startIndex + i];
        float3 distanceVec = posW - Data.Surfels.Storage[surfelIndex].Position;
        if ((SurfelRadius * SurfelRadius) >= dot(distanceVec, distanceVec))
        {
            // Check normals direction
            if (dot(normal, Data.Surfels.Storage[surfelIndex].Normal) > 0)
            {
                color = float4(Data.Surfels.Storage[surfelIndex].Color, 1.0f);
                break;
            }
        }
    }

    return color;
}

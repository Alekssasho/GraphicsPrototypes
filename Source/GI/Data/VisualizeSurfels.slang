__import GICommon;

float4 main(float2 texC : TEXCOORD, float4 pos : SV_POSITION) : SV_TARGET0
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

    uint numSurfels = 0;
    uint stride = 0;
    Data.surfels.SurfelStorage.GetDimensions(numSurfels, stride);

    float3 posW = GetWorldPosition(pos.xy, Data);
    float3 normal = GetNormal(pos.xy, Data);

    for (uint i = 0; i < numSurfels; ++i)
    {
        float3 distanceVec = posW - Data.surfels.SurfelStorage[i].Position;
        if ((SurfelRadius * SurfelRadius) >= dot(distanceVec, distanceVec))
        {
            // Check normals direction
            if (dot(normal, Data.surfels.SurfelStorage[i].Normal) > 0)
            {
                color = float4(Data.surfels.SurfelStorage[i].Color, 1.0f);
                break;
            }
        }
    }

    return color;
}

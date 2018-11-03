#include "HostDeviceSurfelsData.h"

__import ShaderCommon;

Texture2D gBuffNormal;
Texture2D gBuffDepth;

cbuffer GlobalData
{
	CameraData globalCamera;
};

StructuredBuffer<Surfel> gSurfels;

// TODO: Refactor
float3 reconstructWorldPosition(int3 loc, float2 texC)
{
	// Reconstruct World Position
    float depth = gBuffDepth.Load(loc).r;
    float2 ndcCoords = texC * 2.0f - 1.0f;
    ndcCoords.y = -ndcCoords.y;
    float4 projectedPos = float4(ndcCoords, depth, 1.0f);
    float4 transformedPos = mul(projectedPos, globalCamera.invViewProj);
    return transformedPos.xyz / transformedPos.w;
}

float4 main(float2 texC : TEXCOORD, float4 pos : SV_POSITION) : SV_TARGET0
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

    uint numSurfels = 0;
    uint stride = 0;
    gSurfels.GetDimensions(numSurfels, stride);

    float3 posW = reconstructWorldPosition(int3(pos.xy, 0), texC);

    for (uint i = 0; i < numSurfels; ++i)
    {
        float3 distanceVec = posW - gSurfels[i].Position;
        if ((SURFELS_RADIUS * SURFELS_RADIUS) >= dot(distanceVec, distanceVec))
        {
            // Check normals direction
            if (dot(gBuffNormal.Load(int3(pos.xy, 0)).xyz, gSurfels[i].Normal) > 0)
            {
                color = float4(gSurfels[i].Color, 1.0f);
                break;
            }
        }
    }

    return color;
}

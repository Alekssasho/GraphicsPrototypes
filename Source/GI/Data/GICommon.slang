#include "HostDeviceSurfelsData.h"

__import ShaderCommon;

struct GBuffer
{
	Texture2D Normal;
	Texture2D Depth;
};

struct SurfelsData
{
	StructuredBuffer<Surfel> SurfelStorage;
};

struct CommonData
{
    GBuffer gBuff;
    SurfelsData surfels;
    CameraData camera;
};

ParameterBlock<CommonData> Data;

float3 GetWorldPosition(uint2 loc, CommonData data)
{
    uint2 dimensions;
    data.gBuff.Depth.GetDimensions(dimensions.x, dimensions.y);
    float2 texC = float2(loc) / float2(dimensions);

    float depth = data.gBuff.Depth[loc].r;
    float2 ndcCoords = texC * 2.0f - 1.0f;
    ndcCoords.y = -ndcCoords.y;
    float4 projectedPos = float4(ndcCoords, depth, 1.0f);
    float4 transformedPos = mul(projectedPos, data.camera.invViewProj);
    return transformedPos.xyz / transformedPos.w;
}

float3 GetNormal(uint2 loc, CommonData data)
{
    float3 normal = data.gBuff.Normal[loc].xyz;
    return normalize(normal * 2.0f - 1.0f);
}

#include "HostDeviceSurfelsData.h"

import ShaderCommon;

struct GBufferTextures
{
	Texture2D Normal;
	Texture2D Depth;
};

struct SurfelsData
{
	RWStructuredBuffer<Surfel> Storage;
	RWStructuredBuffer<uint> Count;
    StructuredBuffer<WorldStructureChunk> WorldStructure;
    StructuredBuffer<uint> Indices;
};

struct CommonData
{
    GBufferTextures GBuffer;
    SurfelsData Surfels;
    CameraData Camera;
};

ParameterBlock<CommonData> Data;

float3 GetWorldPosition(uint2 loc)
{
    uint2 dimensions;
    Data.GBuffer.Depth.GetDimensions(dimensions.x, dimensions.y);
    float2 texC = float2(loc) / float2(dimensions);

    float depth = Data.GBuffer.Depth[loc].r;
    float2 ndcCoords = texC * 2.0f - 1.0f;
    ndcCoords.y = -ndcCoords.y;
    float4 projectedPos = float4(ndcCoords, depth, 1.0f);
    float4 transformedPos = mul(projectedPos, Data.Camera.invViewProj);
    return transformedPos.xyz / transformedPos.w;
}

float3 GetNormal(uint2 loc)
{
    float3 normal = Data.GBuffer.Normal[loc].xyz;
    return normalize(normal * 2.0f - 1.0f);
}

uint FlattenWorldIndex(uint3 index)
{
    return index.x
         + index.y * WORLD_STRUCTURE_DIMENSION
         + index.z * (WORLD_STRUCTURE_DIMENSION * WORLD_STRUCTURE_DIMENSION);
}

uint GetWorldStructureIndex(float3 pos)
{
    // Translate with world structure center offset
    pos += float3(WORLD_DIMENSION / 2.0f);
    pos = clamp(pos, 0.0f, WORLD_DIMENSION);

    uint3 worldStructure3DIndex = floor((pos / WORLD_DIMENSION) * WORLD_STRUCTURE_DIMENSION);
    return FlattenWorldIndex(worldStructure3DIndex);
}

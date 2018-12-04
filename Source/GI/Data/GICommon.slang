#include "HostDeviceSurfelsData.h"

import ShaderCommon;

struct GBufferTextures
{
	Texture2D Normal;
	Texture2D Depth;
    Texture2D Albedo;
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

float3 GetAlbedo(uint2 loc)
{
    return Data.GBuffer.Albedo[loc].xyz;
}

uint FlattenWorldIndex(uint3 index)
{
    return index.x
         + index.y * WORLD_STRUCTURE_DIMENSION
         + index.z * (WORLD_STRUCTURE_DIMENSION * WORLD_STRUCTURE_DIMENSION);
}

uint3 ThreeDWorldIndex(uint index)
{
    uint3 res;
    res.z = index / (WORLD_STRUCTURE_DIMENSION * WORLD_STRUCTURE_DIMENSION);
    index -= res.z * WORLD_STRUCTURE_DIMENSION * WORLD_STRUCTURE_DIMENSION;
    res.y = index / WORLD_STRUCTURE_DIMENSION;
    res.x = index % WORLD_STRUCTURE_DIMENSION;
    return res;
}

uint3 GetWorldStructureIndex(float3 pos)
{
    // Translate with world structure center offset
    pos += float3(WORLD_DIMENSION / 2.0f);
    pos = clamp(pos, 0.0f, WORLD_DIMENSION);

    uint3 worldStructure3DIndex = floor((pos / WORLD_DIMENSION) * WORLD_STRUCTURE_DIMENSION);
    return worldStructure3DIndex;
}

float3 GetChunkCenter(uint3 index)
{
    float3 result = index * WORLD_STRUCTURE_CHUNK_SIZE + float3(WORLD_STRUCTURE_CHUNK_SIZE / 2.0f);
	// Translate with world structure origin
    return result - float3(WORLD_DIMENSION / 2.0f);
}

import ShaderCommon;
import Raytracing;
import GICommon;
import Random;
import Lights;
import Shading;
import Helpers;
#include "HostDeviceSharedMacros.h"

cbuffer GlobalState
{
    float globalTime;
    float globalSpawnChance;
}

struct SurfelRayPayload
{
	float3 Color;
    uint seed;
};

struct LightRayPayload
{
	bool Hit;
};

RaytracingAccelerationStructure rtScene;

[shader("miss")]
void SurfelRayMiss(inout SurfelRayPayload payload)
{
	payload.Color = float3(0.0f, 0.0f, 0.0f);
}

[shader("miss")]
void LightRayMiss(inout LightRayPayload payload)
{
	payload.Hit = false;
}

[shader("anyhit")]
void LightRayAnyHit(inout LightRayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	payload.Hit = true;
}

[shader("closesthit")]
void SurfelClosestHit(inout SurfelRayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    VertexOut vOut = getVertexAttributes(PrimitiveIndex(), attribs);
    ShadingData sd = prepareShadingData(vOut, gMaterial, WorldRayOrigin(), 0);

    float3 irradiance = GetIrradianceAtPoint(sd.posW, sd.N);

    payload.Color = irradiance * (sd.diffuse / M_PI);


	// Explicit light sample
    float choice = rand_next(payload.seed);
    uint chosenLight = round((gLightsCount - 1) * choice);

    LightSample ls = evalLight(gLights[chosenLight], sd);
    ShadingResult sr = evalMaterial(sd, gLights[chosenLight], 1.0f);

    RayDesc ray;
    ray.Origin = sd.posW;
    ray.Direction = ls.L;
    ray.TMax = length(ls.posW - ray.Origin);

    LightRayPayload lightRayPayload;
    TraceRay(rtScene,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
		0xff,
		1,
		hitProgramCount,
		1,
		ray,
		lightRayPayload);

    float3 radiance = (!lightRayPayload.Hit * sr.diffuse.rgb) * float(gLightsCount);
    payload.Color += radiance;
}

[shader("raygeneration")]
void SurfelRayGeneration()
{
	uint index = DispatchRaysIndex().x;

	uint surfelIndex = index % Data.Surfels.Count[0];

    uint randSeed = rand_init(index, globalTime, 16);
    float2 randVal = float2(rand_next(randSeed), rand_next(randSeed));

	RayDesc ray;
	ray.Origin = Data.Surfels.Storage[surfelIndex].Position;
    ray.Direction = getCosHemisphereSample(randVal,
		Data.Surfels.Storage[surfelIndex].Normal,
		getPerpendicularStark(Data.Surfels.Storage[surfelIndex].Normal));
	// Using lower values than 0.01 is causing artefacts due to inprecision in World Position reconstruction method used
	ray.TMin = 0.01;
	ray.TMax = 100000;

    float3 irradiance = 0.0f;

	SurfelRayPayload surfelRayPayload;
    surfelRayPayload.seed = randSeed;
	TraceRay(rtScene,
		0,
		0xff,
		0,
		hitProgramCount,
		0,
		ray,
		surfelRayPayload);

    irradiance += surfelRayPayload.Color;

	// Moving exponential average
	// TODO: Return to exponential average
    Data.Surfels.Storage[surfelIndex].Color = lerp(
		Data.Surfels.Storage[surfelIndex].Color,
		irradiance,
		1.0f / (Data.Surfels.Storage[surfelIndex].Age + 1));
    Data.Surfels.Storage[surfelIndex].Age++;
}
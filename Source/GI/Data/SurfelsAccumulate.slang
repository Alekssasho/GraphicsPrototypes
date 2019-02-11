import ShaderCommon;
import Raytracing;
import GICommon;
import Random;

struct SurfelRayPayload
{
	float3 Color;
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
	payload.Color = float3(0.1f, 0.1f, 0.1f);
}

[shader("raygeneration")]
void SurfelRayGeneration()
{
	uint index = DispatchRaysIndex().x;

	uint surfelIndex = index % Data.Surfels.Count[0];

	RayDesc ray;
	ray.Origin = Data.Surfels.Storage[surfelIndex].Position;
	ray.Direction = Data.Surfels.Storage[surfelIndex].Normal;
	// Using lower values than 0.01 is causing artefacts due to inprecision in World Position reconstruction method used
	ray.TMin = 0.01;
	ray.TMax = 100000;

	SurfelRayPayload surfelRayPayload;
	TraceRay(rtScene,
		0,
		0xff,
		0,
		hitProgramCount,
		0,
		ray,
		surfelRayPayload);

	// TODO: get lights
	uint seedState = RandomSeed(surfelIndex);
	float choice = RandomFloat(seedState);
	uint chosenLight = round((gLightsCount - 1) * choice);

	float3 surfelToLight = gLights[chosenLight].posW - ray.Origin;
	ray.Direction = normalize(surfelToLight);
	ray.TMax = length(surfelToLight);

	LightRayPayload lightRayPayload;
	TraceRay(rtScene,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
		0xff,
		1,
		hitProgramCount,
		1,
		ray,
		lightRayPayload);

	Data.Surfels.Storage[surfelIndex].Color += surfelRayPayload.Color;
}
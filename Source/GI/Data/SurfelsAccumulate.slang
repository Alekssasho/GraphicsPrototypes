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

    uint seed = payload.seed;
    float choice = rand_next(seed);
    uint chosenLight = round((gLightsCount - 1) * choice);

    LightSample ls = evalLight(gLights[chosenLight], sd);
    ShadingResult sr = evalMaterial(sd, gLights[chosenLight], 1.0f);

    RayDesc lRay;
    lRay.Origin = sd.posW;
    lRay.Direction = ls.L;
    lRay.TMax = length(ls.posW - lRay.Origin);

    LightRayPayload lightRayPayload;
    TraceRay(rtScene,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
		0xff,
		1,
		hitProgramCount,
		1,
		lRay,
		lightRayPayload);

    float3 radiance = (!lightRayPayload.Hit * ls.diffuse.rgb * ls.NdotL) * float(gLightsCount);
    irradiance += radiance;

	payload.Color = irradiance * (sd.diffuse / M_PI);
}

  
float3 MultiscaleMeanEstimator(float3 y,
  inout MultiscaleMeanEstimatorData data,
  float shortWindowBlend = 0.08f)
{
    float3 mean = data.mean;
    float3 shortMean = data.shortMean;
    float vbbr = data.vbbr;
    float3 variance = data.variance;
    float inconsistency = data.inconsistency;
  
  // Suppress fireflies.
  {
        float3 dev = sqrt(max(1e-5, variance));
        float3 highThreshold = 0.1 + shortMean + dev * 8;
        float3 overflow = max(0, y - highThreshold);
        y -= overflow;
    }
  
    float3 delta = y - shortMean;
    shortMean = lerp(shortMean, y, shortWindowBlend);
    float3 delta2 = y - shortMean;
  
  // This should be a longer window than shortWindowBlend to avoid bias
  // from the variance getting smaller when the short-term mean does.
    float varianceBlend = shortWindowBlend * 0.5;
    variance = lerp(variance, delta * delta2, varianceBlend);
    float3 dev = sqrt(max(1e-5, variance));
  
    float3 shortDiff = mean - shortMean;
  
    float relativeDiff = dot(float3(0.299, 0.587, 0.114),
        abs(shortDiff) / max(1e-5, dev));
    inconsistency = lerp(inconsistency, relativeDiff, 0.08);
  
    float varianceBasedBlendReduction =
        clamp(dot(float3(0.299, 0.587, 0.114),
        0.5 * shortMean / max(1e-5, dev)), 1.0 / 32, 1);
  
    float3 catchUpBlend = clamp(smoothstep(0, 1,
        relativeDiff * max(0.02, inconsistency - 0.2)), 1.0 / 256, 1);
    catchUpBlend *= vbbr;
  
    vbbr = lerp(vbbr, varianceBasedBlendReduction, 0.1);
    mean = lerp(mean, y, saturate(catchUpBlend));
  
  // Output
    data.mean = mean;
    data.shortMean = shortMean;
    data.vbbr = vbbr;
    data.variance = variance;
    data.inconsistency = inconsistency;
  
    return mean;
}

[shader("raygeneration")]
void SurfelRayGeneration()
{
	uint index = DispatchRaysIndex().x;

	uint surfelIndex = index % Data.Surfels.Count[0];

	uint randSeed = rand_init(index, globalTime * 100, 16);
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

    MultiscaleMeanEstimator(irradiance, Data.Surfels.Storage[surfelIndex].Irradiance);
}
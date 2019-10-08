#ifndef HOST_DEVICE_SURFELS_DATA_H
#define HOST_DEVICE_SURFELS_DATA_H

#include "Data/HostDeviceData.h"

struct MultiscaleMeanEstimatorData
{
	float3 mean;
	float3 shortMean;
	float vbbr;
	float3 variance;
	float inconsistency;
};

struct Surfel
{
	float3 Position  DEFAULTS(float3(0.0f, 0.0f, 0.0f));
	float3 Normal    DEFAULTS(float3(0.0f, 0.0f, 1.0f));
	MultiscaleMeanEstimatorData Irradiance;
};

struct WorldStructureChunk
{
	uint StartIndex;
	uint Count;
};

static const uint WORLD_STRUCTURE_DIMENSION = 32;
static const float WORLD_DIMENSION = 20.0f;
static const float WORLD_STRUCTURE_CHUNK_SIZE = (WORLD_DIMENSION / WORLD_STRUCTURE_DIMENSION);

static const uint WORLD_STRUCTURE_TOTAL_SIZE = WORLD_STRUCTURE_DIMENSION * WORLD_STRUCTURE_DIMENSION * WORLD_STRUCTURE_DIMENSION;

static const float SurfelRadius = WORLD_STRUCTURE_CHUNK_SIZE / 6.0f;
static const float SurfelRadiusSquared = SurfelRadius * SurfelRadius;
#endif

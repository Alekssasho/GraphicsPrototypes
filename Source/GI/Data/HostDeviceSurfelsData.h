#ifndef HOST_DEVICE_SURFELS_DATA_H
#define HOST_DEVICE_SURFELS_DATA_H

#include "Data/HostDeviceData.h"

struct Surfel
{
	float3 Position DEFAULTS(float3(0.0f, 0.0f, 0.0f));
	float3 Normal   DEFAULTS(float3(0.0f, 0.0f, 1.0f));
	float3 Color    DEFAULTS(float3(0.0f, 0.0f, 0.0f));
};

struct WorldStructureChunk
{
	uint StartIndex;
	uint Count;
};

static const float SurfelRadius = 0.125f;
static const float SurfelRadiusSquared = SurfelRadius * SurfelRadius;

static const uint WORLD_STRUCTURE_DIMENSION = 32;
static const float WORLD_DIMENSION = 6.0f;
static const float WORLD_STRUCTURE_CHUNK_SIZE = (WORLD_DIMENSION / WORLD_STRUCTURE_DIMENSION);

static const uint WORLD_STRUCTURE_TOTAL_SIZE = WORLD_STRUCTURE_DIMENSION * WORLD_STRUCTURE_DIMENSION * WORLD_STRUCTURE_DIMENSION;

#endif

#ifndef HOST_DEVICE_SURFELS_DATA_H
#define HOST_DEVICE_SURFELS_DATA_H

#include <Data/HostDeviceSharedCode.h>

struct Surfel
{
	float3 Position DEFAULTS(float3(0.0f, 0.0f, 0.0f));
	float3 Normal   DEFAULTS(float3(0.0f, 0.0f, 1.0f));
	float3 Color    DEFAULTS(float3(0.0f, 0.0f, 0.0f));
};

#endif

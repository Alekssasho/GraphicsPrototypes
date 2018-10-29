/***************************************************************************
# Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
__import ShaderCommon;
__import Shading;

cbuffer Globals
{
	uint DebugMode;
};

cbuffer PerFrame
{
	CameraData Camera;
	LightData Lights[_LIGHT_COUNT];
};

struct GBufferTextures
{
	Texture2D Texture0;
	Texture2D Texture1;
	Texture2D Texture2;
	Texture2D Depth;
};

ParameterBlock<GBufferTextures> GB;

Texture2D gVisibilityBuffer;

float3 reconstructWorldPosition(int3 loc, float2 texC)
{
	// Reconstruct World Position
	float depth = GB.Depth.Load(loc).r;
	float2 ndcCoords = texC * 2.0f - 1.0f;
	ndcCoords.y = -ndcCoords.y;
	float4 projectedPos = float4(ndcCoords, depth, 1.0f);
	float4 transformedPos = mul(projectedPos, Camera.invViewProj);
	return transformedPos.xyz / transformedPos.w;
}

#ifdef DEBUG_MODE
float4 debugMode(int3 loc)
{
	switch(DebugMode)
	{
	case 1:
		return GB.Texture0.Load(loc);
	case 2:
		return GB.Texture1.Load(loc);
	case 3:
		return float4(GB.Texture2.Load(loc).xyz, 1.0f);
	case 4:
		return float4(GB.Texture2.Load(loc).aaa, 1.0f);
	case 5:
		float depth = GB.Depth.Load(loc).r;
		float f = Camera.farZ;
		float n = Camera.nearZ;
		float q = f / (n - f);
		float z = 1 / ((depth + q) / (q * n));
		z = (z - n) / (f - n);
		return float4(z, z, z, 1.0f);
	default:
		return float4(0.0f);
	}
}
#endif

float4 applyLighting(int3 loc, float2 texC)
{
	ShadingData sd = initShadingData();

	float4 texture2Data = GB.Texture2.Load(loc);

	sd.posW = reconstructWorldPosition(loc, texC);
	sd.N = normalize(texture2Data.xyz * 2.0f - 1.0f);
	sd.V = normalize(Camera.posW - sd.posW);
	sd.NdotV = dot(sd.N, sd.V);
	sd.diffuse = GB.Texture0.Load(loc).rgb;
	sd.linearRoughness = texture2Data.w;
	sd.specular = GB.Texture1.Load(loc).rgb;
	sd.roughness = sd.linearRoughness * sd.linearRoughness;

	float4 finalColor = float4(0, 0, 0, 1);

	[unroll]
	for (uint l = 0; l < _LIGHT_COUNT; l++)
	{
		float shadowFactor = 1;
#ifdef _ENABLE_SHADOWS
		if (l == 0)
		{
			shadowFactor = gVisibilityBuffer.Load(loc).r;
		}
#endif
		finalColor.rgb += evalMaterial(sd, Lights[l], shadowFactor).color.rgb;
	}

	return finalColor;
}

float4 main(float2 texC : TEXCOORD, float4 pos : SV_POSITION) : SV_TARGET0
{
	float4 color;
	int3 position = int3(pos.xy, 0);
#ifdef DEBUG_MODE
	color = debugMode(position);
#else
	color = applyLighting(position, texC);
#endif
	return color;
}

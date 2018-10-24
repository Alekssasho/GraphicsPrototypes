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
__import Shading;


Texture2D gGBufferTexture0;
Texture2D gGBufferTexture1;

layout(binding = 0) cbuffer Globals : register(b0)
{
	uint gDebugMode;
}

layout(binding = 1) cbuffer PerFrame : register(b1)
{
    CameraData gCamera;
    LightData gLights[_LIGHT_COUNT];
}

#ifdef DEBUG_MODE
float4 debugMode(int3 loc)
{
	switch(gDebugMode)
	{
	case 1:
		return gGBufferTexture0.Load(loc);
		break;
	case 2:
		return gGBufferTexture1.Load(loc);
		break;
	default:
		return float4(0.0f);
	}
}
#endif

float4 applyLighting(int3 loc)
{
	return gGBufferTexture0.Load(loc);
}

float4 main(float2 texC : TEXCOORD, float4 pos : SV_POSITION) : SV_TARGET0
{
	float4 color;
	int3 position = int3(pos.xy, 0);
#ifdef DEBUG_MODE
	color = debugMode(position);
#else
	color = applyLighting(position);
#endif
	return color;
}

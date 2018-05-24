__import DefaultVS;
__import Shading;

#define LightDirectional 1

SamplerState gShadowMapSampler;
Texture2D gShadowMap;

cbuffer PerFrameCB : register(b0)
{
	float3 gAmbient;
	float4x4 gLightViewMatrix;
};

float4 PSmain(VertexOut vOut) : SV_TARGET0
{
	ShadingData sd = prepareShadingData(vOut, gMaterial, gCamera.posW);
	float4 finalColor;
	finalColor.a = 1;

	for (uint i = 0; i < gLightsCount; ++i)
	{
		float shadowFactor = 1.0f;
		if (gLights[i].type == LightDirectional)
		{
			float4 posLightSpace = mul(float4(vOut.posW.xyz, 1.0), gLightViewMatrix);
			posLightSpace.xyz /= posLightSpace.w; // Perspective divide

			float2 shadowMapUv = posLightSpace.xy * 0.5 + 0.5;
			shadowMapUv.y = 1.0 - shadowMapUv.y;
			float shadowMapDepth = gShadowMap.Sample(gShadowMapSampler, shadowMapUv).x;
			shadowFactor = shadowMapDepth >= posLightSpace.z ? 1.0f : 0.0f;
		}
		finalColor.rgb += evalMaterial(sd, gLights[i], shadowFactor).color.rgb;
	}

	finalColor.rgb += sd.diffuse.rgb * gAmbient;
	return finalColor;
}
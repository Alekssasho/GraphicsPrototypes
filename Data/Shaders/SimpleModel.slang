__import DefaultVS;
__import Shading;

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
        float4 posLightSpace = mul(float4(vOut.posW, 1.0), gLightViewMatrix);
        posLightSpace.xyz /= posLightSpace.w; // Perspective divide

        float shadowMapDepth = gShadowMap.Sample(gShadowMapSampler, posLightSpace.xy * 0.5 + 0.5).x;
        shadowFactor = shadowMapDepth >= posLightSpace.z ? 1.0f : 0.0f;
        finalColor.rgb += evalMaterial(sd, gLights[i], shadowFactor).color.rgb;
    }

    finalColor.rgb += sd.diffuse.rgb * gAmbient;
    return finalColor;
}
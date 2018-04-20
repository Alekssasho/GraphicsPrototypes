__import DefaultVS;
__import Shading;

cbuffer PerFrameCB : register(b0)
{
    float3 gAmbient;
};

float4 PSmain(VertexOut vOut) : SV_TARGET0
{
    ShadingData sd = prepareShadingData(vOut, gMaterial, gCamera.posW);
    float4 finalColor;
    finalColor.a = 1;

    for (uint i = 0; i < gLightsCount; ++i)
    {
        finalColor.rgb += evalMaterial(sd, gLights[i], 1).color.rgb;
    }

    finalColor.rgb += sd.diffuse.rgb * gAmbient;
    return finalColor;
}
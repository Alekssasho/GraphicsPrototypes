__import DefaultVS;
__import Shading;

cbuffer PerFrameCB : register(b0)
{
    LightData gDirLight;
    float3 gAmbient;
};


float4 PSmain(VertexOut vOut) : SV_TARGET0
{
    ShadingData sd = prepareShadingData(vOut, gMaterial, gCamera.posW);
    float4 finalColor;
    finalColor.a = 1;
    finalColor.rgb = evalMaterial(sd, gDirLight, 1).color.rgb;
    return finalColor;
}
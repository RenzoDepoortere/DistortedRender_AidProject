
cbuffer CB_Object : register(b0)
{
    float4 g_objectColor;
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // System value
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    return float4(1, 0, 0, 1);
}
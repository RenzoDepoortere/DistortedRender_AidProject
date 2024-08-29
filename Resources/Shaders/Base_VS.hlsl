
cbuffer CB_World : register(b0)    // Register for GPU access (b. for constant buffers)
{
    matrix g_WorldViewProjection;   // World to projection space
    matrix g_World;                 // World space
};

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;  // System value
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    
    output.position = mul(float4(input.position, 1.0), g_WorldViewProjection);
    output.normal = normalize(mul(input.normal, (float3x3) g_World));
    output.uv = input.uv;
    
    return output;
}
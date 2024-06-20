struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PSOutput  
{  
    float4 Color : SV_Target;  
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float2 uv : TEXCOORD)
{
    PSInput result;

    result.position = position;
    result.uv = uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // float y = g_texture.Sample(g_sampler, input.uv).r;

    // float2 uv_chroma = input.uv * float2(1.0, 0.5);
    // float2 uv_color = (uv_chroma - 0.5) * float2(1, -1) + 0.5;

    // float u = g_texture.Sample(g_sampler, uv_color).r;
    // float v = g_texture.Sample(g_sampler, uv_color + float2(0, 0.5)).r;

    // float4 color;
    // color.r = y + 1.370705 * v;
    // color.g = y - 0.698001 * v - 0.337633 * u;
    // color.b = y + 1.732446 * u;
    // color.a = 1.0;
    return g_texture.Sample(g_sampler, input.uv);
}
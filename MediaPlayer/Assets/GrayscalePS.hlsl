Texture2D tex : register(t0);
SamplerState sampl : register(s0);

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : UV;
};

float4 main(PS_IN input) : SV_TARGET
{
    float4 color = tex.Sample(sampl, input.uv);
    float luminance = dot(color.rgb, float3(0.299, 0.587, 0.114));
    return float4(luminance, luminance, luminance, color.a);
}
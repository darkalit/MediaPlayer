Texture2D tex : register(t0);
SamplerState sampl : register(s0);

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : UV;
};

float4 main(PS_IN input) : SV_TARGET
{
    return tex.Sample(sampl, input.uv);
}
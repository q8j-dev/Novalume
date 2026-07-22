struct FragmentInput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

SamplerState s_colorSampler : register(s0);
Texture2D<float4> s_colorTexture : register(t0);

float4 main(FragmentInput input) : SV_Target
{
    return s_colorTexture.Sample(s_colorSampler, input.texcoord) * input.color;
}

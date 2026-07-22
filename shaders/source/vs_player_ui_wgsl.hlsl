struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

uniform float4x4 u_modelViewProj;

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.position = mul(u_modelViewProj, float4(input.position, 1.0));
    output.color = input.color;
    output.texcoord = input.texcoord;
    return output;
}

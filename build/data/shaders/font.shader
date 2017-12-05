#include "data/shaders/common.shader-include" // @Cleanup Make the compile working dir data/shaders somehow

Texture2D m_texture : register(t0);
SamplerState m_sampler_state;

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 tex_coord : TEXCOORD;
    float4 color : COLOR;
};

VS_FUNC VS_OUTPUT VS(float4 input_pos : POSITION, float2 input_tex_coord : TEXCOORD, float4 input_color : COLOR)
{
    VS_OUTPUT output;

    output.pos = convert_coords(input_pos);
    output.tex_coord = input_tex_coord;
    output.color = input_color;

    return output;
}

PS_FUNC float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float alpha =  m_texture.Sample( m_sampler_state, input.tex_coord).a;

    input.color.a = alpha;

    return input.color;
}

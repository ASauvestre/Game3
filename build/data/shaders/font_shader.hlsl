#include "data/shaders/common.hlsl" // @Cleanup Make the compile working dir data/shaders somehow

Texture2D m_texture : register(t0);
SamplerState m_sampler_state;

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 tex_coord : TEXCOORD;
};

VS_FUNC VS_OUTPUT VS(float4 input_pos : POSITION, float2 input_tex_coord : TEXCOORD)
{
    VS_OUTPUT output;

    output.pos = convert_coords(input_pos);
    output.tex_coord = input_tex_coord;

    return output;
}

PS_FUNC float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float4 textureColor =  m_texture.Sample( m_sampler_state, input.tex_coord);

    textureColor.r = 1.0;
    textureColor.g = 1.0;
    textureColor.b = 1.0;

    return textureColor;
}

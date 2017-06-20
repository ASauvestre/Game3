Texture2D m_texture : register(t0);
SamplerState m_sampler_state;

struct VS_OUTPUT 
{   
    float4 pos : SV_POSITION;
    float2 tex_coord : TEXCOORD;
};

VS_OUTPUT VS(float4 input_pos : POSITION, float2 input_tex_coord : TEXCOORD) 
{
    VS_OUTPUT output;

    output.pos = input_pos;
    output.tex_coord = input_tex_coord;

    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float4 textureColor =  m_texture.Sample( m_sampler_state, input.tex_coord);

    // @Temporary
    // Discard transparent pixels
    if(textureColor.a < 1.0) {  
        discard;
    }

    return textureColor;
} 
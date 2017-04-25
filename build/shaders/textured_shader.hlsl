Texture2DArray m_texture_array;
SamplerState m_sampler_state;

cbuffer vs_buffer : register(b0) {
	int4 texture_index_map[512]; // Max number of textures
};

struct VS_OUTPUT 
{
	float4 pos : SV_POSITION;
	float2 tex_coord : TEXCOORD;
	float texid : TEXID;
};

VS_OUTPUT VS(float4 input_pos : POSITION, float2 input_tex_coord : TEXCOORD, float input_texid : TEXID) 
{
	VS_OUTPUT output;

	output.pos = input_pos;
	output.tex_coord = input_tex_coord;
	output.texid = texture_index_map[input_texid/4][input_texid%4];

	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	float3 coords;
	coords.x = input.tex_coord.x;
	coords.y = input.tex_coord.y;
	coords.z = input.texid;
	float4 textureColor =  m_texture_array.Sample( m_sampler_state, coords);

	// Discard transparent pixels
	if(textureColor.a < 1.0) {	
		discard;
	}

	return textureColor;
}
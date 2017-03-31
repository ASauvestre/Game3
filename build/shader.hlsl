Texture2DArray m_texture_array;
SamplerState m_sampler_state;

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
	output.texid = input_texid;

	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	float3 coords;
	coords.x = input.tex_coord.x;
	coords.y = input.tex_coord.y;
	coords.z = input.texid;
	return m_texture_array.Sample( m_sampler_state, coords);
}
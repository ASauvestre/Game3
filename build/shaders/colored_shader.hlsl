Texture2DArray m_texture_array;
SamplerState m_sampler_state;

cbuffer vs_buffer : register(b0) {
	int4 texture_index_map[512]; // Max number of textures
};

struct VS_OUTPUT 
{
	float4 pos : SV_POSITION;
	float4 color: COLOR;
};

VS_OUTPUT VS(float4 input_pos : POSITION, float4 input_color : COLOR) 
{
	VS_OUTPUT output;

	output.pos = input_pos;
	output.color = input_color;

	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	return input.color;
}
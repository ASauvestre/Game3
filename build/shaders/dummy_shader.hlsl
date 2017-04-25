struct VS_OUTPUT 
{
	float4 pos : SV_POSITION;
	float2 tex_coord : TEXCOORD;
	float texid : TEXID;
};

VS_OUTPUT dummy_vs(float4 input_pos : POSITION, float2 input_tex_coord : TEXCOORD, float input_texid : TEXID) 
{
	VS_OUTPUT output;

	output.pos = input_pos;
	output.tex_coord = input_tex_coord;
	output.texid = 0;

	return output;
}
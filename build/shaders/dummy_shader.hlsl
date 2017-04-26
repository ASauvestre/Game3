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
	output.texid = 0;

	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	return float4(0, 1, 0 , 1);
}
float4 VS(float4 input_pos : POSITION) : SV_POSITION
{
	return input_pos;
}

float4 PS() : SV_TARGET
{
	return float4(0, 1, 0, 1);
}
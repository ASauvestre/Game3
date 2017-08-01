#include "data/shaders/common.hlsl"

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color: COLOR;
};

VS_OUTPUT VS(float4 input_pos : POSITION, float4 input_color : COLOR)
{
    VS_OUTPUT output;

    output.pos = convert_coords(input_pos);
    output.color = input_color;

    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    return input.color;
}

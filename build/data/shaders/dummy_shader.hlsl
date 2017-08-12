#include "data/shaders/common.hlsl"

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
};

VS_FUNC VS_OUTPUT VS(float4 input_pos : POSITION)
{
    VS_OUTPUT output;

    output.pos = convert_coords(input_pos);

    return output;
}

PS_FUNC float4 PS() : SV_TARGET
{
    return float4(0, 1, 0, 1);
}

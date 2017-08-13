// For parsing the input layout
#define VS_FUNC
#define PS_FUNC // Useless

float4 convert_coords(float4 pos) {
    pos.x *= 2.0f;
    pos.y *= 2.0f;

    pos.x -= 1.0f;
    pos.y -= 1.0f;

    // Flip the z axis
    pos.z *= -1.0f;
    pos.z += 1.0f;

    return pos;
}

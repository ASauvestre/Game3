float4 convert_coords(float4 pos) {
    pos.x *= 2.0f;
    pos.y *= 2.0f;

    pos.x -= 1.0f;
    pos.y -= 1.0f;

    return pos;
}

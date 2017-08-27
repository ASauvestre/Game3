#pragma once

struct Quad;
struct Vector2;
struct Vector2f;
struct Vector3f;

struct Quad {
    float x0, y0;
    float x1, y1;
};

struct Vector2 {
    union {
        int x;
        int width;
    };

    union {
        int y;
        int height;
    };
};

struct Vector2f {
    union {
        float x;
        float width;
    };

    union {
        float y;
        float height;
    };
};

struct Vector3f {
    float x;
    float y;
    float z;
};

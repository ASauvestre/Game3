#pragma once

struct Vector2;
struct Vector2f;
struct Vector3f;

struct Vector2 {
    union {
        int x;
        int width;
    };

    union {
        int y;
        int height;
    };

    void operator=(Vector2 b) {
        x = b.x;
        y = b.y;
    }
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

    void operator=(Vector2 b) {
        x = b.x;
        y = b.y;
    }
};

struct Vector3f {
    float x;
    float y;
    float z;
};

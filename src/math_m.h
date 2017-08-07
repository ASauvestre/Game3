#pragma once

struct Vector2;
struct Vector2f;
struct Vector3f;

struct Vector2 {
    Vector2() {}
    Vector2(int x, int y) {
        this->x = x;
        this->y = y;
    }

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

    Vector2f() {};
    Vector2f(float x, float y) {
        this->x = x;
        this->y = y;
    }

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
    Vector3f() {}
    Vector3f(float x, float y, float z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    float x;
    float y;
    float z;
};

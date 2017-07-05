#pragma once

#include <vector>

struct Font;

struct Vector2;
struct Vector2f;
struct Vector3f;
struct Color4f;

struct VertexBuffer;
struct IndexBuffer;

struct Texture;

struct VertexTextureInfo;
struct Vertex;

enum BufferMode;

struct PlatformTextureInfo;

#ifdef COMMON_TYPES_IMPLEMENTATION
#undef COMMON_TYPES_IMPLEMENTATION

#include "stb_truetype.h"

struct Texture {
    char * name;

    int width;
    int height;

    int bytes_per_pixel;

    unsigned char * bitmap;
    int width_in_bytes;
    int num_bytes;

    PlatformTextureInfo * platform_info; // Point to data structure containing platform specific fields
};

struct Font {
    char * name;
    Texture * texture;
    stbtt_bakedchar char_data[96]; // 96 ASCII characters
};

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

struct Color4f {
    Color4f() {}
    Color4f(float r, float g, float b, float a) {
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

struct VertexBuffer {
    std::vector<Vertex> vertices;
};

struct IndexBuffer {
    std::vector<int> indices;
};

struct VertexTextureInfo {
    Vector2f tex_coord;
};

struct Vertex {
    Vertex() {
        // Default constructor
    }

    Vertex( float x, float y, float z, float u, float v) {
        this->position = Vector3f(x, y, z);
        this->tex_info.tex_coord = Vector2f(u,v);
    }

    Vertex( float x, float y, float z, Color4f color) {
        this->position = Vector3f(x, y, z);
        this->color = color;
    }

    Vector3f position;

    union {
        Color4f color;
        VertexTextureInfo tex_info;
    };
};

enum BufferMode {
    QUADS
}; 
#endif
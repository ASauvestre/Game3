#pragma once

#include "array.h"
#include "math_m.h"

enum BufferMode;

struct Shader;
struct DrawBatch;
struct DrawBatchInfo;

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

enum BufferMode {
    QUADS
};

struct GraphicsBuffer {
	Array <DrawBatch> batches;
};

struct DrawBatchInfo {
	char * texture;
	Shader * shader;
};

struct DrawBatch {
	Array<Vector3f> positions;
    Array<Color4f>  colors;
    Array<Vector2f> texture_uvs;

	Array<int>      indices;

	DrawBatchInfo info;
};


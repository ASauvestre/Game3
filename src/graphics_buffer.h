#pragma once

#include "array.h"
#include "math_m.h"

enum BufferMode;

struct Shader;
struct DrawBatch;
struct DrawBatchInfo;
struct Texture;

struct Color4f {
    float r;
    float g;
    float b;
    float a;
};

enum BufferMode {
    QUADS
};

struct GraphicsBuffer {
	Array <DrawBatch> batches;
};

struct DrawBatchInfo {
    Texture * texture;
	Shader  * shader;
};

struct DrawBatch {
	Array<Vector3f> positions;
    Array<Color4f>  colors;
    Array<Vector2f> texture_uvs;

	Array<int>      indices;

	DrawBatchInfo info;
};


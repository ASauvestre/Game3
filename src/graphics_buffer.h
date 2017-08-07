#include "array.h"
#include "math_m.h"

enum BufferMode;

struct VertexBuffer;
struct IndexBuffer;
struct Shader;
struct DrawBatch;
struct DrawBatchInfo;
struct Vertex;

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
    Array<Vertex> vertices;
};

struct IndexBuffer {
    Array<int> indices;
};

// @Cleanup
struct VertexTextureInfo {
    Vector2f tex_coord;
};

struct Vertex {
    Vertex() {}
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
        VertexTextureInfo tex_info; // @Cleanup
    };
};

enum BufferMode {
    QUADS
};

struct GraphicsBuffer {
	Array <DrawBatch> batches;
};

struct DrawBatchInfo {
	char * texture;
	Shader ** shader;
};

struct DrawBatch {
	VertexBuffer vb;
	IndexBuffer ib;

	DrawBatchInfo info;
};


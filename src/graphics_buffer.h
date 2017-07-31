#include "array.h"

struct VertexBuffer;
struct IndexBuffer;
struct Shader;
struct DrawBatch;
struct DrawBatchInfo;

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


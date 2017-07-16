struct VertexBuffer;
struct IndexBuffer;
struct Shader;
struct DrawBatch;
struct DrawBatchInfo;

/*struct GraphicsBuffer {
    GraphicsBuffer() {};
    ~GraphicsBuffer() {};

    std::vector<Shader **> shaders;
    std::vector<VertexBuffer> vertex_buffers;
    std::vector<IndexBuffer> index_buffers;
    std::vector<char *> texture_id_buffer;
};*/

struct GraphicsBuffer {
	std::vector<DrawBatch> batches;
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


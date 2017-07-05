struct VertexBuffer;
struct IndexBuffer;
struct Shader;

struct GraphicsBuffer { // @Temporary Shouldn't be here
    GraphicsBuffer() {};
    ~GraphicsBuffer() {};

    std::vector<Shader *> shaders;
    std::vector<VertexBuffer> vertex_buffers;
    std::vector<IndexBuffer> index_buffers;
    std::vector<char *> texture_id_buffer;
};
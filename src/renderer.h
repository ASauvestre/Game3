#include "graphics_buffer.h"

struct TextureManager;
struct ShaderManager;

// Init
void init_renderer(int width, int height, void * handle); // handle is an HWND used for d3d

// Draw process
void draw(TextureManager * tm);
void clear_buffers();

// Buffering process
void set_shader(Shader * shader);
void set_texture(char * texture);
void set_buffer_mode(BufferMode buffer_mode);

void start_buffer();

void add_vertex(float x, float y, float z, float u, float v);
void add_vertex(float x, float y, float z, Color4f color);


void end_buffer();

// Shaders
void do_load_shader(Shader * shader);

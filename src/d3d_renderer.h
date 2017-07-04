#include <d3d11.h>

#include "win32_texture_manager.h"

#define COMMON_TYPES_IMPLEMENTATION
#include "common_types.h"

enum ShaderInputMode;

struct ShaderInputFormat;
struct Shader;
struct Font;

// Init
void init_renderer(int width, int height);
void init_d3d();
void init_shaders();
void init_textures();
void init_fonts();

// Buffering process
void set_shader(Shader * shader);
void set_texture(char * texture);
void set_buffer_mode(BufferMode buffer_mode);

void start_buffer();

void add_to_buffer(Vertex vertex);

void end_buffer();

// Other
void check_shader_files_modification();

// Private
Font * load_font(char * name);

int get_file_size(FILE * file);

void load_texture(char * name);
void do_load_texture(Texture * texture);

void bind_srv_to_texture(Texture * texture);

bool create_shader(char * filename, char * vs_name, char * ps_name, D3D11_INPUT_ELEMENT_DESC * layout_desc, int num_shader_inputs, ShaderInputMode input_mode, Shader * shader);

bool compile_shader(Shader * shader, wchar_t * path);
bool compile_shader(Shader * shader);
void check_specific_shader_file_modification(Shader * shader);

void draw_buffer(int buffer_index);
void clear_buffers();

void switch_to_shader(Shader shader);

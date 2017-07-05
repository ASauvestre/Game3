//#include "d3d_renderer.h" // @Temporary use .dll

enum BufferMode;

struct Shader;
struct Vertex;
struct Texture;
struct Font;

// Init
void init_renderer(int width, int height, void * handle); // handle is an HWND used for d3d

// Draw process
void draw();
void clear_buffers();

// Buffering process
void set_shader(Shader * shader);
void set_texture(char * texture);
void set_buffer_mode(BufferMode buffer_mode);

void start_buffer();

void add_to_buffer(Vertex vertex);

void end_buffer();

// Loading and creating ressources
Texture * create_texture(char * name, unsigned char * data, int width, int height, int bytes_per_pixel);
Texture * create_texture(char * name, unsigned char * data, int width, int height);

Font * load_font(char * name);
void load_texture(char * name);

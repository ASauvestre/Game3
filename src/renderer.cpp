#include <assert.h>

#include "renderer.h"
#include "texture_manager.h"
#include "graphics_buffer.h"

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "macros.h"

#include "os/layer.h"

// DLL functions
typedef void (*INIT_PLATFORM_RENDERER_FUNC)(Vector2f, void*);
typedef void (*INIT_PLATFORM_SHADERS_FUNC)();
typedef void (*DRAW_FRAME_FUNC)(GraphicsBuffer*, int, TextureManager*);

INIT_PLATFORM_RENDERER_FUNC init_platform_renderer;
INIT_PLATFORM_SHADERS_FUNC init_platform_shaders;
DRAW_FRAME_FUNC draw_frame;

// DLL variables
Shader ** font_shader;
Shader ** colored_shader;
Shader ** textured_shader;

// Extern globals
Font * my_font;

// Private functions
static void init_textures();
static void init_fonts();

static int get_file_size(FILE * file);

static void do_load_texture(Texture * texture);

// Globals
static BufferMode current_buffer_mode = QUADS;

static DrawBatch current_batch;
static DrawBatchInfo previous_batch_info;

static int first_vertex_index_in_buffer = 0;

static GraphicsBuffer graphics_buffer;

static bool buffering = false;

static int num_buffers = 0;

static Vector2f rendering_resolution;

static TextureManager texture_manager;

static

void load_graphics_dll() {
    void * graphics_library_dll = os_specific_load_dll("d3d_renderer.dll"); //@Temporary use file manager ? //@Robustness Handle failed loading (maybe try another dll or at least die gracefully)

    init_platform_renderer = (INIT_PLATFORM_RENDERER_FUNC) os_specific_get_address_from_dll(graphics_library_dll, "init_platform_renderer");
    init_platform_shaders =  (INIT_PLATFORM_SHADERS_FUNC)  os_specific_get_address_from_dll(graphics_library_dll, "init_platform_shaders");
    draw_frame =             (DRAW_FRAME_FUNC)             os_specific_get_address_from_dll(graphics_library_dll, "draw_frame");

    font_shader =     (Shader **) os_specific_get_address_from_dll(graphics_library_dll, "font_shader");
    colored_shader =  (Shader **) os_specific_get_address_from_dll(graphics_library_dll, "colored_shader");
    textured_shader = (Shader **) os_specific_get_address_from_dll(graphics_library_dll, "textured_shader");
}

void init_renderer(int width, int height, void * handle) {
	rendering_resolution.width = width;
	rendering_resolution.height = height;

	load_graphics_dll();

	// From platform renderer
    init_platform_renderer(rendering_resolution, handle);
    init_platform_shaders();

    init_textures();
    init_fonts();
}

static void init_textures() {
    load_texture("grass1.png");
    load_texture("grass2.png");
    load_texture("grass3.png");
    load_texture("dirt_road.png");
    load_texture("dirt_road_bottom.png");
    load_texture("dirt_road_top.png");
    load_texture("megaperson.png");
    load_texture("tree.png");
}

static void init_fonts() {
    my_font = load_font("Inconsolata.ttf");
}


void draw() {
    assert(!buffering);

    // Add the last batch
    graphics_buffer.batches.push_back(current_batch);

	// From platfrom_renderer
	draw_frame(&graphics_buffer, num_buffers, &texture_manager);
}

void set_shader(Shader ** shader) {
    current_batch.info.shader = shader;
}

void set_texture(char * texture) {
    assert(buffering == false); // Can't change texture while we're buffering
    current_batch.info.texture = texture; // @Temporary, find texture here ? At least check it exists.
}

void set_buffer_mode(BufferMode buffer_mode) {
    assert(buffering == false); // Can't change buffer_mode while we're buffering
    current_buffer_mode = buffer_mode;
}

//
// Renderer optimization to begin making it group draw calls when two consecutive calls use the same
// shader and texture (if applicable) -> Almost no performance impact due to drawing editor overlay,
// but since our textures are quite varied (5 of 'em !) little impact on game mode perf. We could sort
// those calls by texture, but then this would mess with the draw order, but we probably shouldn't
// rely much on the order in which we draw things expect for transparent last.
//
// @Think @Optimization
//
void find_or_create_compatible_batch(DrawBatch * batch) {

    if(num_buffers > 0) {
        if(previous_batch_info.shader == batch->info.shader) {
            if(batch->info.shader == colored_shader) {

                first_vertex_index_in_buffer = batch->vb.vertices.size();
                return;
            }

            if((batch->info.shader == font_shader) || (batch->info.shader == textured_shader)) {
                if (strcmp(batch->info.texture, previous_batch_info.texture) == 0) {

                    first_vertex_index_in_buffer = batch->vb.vertices.size();
                    return;
                }
            }
        }

        // No match found, add the previous batch to the buffer. @Cleanup, I don't like this system.
        // Put back the previous_batch_info
        DrawBatchInfo temp_info = batch->info;

        batch->info = previous_batch_info;

        graphics_buffer.batches.push_back(*batch);

        batch->vb.vertices.clear();
        batch->ib.indices.clear();

        batch->info = temp_info;
    }

    first_vertex_index_in_buffer = 0;

    num_buffers++;
}

void start_buffer() {
    assert(buffering == false);
    buffering = true;

    find_or_create_compatible_batch(&current_batch);
}

void add_to_buffer(Vertex vertex) {
    assert(buffering == true);

    current_batch.vb.vertices.push_back(vertex);
}

void end_buffer() {
    assert(buffering == true);

    // Fill the index buffer
    if(current_buffer_mode == QUADS) {

        assert(current_batch.vb.vertices.size() % 4 == 0); // Assert we actually have Quads // @Temporary, we shouldn't crash here.

        int first_index = first_vertex_index_in_buffer;

        for(int i = 0; i < (current_batch.vb.vertices.size() - first_vertex_index_in_buffer) / 4; i++) {
            current_batch.ib.indices.push_back(first_index);
            current_batch.ib.indices.push_back(first_index+1);
            current_batch.ib.indices.push_back(first_index+2);
            current_batch.ib.indices.push_back(first_index);
            current_batch.ib.indices.push_back(first_index+3);
            current_batch.ib.indices.push_back(first_index+1);

            first_index += 4;
        } 
    }

    previous_batch_info = current_batch.info;

	buffering = false;
}

void clear_buffers() {
    // Clear buffer;
    graphics_buffer.batches.clear();

    current_batch.vb.vertices.clear();
    current_batch.ib.indices.clear();

    previous_batch_info.shader = NULL;
	previous_batch_info.texture = NULL;

    num_buffers = 0;
}

Font * load_font(char * name) {
    Font * font = (Font *) malloc(sizeof(Font));

    font->name = name;

    char path[512];
    snprintf(path, 512, "data/fonts/%s", font->name);

    FILE * font_file = fopen(path, "rb");

    if(font_file == NULL) {
        log_print("load_font", "Font %s not found at %s", name, path)
        free(font);
        return NULL;
    }

    int font_file_size = get_file_size(font_file);

    unsigned char * font_file_buffer = (unsigned char *) malloc(font_file_size);

    // log_print("font_loading", "Font file for %s is %d bytes long", my_font->name, font_file_size);

    fread(font_file_buffer, 1, font_file_size, font_file);

    // We no longer need the file
    fclose(font_file);

    unsigned char * bitmap = (unsigned char *) malloc(256*256*4); // Our bitmap is 512x512 pixels and each pixel takes 4 bytes @Robustness, make sure the bitmap is big enough for the font

    int result = stbtt_BakeFontBitmap(font_file_buffer, 0, 16.0, bitmap, 512, 512, 32, 96, font->char_data); // From stb_truetype.h : "no guarantee this fits!""

    if(result <= 0) {
        log_print("load_font", "The font %s could not be loaded, it is too large to fit in a 512x512 bitmap", name);
    }

    font->texture = create_texture(font->name, bitmap, 512, 512, 1);

    free(font_file_buffer);

    log_print("load_font", "Loaded font %s", font->name);

    return font;
}

static int get_file_size(FILE * file) {
    fseek (file , 0 , SEEK_END);
    int size = ftell (file);
    fseek (file , 0 , SEEK_SET);
    return size;
}

Texture * create_texture(char * name, unsigned char * data, int width, int height) {
    return create_texture(name, data, width, height, 4); // Default is 32-bit bitmap.
}

Texture * create_texture(char * name, unsigned char * data, int width, int height, int bytes_per_pixel) {
    Texture * texture = texture_manager.get_new_texture_slot();

    texture->name               = name;
    texture->bitmap             = data;
    texture->width              = width;
    texture->height             = height;
    texture->bytes_per_pixel    = bytes_per_pixel;
    texture->width_in_bytes     = texture->width * texture->bytes_per_pixel;
    texture->num_bytes          = texture->width_in_bytes * texture->height;

    texture_manager.register_texture(texture);

    return texture;
}

void load_texture(char * name) {
    Texture * texture = texture_manager.get_new_texture_slot();

    texture->name = name;

    do_load_texture(texture);

    texture_manager.register_texture(texture);
}

static void do_load_texture(Texture * texture) {

    char path[512];
    snprintf(path, 512, "data/textures/%s", texture->name);
    
    int x,y,n;
    texture->bitmap = stbi_load(path, &x, &y, &n, 4);

    if(texture->bitmap == NULL) {
        log_print("do_load_texture", "Failed to load texture \"%s\"", texture->name);
        return;
    }

    if(n != 4) {
        log_print("do_load_texture", "Loaded texture \"%s\", it has %d bit depth, please convert to 32 bit depth", texture->name, n*8);
        n = 4;
    } else {
        log_print("do_load_texture", "Loaded texture \"%s\"", texture->name);
    }

    texture->width              = x;
    texture->height             = y;
    texture->bytes_per_pixel    = n;
    texture->width_in_bytes     = x * n;
    texture->num_bytes          = texture->width_in_bytes * y;
}
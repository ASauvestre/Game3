#include <assert.h>

#include "renderer.h"
#include "texture_manager.h"
#include "graphics_buffer.h"

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

// Globals
static BufferMode current_buffer_mode = QUADS;

static DrawBatch current_batch;
static DrawBatchInfo previous_batch_info;

static int first_vertex_index_in_buffer = 0;

static GraphicsBuffer graphics_buffer;

static bool buffering = false;

static int num_buffers = 0;

static Vector2f rendering_resolution;

static void load_graphics_dll() {
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
}

void draw(TextureManager * tm) {
    assert(!buffering);

    // Add the last batch
    graphics_buffer.batches.push_back(current_batch);

	// From platfrom_renderer
	draw_frame(&graphics_buffer, num_buffers, tm);
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

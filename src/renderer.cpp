#include <assert.h>

#include "renderer.h"
#include "texture_manager.h"
#include "shader_manager.h"

#include "math_m.h"

#include "macros.h"

#include "os/layer.h"

struct Font;

// DLL functions
typedef void (*INIT_PLATFORM_RENDERER_FUNC) (Vector2f, void*);
typedef bool (*COMPILE_SHADER_FUNC)         (Shader*);
typedef bool (*INIT_FRAME)                  ();
typedef bool (*DRAW_BATCH)                  (DrawBatch*);
typedef bool (*PRESENT_FRAME)               ();

INIT_PLATFORM_RENDERER_FUNC init_platform_renderer;
COMPILE_SHADER_FUNC compile_shader;
INIT_FRAME init_frame;
DRAW_BATCH draw_batch;
PRESENT_FRAME present_frame;

// Prototypes
static void clear_buffers();

// Globals
static BufferMode current_buffer_mode = QUADS;

static DrawBatch * current_batch;
static DrawBatchInfo current_batch_info;
static DrawBatchInfo previous_batch_info;

static int first_vertex_index_in_buffer = 0;

static GraphicsBuffer graphics_buffer;

static bool buffering = false;

static int num_buffers = 0;

static Vector2f rendering_resolution;

static bool frame_initted = false;

static void load_graphics_dll() {
    void * graphics_library_dll = os_specific_load_dll("d3d_renderer.dll"); //@Robustness Handle failed loading (maybe try another dll or at least die gracefully)

    init_platform_renderer = (INIT_PLATFORM_RENDERER_FUNC) os_specific_get_address_from_dll(graphics_library_dll, "init_platform_renderer");
    compile_shader         = (COMPILE_SHADER_FUNC)         os_specific_get_address_from_dll(graphics_library_dll, "compile_shader");
    init_frame             = (INIT_FRAME)                  os_specific_get_address_from_dll(graphics_library_dll, "init_frame");
    draw_batch             = (DRAW_BATCH)                  os_specific_get_address_from_dll(graphics_library_dll, "draw_batch");
    present_frame          = (PRESENT_FRAME)               os_specific_get_address_from_dll(graphics_library_dll, "present_frame");
}

void init_renderer(int width, int height, void * handle) {
    rendering_resolution.width = width;
    rendering_resolution.height = height;

    load_graphics_dll();

    // From platform renderer
    init_platform_renderer(rendering_resolution, handle);
}

void draw_frame() {
    flush_buffers();

    present_frame();
    frame_initted = false;
}

void flush_buffers() {
    perf_monitor();

    assert(!buffering);

    if(!frame_initted) {
        frame_initted = true;
        init_frame();
    }

    for(int i = 0; i < num_buffers; i++) {
        DrawBatch * batch = &graphics_buffer.batches.data[i];
        draw_batch(batch);
    }

    clear_buffers();
}

void set_shader(Shader * shader) {
    current_batch_info.shader = shader;
}

void set_texture(Texture * texture) {
    assert(buffering == false); // Can't change texture while we're buffering
    current_batch_info.texture = texture; // @Temporary, find texture here ? At least check it exists.
}

void set_buffer_mode(BufferMode buffer_mode) {
    assert(buffering == false); // Can't change buffer_mode while we're buffering
    current_buffer_mode = buffer_mode;
}

//
// @Think @Optimization Look up virtual textures to improve batching.
//
void find_or_create_compatible_batch() {

    // @Ugly @Refactor: Fix this.
    if(current_batch) {
        if(previous_batch_info.shader == current_batch_info.shader) {

            Shader * current_shader = current_batch_info.shader;

            // If we have colors
            if(current_shader->color_index >= 0) {
                // And same texture as before if we have textures.
                if(current_shader->uv_index >= 0) {
                    if (current_batch_info.texture == previous_batch_info.texture) {
                        first_vertex_index_in_buffer = current_batch->positions.count;
                        return;
                    }
                } else {
                    first_vertex_index_in_buffer = current_batch->positions.count;
                    return;
                }
            } else {
                // We don't have colors, do we have the same texture as before ?
                if(current_shader->uv_index >= 0) {
                    if (current_batch_info.texture == previous_batch_info.texture) {
                        first_vertex_index_in_buffer = current_batch->positions.count;
                        return;
                    }
                }
            }
        }
    }

    // Create a new batch and add it to the buffer.
    DrawBatch new_batch;
    new_batch.info = current_batch_info;

    // If we had a batch before put the previous batch_info in it again. Since the modifications
    // that happened are meant for the new batch.
    if(current_batch) {
        current_batch->info = previous_batch_info;
    }

    graphics_buffer.batches.add(new_batch);

    // Get pointer to the new batch.
    current_batch = &graphics_buffer.batches.data[graphics_buffer.batches.count - 1];

    first_vertex_index_in_buffer = 0;

    num_buffers++;
}

void start_buffer() {
    assert(buffering == false);

    find_or_create_compatible_batch();

    buffering = true;
}

void add_vertex(float x, float y, float z, float u, float v, Color4f color) {
    assert(buffering == true);

    Vector3f pos = {x, y, z};
    Vector2f uv  = {u, v};

    current_batch->positions.add(pos);
    current_batch->texture_uvs.add(uv);
    current_batch->colors.add(color);
}

void add_vertex(float x, float y, float z, float u, float v) {
    assert(buffering == true);

    Vector3f pos = {x, y, z};
    Vector2f uv  = {u, v};

    current_batch->positions.add(pos);
    current_batch->texture_uvs.add(uv);
}

void add_vertex(float x, float y, float z, Color4f color) {
    assert(buffering == true);

    Vector3f pos = {x, y, z};

    current_batch->positions.add(pos);
    current_batch->colors.add(color);
}

void end_buffer() {
    assert(buffering == true);

    // Fill the index buffer
    if(current_buffer_mode == QUADS) {

        assert(current_batch->positions.count % 4 == 0); // Assert we actually have Quads // @Temporary, we shouldn't crash here.

        int first_index = first_vertex_index_in_buffer;

        for(int i = 0; i < (current_batch->positions.count - first_vertex_index_in_buffer) / 4; i++) {

            // 1---3
            // | \ |   0->1->2 3->2->1 CW
            // 0---2

            current_batch->indices.add(first_index);
            current_batch->indices.add(first_index+1);
            current_batch->indices.add(first_index+2);
            current_batch->indices.add(first_index+3);
            current_batch->indices.add(first_index+2);
            current_batch->indices.add(first_index+1);

            first_index += 4;
        }
    }

    previous_batch_info = current_batch_info;

    buffering = false;
}

static void clear_buffers() {
    for_array(graphics_buffer.batches.data, graphics_buffer.batches.count) {
        it->positions.reset(true);
        it->colors.reset(true);
        it->texture_uvs.reset(true);
        it->indices.reset(true);
    }

    graphics_buffer.batches.reset();

    previous_batch_info.shader = NULL;
    previous_batch_info.texture = NULL;

    num_buffers = 0;
}

// Meh
void do_load_shader(Shader * shader) {
    compile_shader(shader);
}

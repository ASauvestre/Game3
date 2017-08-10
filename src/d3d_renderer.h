#pragma once
#include "macros.h"
#include "math_m.h"

struct GraphicsBuffer;
struct TextureManager;
struct Shader;

extern "C" {
    // Init
    DLLEXPORT void init_platform_renderer(Vector2f rendering_resolution, void * handle);

    // Draw
    DLLEXPORT void draw_frame(GraphicsBuffer * graphics_buffer, int num_buffers, TextureManager * texture_manager);

    // Shaders
    DLLEXPORT bool compile_shader(Shader * shader);
}

#pragma once
#include "macros.h"
#include "math_m.h"

struct GraphicsBuffer;
struct TextureManager;
struct Shader;
struct DrawBatch;

extern "C" {
    // Init
    DLLEXPORT void init_platform_renderer(Vector2f rendering_resolution, void * handle);

    // Draw
    DLLEXPORT void init_frame();

    DLLEXPORT void draw_batch(DrawBatch * batch);

    DLLEXPORT void present_frame(int sync_interval);

    // Shaders
    DLLEXPORT bool compile_shader(Shader * shader);
}

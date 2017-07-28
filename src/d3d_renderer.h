#pragma once
#include "macros.h"

struct GraphicsBuffer;
struct TextureManager;

extern "C" {

// Variables
DLLEXPORT Shader * font_shader;
DLLEXPORT Shader * textured_shader;
DLLEXPORT Shader * colored_shader;

// Init
DLLEXPORT void init_platform_renderer(Vector2f rendering_resolution, void * handle);
DLLEXPORT void init_platform_shaders();

// Draw
DLLEXPORT void draw_frame(GraphicsBuffer * graphics_buffer, int num_buffers, TextureManager * texture_manager);

}

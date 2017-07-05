#pragma once
#include "macros.h"

struct GraphicsBuffer;
struct TextureManager;

// Variables
DLLEXPORT Shader * font_shader;
DLLEXPORT Shader * textured_shader;
DLLEXPORT Shader * colored_shader;
DLLEXPORT Shader * dummy_shader;

// Init
DLLEXPORT void init_platform_renderer(Vector2f rendering_resolution, void * handle);
DLLEXPORT void init_platform_shaders();

// Draw
DLLEXPORT void draw_frame(GraphicsBuffer * graphics_buffer, int num_buffers, TextureManager * texture_manager);
//   ^ Rename it to comething about the platform, also maybe rename platform to graphics_api or 
//     something less confusing (the API isn't related to the platform(win32, osx, linux, ...))
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// @Temporary @Cleanup

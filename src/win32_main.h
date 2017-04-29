#pragma once

#include <windows.h>
#include <d3d11.h>
#include <D3Dcompiler.h>

#include "game_main.h"

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum ShaderInputMode;

void update_window_events();
bool create_window(int width, int height, char * name);
bool create_shader(char * filename, char * vs_name, char * ps_name, D3D11_INPUT_ELEMENT_DESC * layout_desc, int num_shader_inputs, ShaderInputMode input_mode, Shader * shader);
bool compile_shader(Shader * shader, wchar_t * path);
bool compile_shader(Shader * shader);
void init_d3d();
void draw_frame();
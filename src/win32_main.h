#pragma once

#include <windows.h>
#include <d3d11.h>
#include <D3Dcompiler.h>

#include "game_main.h"

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void update_window_events();
bool create_window(int width, int height, char * name);
void init_d3d();
void draw_frame();
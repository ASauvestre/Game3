#pragma once

#include <windows.h>

#include "game_main.h"

void win32_sleep_for_ms(float ms);
void update_window_events();
bool create_window(int width, int height, char * name);
void draw_frame();
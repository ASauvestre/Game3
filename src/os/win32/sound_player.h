#include "windows.h"

void win32_init_sound_player(void * handle);
void win32_play_sounds(double delta_t);
void win32_play_sound_wave(double wave_frequency, float length = -1.0f);

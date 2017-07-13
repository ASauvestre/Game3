void * win32_create_window(int width, int height, char * name);

struct Keyboard;
Keyboard win32_update_keyboard();
bool win32_update_window_events(void * handle);

// Time functions
void win32_init_clock();
double win32_get_time();

void win32_sleep(int ms);
//@Incomplete, rework event system, reduce involvment on this end, make processing happen on the game side

#include "windows.h"

#include "os/win32/core.h"

#include "os/common.h"

#include "macros.h"

// From "windowsx.h"
// Gets Low short and high short from LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

static Keyboard local_keyboard;
static bool local_should_quit; // @Temporary, probably shouldn't get that from here.

static int window_width = 0;
static int window_height = 0;

Keyboard win32_update_keyboard() {
	return local_keyboard;
}

bool win32_update_window_events(void * handle) {
    MSG message;
    while(PeekMessage(&message, (HWND) handle, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return local_should_quit;
}

LRESULT CALLBACK WndProc(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param) {
    switch(message) {
        case WM_QUIT :
        case WM_CLOSE :
        case WM_DESTROY : {
            local_should_quit = true;
            break;
        }
        case WM_KEYDOWN : {

            // if(l_param & 0x40000000) { break; } // Bit 30 "The previous key state. The value is 1 if the key is down
                                                   // before the message is sent, or it is zero if the key is up." (MSDN)

            switch(w_param) {
                case VK_LEFT : {
                    local_keyboard.key_left = true;
                    break;
                }
                case VK_RIGHT : {
                    local_keyboard.key_right = true;
                    break;
                }
                case VK_UP : {
                    local_keyboard.key_up = true;
                    break;
                }
                case VK_DOWN : {
                    local_keyboard.key_down = true;
                    break;
                }
                case VK_SHIFT : {
                    local_keyboard.key_shift = true;
                    break;
                }
                case VK_CONTROL : {
                    break;
                }
                case VK_SPACE : {
                    local_keyboard.key_space = true;
                    break;
                }
                case VK_F1 : {
                    local_keyboard.key_F1 = true;
                    break;
                }
                case VK_F2 : {
                    local_keyboard.key_F2 = true;
                    break;
                }
                case VK_F3 : {
                    local_keyboard.key_F3 = true;
                    break;
                }
                case VK_ESCAPE : {
                    break;
                }
                default : {
                    // printf("Keyboard Event: Unknown key was pressed.\n");
                }
            }
            break;
        }
        case WM_KEYUP : {
            switch(w_param) {
                case VK_LEFT : {
                    local_keyboard.key_left = false;
                    break;
                }
                case VK_RIGHT : {
                    local_keyboard.key_right = false;
                    break;
                }
                case VK_UP : {
                    local_keyboard.key_up = false;
                    break;
                }
                case VK_DOWN : {
                    local_keyboard.key_down = false;
                    break;
                }
                case VK_SHIFT : {
                    local_keyboard.key_shift = false;
                    break;
                }
                case VK_CONTROL : {
                    break;
                }
                case VK_SPACE : {
                    local_keyboard.key_space = false;
                    break;
                }
                case VK_F1 : {
                    local_keyboard.key_F1 = false;
                    break;
                }
                case VK_F2 : {
                    local_keyboard.key_F2 = false;
                    break;
                }
                case VK_F3 : {
                    local_keyboard.key_F3 = false;
                    break;
                }
                case VK_ESCAPE : {
                    SendMessage(window_handle, WM_QUIT, NULL, NULL);
                    break;
                }
                default : {
                    // printf("Keyboard Event: Unknown key was released.\n");
                }
            }
            break;
        }
        case WM_MOUSEMOVE : {
            if(window_width == 0) {
                local_keyboard.mouse_position.x = 0.0f;
            } else {
                local_keyboard.mouse_position.x = (float)GET_X_LPARAM(l_param) / window_width;
            }

            if(window_height == 0) {
                local_keyboard.mouse_position.y = 0.0f;
            } else {
                local_keyboard.mouse_position.y = 1.0f - ((float)GET_Y_LPARAM(l_param) / window_height);
            }

            break;
        }
        case WM_LBUTTONDOWN : {
            local_keyboard.mouse_left = true;

            // SetCapture allows us to detect mouse up if the cursor leaves the window.
            SetCapture(window_handle);


            // Store the position at which the button was pressed @Pasted // @Cleanup, according to the MSDN, those coordinates are relative to the upper left corner
            if(window_width == 0) {
                local_keyboard.mouse_left_pressed_position.x = 0.0f;
            } else {
                local_keyboard.mouse_left_pressed_position.x = (float)GET_X_LPARAM(l_param) / window_width;
            }

            if(window_height == 0) {
                local_keyboard.mouse_left_pressed_position.y = 0.0f;
            } else {
                local_keyboard.mouse_left_pressed_position.y = (float)GET_Y_LPARAM(l_param) / window_height;
            }

            break;
        }
        case WM_LBUTTONUP : {
            local_keyboard.mouse_left = false;
            ReleaseCapture();
            break;
        }
        case WM_RBUTTONDOWN : {
            local_keyboard.mouse_right = true;

            // SetCapture allows us to detect mouse up if the cursor leaves the window.
            SetCapture(window_handle);

            // Store the position at which the button was pressed @Pasted
            if(window_width == 0) {
                local_keyboard.mouse_right_pressed_position.x = 0.0f;
            } else {
                local_keyboard.mouse_right_pressed_position.x = (float)GET_X_LPARAM(l_param) / window_width;
            }

            if(window_height == 0) {
                local_keyboard.mouse_right_pressed_position.y = 0.0f;
            } else {
                local_keyboard.mouse_right_pressed_position.y = (float)GET_Y_LPARAM(l_param) / window_height;
            }

            break;
        }
        case WM_RBUTTONUP : {
            local_keyboard.mouse_right= false;
            ReleaseCapture();
            break;
        }
    }
    return DefWindowProc(window_handle, message, w_param, l_param);
}

void * win32_create_window(int width, int height, char * name) {
	HWND handle;
    window_width = width; // @Temporary eh.
    window_height = height;


    HINSTANCE instance = GetModuleHandle(NULL);
    WNDCLASSEX  window_class = {};
                window_class.cbSize         = sizeof(WNDCLASSEX);
                window_class.style          = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
                window_class.hCursor        = LoadCursor(NULL, IDC_ARROW);
                window_class.lpfnWndProc    = WndProc;
                window_class.hInstance      = instance;
                window_class.lpszClassName  = "Win32WindowClass";

    if(RegisterClassEx(&window_class)) {

        RECT window_dimensions = {};
        window_dimensions.right = width;
        window_dimensions.bottom = height;

        int window_style = WS_CAPTION | WS_SIZEBOX | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

        AdjustWindowRect(&window_dimensions, window_style, false);

        int centered_x = (GetSystemMetrics(SM_CXSCREEN)-width)/2;
        int centered_y = (GetSystemMetrics(SM_CYSCREEN)-height)/2;

        handle = CreateWindowEx(0, window_class.lpszClassName, (LPCTSTR) name, window_style, centered_x,
                                                     centered_y, window_dimensions.right - window_dimensions.left,
                                                     window_dimensions.bottom - window_dimensions.top, NULL, NULL, instance, NULL);

        if(handle) {
            HDC device_context = GetDC((HWND) handle);
            if(device_context) {
                //ShowCursor(false);
                ShowWindow((HWND) handle, 1);
                UpdateWindow((HWND) handle);
            }
        }
    }

    return (void *)handle;
}

// Time functions
LARGE_INTEGER start_time;
LARGE_INTEGER frequency;

void win32_init_clock() {
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start_time);
}

double win32_get_time() {
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return (time.QuadPart - start_time.QuadPart) / (double)frequency.QuadPart;
}

void win32_sleep(int ms) {
    Sleep(ms);
}

// DLL
void * win32_load_dll(char * name) {
    return (void *) LoadLibrary(name);
}
void * win32_get_address_from_dll(void * dll, char * name) {
    return GetProcAddress((HMODULE) dll, name);
}

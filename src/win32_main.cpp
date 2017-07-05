#include "win32_main.h"
#include "macros.h"

// From "windowsx.h" 
// Gets Low short and high short from LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

// Globals
HWND handle;

static HDC device_context;

static bool should_quit = false;

static Keyboard keyboard = {};
static WindowData window_data = {};

// Perf counters
LARGE_INTEGER start_time, end_time;
LARGE_INTEGER frequency;

void update_window_events() {
    MSG message;
    while(PeekMessage(&message, handle, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

LRESULT CALLBACK WndProc(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param) {
    switch(message) {
        case WM_QUIT :
        case WM_CLOSE :
        case WM_DESTROY : {
            should_quit = true;
            break;
        }
        case WM_KEYDOWN : {

            // if(l_param & 0x40000000) { break; } // Bit 30 "The previous key state. The value is 1 if the key is down 
                                                // before the message is sent, or it is zero if the key is up." (MSDN)

            switch(w_param) {
                case VK_LEFT : {
                    keyboard.key_left = true;
                    break;
                }
                case VK_RIGHT : {
                    keyboard.key_right = true;
                    break;
                }
                case VK_UP : {
                    keyboard.key_up = true;
                    break;
                }
                case VK_DOWN : {
                    keyboard.key_down = true;
                    break;
                }
                case VK_SHIFT : {
                    keyboard.key_shift = true;
                    break;
                }
                case VK_CONTROL : {
                    break;
                }
                case VK_SPACE : {
                    keyboard.key_space = true;
                    break;
                }
                case VK_F1 : {
                    keyboard.key_F1 = true;
                    break;
                }
                case VK_F2 : {
                    keyboard.key_F2 = true;
                    break;
                }
                case VK_F3 : {
                    keyboard.key_F3 = true;
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
                    keyboard.key_left = false;
                    break;
                }
                case VK_RIGHT : {
                    keyboard.key_right = false;
                    break;
                }
                case VK_UP : {
                    keyboard.key_up = false;
                    break;
                }
                case VK_DOWN : {
                    keyboard.key_down = false;
                    break;
                }
                case VK_SHIFT : {
                    keyboard.key_shift = false;
                    break;
                }
                case VK_CONTROL : {
                    break;
                }
                case VK_SPACE : {
                    keyboard.key_space = false;
                    break;
                }
                case VK_F1 : {
                    keyboard.key_F1 = false;
                    break;
                }
                case VK_F2 : {
                    keyboard.key_F2 = false;
                    break;
                }
                case VK_F3 : {
                    keyboard.key_F3 = false;
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
            if(window_data.width == 0) {
                keyboard.mouse_position.x = 0.0f;
            } else {
                keyboard.mouse_position.x = (float)GET_X_LPARAM(l_param) / window_data.width;
            }

            if(window_data.height == 0) {
                keyboard.mouse_position.y = 0.0f;
            } else {
                keyboard.mouse_position.y = (float)GET_Y_LPARAM(l_param) / window_data.height;
            }

            break;
        }
        case WM_LBUTTONDOWN : {
            keyboard.mouse_left = true;

            // SetCapture allows us to detect mouse up if the cursor leaves the window.
            SetCapture(window_handle);


            // Store the position at which the button was pressed @Pasted
            if(window_data.width == 0) {
                keyboard.mouse_left_pressed_position.x = 0.0f;
            } else {
                keyboard.mouse_left_pressed_position.x = (float)GET_X_LPARAM(l_param) / window_data.width;
            }

            if(window_data.height == 0) {
                keyboard.mouse_left_pressed_position.y = 0.0f;
            } else {
                keyboard.mouse_left_pressed_position.y = (float)GET_Y_LPARAM(l_param) / window_data.height;
            }

            break;
        }
        case WM_LBUTTONUP : {
            keyboard.mouse_left = false;
            ReleaseCapture();
            break;
        }
        case WM_RBUTTONDOWN : {
            keyboard.mouse_right = true;

            // SetCapture allows us to detect mouse up if the cursor leaves the window.
            SetCapture(window_handle);

            // Store the position at which the button was pressed @Pasted
            if(window_data.width == 0) {
                keyboard.mouse_right_pressed_position.x = 0.0f;
            } else {
                keyboard.mouse_right_pressed_position.x = (float)GET_X_LPARAM(l_param) / window_data.width;
            }

            if(window_data.height == 0) {
                keyboard.mouse_right_pressed_position.y = 0.0f;
            } else {
                keyboard.mouse_right_pressed_position.y = (float)GET_Y_LPARAM(l_param) / window_data.height;
            }

            break;
        }
        case WM_RBUTTONUP : {
            keyboard.mouse_right= false;
            ReleaseCapture();
            break;
        }
    }
    return DefWindowProc(window_handle, message, w_param, l_param);
}

bool create_window(int width, int height, char * name) {
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
            device_context = GetDC(handle);
            if(device_context) {
                //ShowCursor(false);
                ShowWindow(handle, 1);
                UpdateWindow(handle);

            }
        }
    }
    return false;
}

const int TARGET_FPS = 60;

void main() {
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start_time);

    window_data.width   = 1280;
    window_data.height  = 960;

    // window_data.width    = 1920;
    // window_data.height   = 1080;

    window_data.aspect_ratio = (float) window_data.width/window_data.height;
    char * window_name = "Game3";

    create_window(window_data.width, window_data.height, window_name);

    init_renderer(window_data.width, window_data.height, (void *) handle);

    init_game();

    Keyboard previous_keyboard;

    QueryPerformanceCounter(&end_time);
    float frame_time = ((float)(end_time.QuadPart - start_time.QuadPart)*1000/(float)frequency.QuadPart);
    log_print("perf_counter", "Startup time : %.3f seconds", frame_time/1000.f);

    while(!should_quit) {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start_time);

        update_window_events();

        game(&window_data, &keyboard, &previous_keyboard);
        
        draw();
        clear_buffers();

        previous_keyboard = keyboard; // @Framerate

        QueryPerformanceCounter(&end_time);
        
        frame_time = ((float)(end_time.QuadPart - start_time.QuadPart)*1000/(float)frequency.QuadPart);

        if (window_data.locked_fps) {
            while (frame_time < 1000.0f / TARGET_FPS) {
                Sleep(0);

                QueryPerformanceCounter(&end_time);

                frame_time = ((float)(end_time.QuadPart - start_time.QuadPart) * 1000 / (float)frequency.QuadPart);
            }
        }
        window_data.frame_time = frame_time;
        // log_print("perf_counter", "Frame time : %f", frame_time);
    }
}
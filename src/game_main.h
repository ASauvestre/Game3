#include <stdio.h>

#include "d3d_renderer.h" // @Temporary!!!!!!!! Replace by dll.

// Structs
struct Tile;
struct Room;
struct Entity;

struct Vector2f;
struct Vector3f;
struct Color4f;

struct Vertex;
struct VertexBuffer;
struct IndexBuffer;
struct GraphicsBuffer;

struct Shader; // Platform specific definition

struct Keyboard;
struct WindowData;

struct Keyboard {
    bool key_left  = false;
    bool key_right = false;
    bool key_up    = false;
    bool key_down  = false;

    bool key_shift = false;
    bool key_space = false;

    bool key_F1 = false;
    bool key_F2 = false;
    bool key_F3 = false;

    // Mouse input
    Vector2f mouse_position;

    bool mouse_left = false;
    bool mouse_right = false;

    Vector2f mouse_left_pressed_position;
    Vector2f mouse_right_pressed_position;
};

struct WindowData {
    int width;
    int height;
    float aspect_ratio;
    Color4f background_color;

    float frame_time = 0.0f;
    bool locked_fps = false;
};

void init_game();

void game(WindowData * window_data, Keyboard * keyboard, Keyboard * previous_keyboard);


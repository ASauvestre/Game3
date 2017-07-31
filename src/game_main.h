#include <stdio.h>

#include "renderer.h"

#define COMMON_TYPES_IMPLEMENTATION
#include "common_types.h"

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

struct Shader; // API specific definition

struct Keyboard;
struct WindowData;

struct WindowData {
    int width;
    int height;
    float aspect_ratio;
    Color4f background_color;

    double current_time = 0.0f;
    float current_dt = 0.0f;
    bool locked_fps = false;

    void * handle;
};

void init_game();

void game();


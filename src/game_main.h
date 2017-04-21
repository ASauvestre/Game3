#pragma once

#include <stdio.h>
#include <vector>


// Macros
#define log_print(category, format, ...)					\
	char message [2048];									\
	sprintf(message, format, __VA_ARGS__);					\
	printf("[%s]: %s\n", category, message, __VA_ARGS__);	\

#define assert(condition) if(!condition) { *(int *)0 = 0; }

// Constants
const int TILES_PER_ROW		= 32;
const int ROWS_PER_SCREEN	= 18;
const int TILES_PER_SCREEN	= TILES_PER_ROW * ROWS_PER_SCREEN;

// Structs
struct Tile;
struct TileScreen;
struct Vector2f;
struct Vector3f;
struct Color4f;
struct Vertex;
struct VertexBuffer;
struct IndexBuffer;
struct GraphicsBuffer;
struct Keyboard;
struct WindowData;


struct Tile {
	char * texture;
	int local_x;
	int local_y;
};

struct TileScreen {
	Tile tiles[TILES_PER_SCREEN];
};

struct Vector2 {
	Vector2() {}
	Vector2(int x, int y) {
		this->x = x;
		this->y = y;
	}
	int x;
	int y;
};

struct Vector2f {
	
	Vector2f() {}
	;
	Vector2f(float x, float y) {
		this->x = x;
		this->y = y;
	}
	float x;
	float y;
};

struct Vector3f {
	Vector3f(float x, float y, float z) {
		this->x = x;
		this->y = y;
		this->z = z;
	}
	float x;
	float y;
	float z;
};

struct Color4f {
	Color4f() {}
	Color4f(float r, float g, float b, float a) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 1.0f;
};

struct Vertex {
	Vertex(	float x, float y, float z, float u, float v) 
			: position(x,y,z), tex_coord(u, v) {}
	Vertex(	float x, float y, float z, float u, float v, float texid) 
			: position(x,y,z), tex_coord(u, v),  texture_depth(texid) {}
	Vector3f position;
	Vector2f tex_coord;
	float texture_depth;
};

struct VertexBuffer {
	std::vector<Vertex> vertices;
};

struct IndexBuffer {
	std::vector<int> indices;
};

struct GraphicsBuffer {
	std::vector<VertexBuffer> vertex_buffers;
	std::vector<IndexBuffer> index_buffers;
	std::vector<std::vector<char *>> texture_ids;
};

struct Keyboard {
	bool key_left 	= false;
	bool key_right 	= false;
	bool key_up 	= false;
	bool key_down 	= false;

	bool key_shift 	= false;
	bool key_space 	= false;
};

struct WindowData {
	int width;
	int height;
	float aspect_ratio;
	Color4f background_color;
};
void init_game();

void game(WindowData * window_data, Keyboard * keyboard, GraphicsBuffer * graphics_buffer, float dt);

void buffer_player();

void buffer_tiles(TileScreen * tile_screen);

void buffer_title_block();

void buffer_quad(Vertex v1, Vertex v2, Vertex v3, Vertex v4, VertexBuffer * vb, IndexBuffer * ib);

void buffer_quad_centered_at(Vector2f center, float radius, float depth, VertexBuffer * vb, IndexBuffer * ib);
void buffer_quad_centered_at(float radius, float depth, VertexBuffer * vb, IndexBuffer * ib);

void convert_top_left_coords_to_centered(Vertex * v1, Vertex * v2, Vertex * v3, Vertex * v4);
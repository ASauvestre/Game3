#pragma once

#include <stdio.h>
#include <vector>
#include <assert.h>

#include "macros.h"

#include "texture_manager.h"

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

struct Vector2 {
	Vector2() {}
	Vector2(int x, int y) {
		this->x = x;
		this->y = y;
	}
	int x;
	int y;

	void operator=(Vector2 b) {
		x = b.x;
		y = b.y;
	}
};

struct Vector2f {
	
	Vector2f() {};
	Vector2f(float x, float y) {
		this->x = x;
		this->y = y;
	}
	float x;
	float y;

	void operator=(Vector2 b) {
		x = b.x;
		y = b.y;
	}
};

struct Vector3f {
	Vector3f() {}
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

struct VertexTextureInfo {
	Vector2f tex_coord;
};

struct Vertex {
	Vertex(	float x, float y, float z, float u, float v, int texid) {
		this->position = Vector3f(x, y, z);
		this->tex_info.tex_coord = Vector2f(u,v);
	}

	Vertex(	float x, float y, float z, Color4f color) {
		this->position = Vector3f(x, y, z);
		this->color = color;
	}

	Vector3f position;

	union {
		Color4f color;
		VertexTextureInfo tex_info;
	};
};

struct VertexBuffer {
	std::vector<Vertex> vertices;
};

struct IndexBuffer {
	std::vector<int> indices;
};

struct GraphicsBuffer {
	GraphicsBuffer() {};
	~GraphicsBuffer() {};

	std::vector<Shader *> shaders;
	std::vector<VertexBuffer> vertex_buffers;
	std::vector<IndexBuffer> index_buffers;
	std::vector<char *> texture_id_buffer;
};

struct Keyboard {
	bool key_left 	= false;
	bool key_right 	= false;
	bool key_up 	= false;
	bool key_down 	= false;

	bool key_shift 	= false;
	bool key_space 	= false;

	bool key_F1	= false;
};

struct WindowData {
	int width;
	int height;
	float aspect_ratio;
	Color4f background_color;
	float frame_time;
};

int get_file_size(FILE * file);

void init_textures();

void init_game(TextureManager * texture_manager);

Texture * create_texture(char * name, unsigned char * data, int width, int height, int bytes_per_pixel);

Texture * create_texture(char * name, unsigned char * data, int width, int height);

void game(WindowData * window_data, Keyboard * keyboard, GraphicsBuffer * graphics_buffer, TextureManager * texture_manager, float dt);

void buffer_player();

void buffer_trees();

void buffer_entity(Entity entity);

void buffer_tiles(Room * room);

void buffer_editor_tile_overlay(Room * room);

void buffer_title_block();

void buffer_quad(Vertex v1, Vertex v2, Vertex v3, Vertex v4, VertexBuffer * vb, IndexBuffer * ib);

void buffer_quad_centered_at(Vector2f center, float radius, float depth, VertexBuffer * vb, IndexBuffer * ib);
void buffer_quad_centered_at(float radius, float depth, VertexBuffer * vb, IndexBuffer * ib);

void convert_top_left_coords_to_centered(Vertex * v1, Vertex * v2, Vertex * v3, Vertex * v4);
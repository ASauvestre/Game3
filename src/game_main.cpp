#include "game_main.h"

enum GameMode {
	TITLE_SCREEN,
	GAME
};

static GraphicsBuffer * m_graphics_buffer;

static float square_size = 0.5f;

void game(WindowData * window_data, Keyboard * keyboard, GraphicsBuffer * graphics_buffer) {

	m_graphics_buffer = graphics_buffer;

	// Color4f * current_color = &window_data->background_color;
	// current_color->r = (current_color->r + 0.0001f);
	// current_color->g = (current_color->g + 0.0001f);
	// current_color->b = (current_color->b + 0.0001f);

	// if(current_color->r > 1.0f) { current_color->r = 0.0f; }
	// if(current_color->b > 1.0f) { current_color->b = 0.0f; }
	// if(current_color->g > 1.0f) { current_color->g = 0.0f; }

	
	// square_size += d_square_size;

	// if(square_went_full_size) {
	// 	if(square_size < 0.557f ) {
	// 		square_size = 0.557f; 
	// 		d_square_size = 0.00001f; 
	// 		square_went_full_size_and_back = true;
	// 	}

	// 	if(square_size > 0.56f ) {
	// 		if(square_went_full_size_and_back) { 
	// 			square_size = 0.56f; 
	// 			d_square_size = -0.00001f; } 
	// 		}

	// } else {
	// 	if(square_size > 0.58f ) {square_size = 0.58f; d_square_size = -0.0002f; square_went_full_size = true;}
	// }

	// buffer_title_block();

	// TEST
	// Vector2f offset_point = {0.3f, -0.1f};
	// buffer_quad_centered_at(offset_point, square_size/2, 0.2f);

	TileScreen current_tile_screen;

	// @Todo: Make this handle ratio different from screen ratio
	for(int i=0; i<TILES_PER_SCREEN; i++) {
		Tile tile;
		tile.texture = "grass.png";
		tile.local_x = i % TILES_PER_ROW;
		tile.local_y = i / TILES_PER_ROW;

		if(tile.local_x % 2 == 0 && tile.local_y % 2 == 0) {
			// tile.texture = "title_screen_logo.png";
		}

		current_tile_screen.tiles[i] = tile;

	}

	buffer_tiles(&current_tile_screen);

}

void buffer_tiles(TileScreen * tile_screen) {
	VertexBuffer vb;
	IndexBuffer ib;

	std::vector<char *> texture_ids;	

	for(int i=0; i<TILES_PER_SCREEN; i++) {
		Tile tile = tile_screen->tiles[i];

		float tile_width = 1.0f/TILES_PER_ROW;
		float tile_height = 1.0f/ROWS_PER_SCREEN;

		Vertex v1 = {tile_width * (tile.local_x + 0), tile_height * (tile.local_y + 0), 0.0f, 0.0f, 0.0f};
		Vertex v2 = {tile_width * (tile.local_x + 1), tile_height * (tile.local_y + 1), 0.0f, 1.0f, 1.0f};
		Vertex v3 = {tile_width * (tile.local_x + 0), tile_height * (tile.local_y + 1), 0.0f, 0.0f, 1.0f};
		Vertex v4 = {tile_width * (tile.local_x + 1), tile_height * (tile.local_y + 0), 0.0f, 1.0f, 0.0f};

		buffer_quad_gl(v1, v2, v3, v4, &vb, &ib);

		texture_ids.push_back(tile.texture);
	}

	m_graphics_buffer->texture_ids.push_back(texture_ids);
	m_graphics_buffer->vertex_buffers.push_back(vb);
	m_graphics_buffer->index_buffers.push_back(ib);
}

void buffer_title_block() {
	VertexBuffer vb;
	IndexBuffer ib;

	std::vector<char *> texture_ids;
	texture_ids.push_back("title_screen_logo.png");
	m_graphics_buffer->texture_ids.push_back(texture_ids);

	buffer_quad_centered_at(square_size, 0.0f, &vb, &ib);


}

void buffer_quad_centered_at(float radius, float depth, VertexBuffer * vb, IndexBuffer * ib) {
	Vector2f center = {0.0f, 0.0f};
	buffer_quad_centered_at(center, radius, depth, vb, ib);
}

void buffer_quad_centered_at(Vector2f center, float radius, float depth, VertexBuffer * vb, IndexBuffer * ib) {
	Vertex v1 = {center.x - radius, center.y + radius, depth, 0.0f, 0.0f};
	Vertex v2 = {center.x + radius, center.y - radius, depth, 1.0f, 1.0f};
	Vertex v3 = {center.x - radius, center.y - radius, depth, 0.0f, 1.0f};
	Vertex v4 = {center.x + radius, center.y + radius, depth, 1.0f, 0.0f};

	buffer_quad(v1, v2, v3, v4, vb, ib);
}

void buffer_quad(Vertex v1, Vertex v2, Vertex v3, Vertex v4, VertexBuffer * vb, IndexBuffer * ib) {

	int first_index = vb->vertices.size();

	vb->vertices.push_back(v1);
	vb->vertices.push_back(v2);
	vb->vertices.push_back(v3);
	vb->vertices.push_back(v4);
	
	ib->indices.push_back(first_index);
	ib->indices.push_back(first_index+1);
	ib->indices.push_back(first_index+2);
	ib->indices.push_back(first_index);
	ib->indices.push_back(first_index+3);
	ib->indices.push_back(first_index+1);
}

void buffer_quad_gl(Vertex v1, Vertex v2, Vertex v3, Vertex v4, VertexBuffer * vb, IndexBuffer * ib) {
	// Convert from GL coords to centered
	v1.position.x *= 2.0f;
	v2.position.x *= 2.0f;
	v3.position.x *= 2.0f;
	v4.position.x *= 2.0f;

	v1.position.y *= -2.0f;
	v2.position.y *= -2.0f;
	v3.position.y *= -2.0f;
	v4.position.y *= -2.0f;

	v1.position.x -= 1.0f;
	v2.position.x -= 1.0f;
	v3.position.x -= 1.0f;
	v4.position.x -= 1.0f;

	v1.position.y += 1.0f;
	v2.position.y += 1.0f;
	v3.position.y += 1.0f;
	v4.position.y += 1.0f;

	buffer_quad(v1, v2, v3, v4, vb, ib);
}

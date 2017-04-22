#include "game_main.h"

enum GameMode {
	TITLE_SCREEN,
	GAME
};

static GraphicsBuffer * m_graphics_buffer;
static WindowData     * m_window_data;

static float square_size = 0.5f;

struct Entity {
	Entity() {}
	Vector2f position;
	Vector2f velocity;
	char * texture;
	bool is_player;
	float size; // relative to tile
};

Entity player;
Entity trees[30];

Room * current_room;

Vector2f camera_offset;

float tile_width = 1.0f/TILES_PER_ROW;
float tile_height = 1.0f/ROWS_PER_SCREEN; // @Incomplete, handle aspect ratios

void init_game() {
	// Init player
	{
		player.position.x = 0;
		player.position.y = 0;
		player.texture = "megaperson.png";
		player.is_player = true;
		player.size = 1; 
	}

	{
		// TEST Create test room
		Room * room = new Room(64, 64);

		for(int i=0; i<(room->num_tiles); i++) {
			Tile tile;
			tile.texture = "grass.png";
			tile.local_x = i % room->width;
			tile.local_y = i / room->height;

			if(tile.local_x % 2 == 1 && tile.local_y % 2 == 1) {
				tile.texture = "dirt.png";
			}

			room->tiles[i] = tile;

		}

		current_room = room;
	}

	// Init camera position
	
	camera_offset.x = player.position.x;
	camera_offset.y = player.position.y;

	// TEST Init trees
	{
		for(int i = 0; i < array_size(trees)/2; i++) {
			trees[i].position.x = 0.5+(14*i)%32;
			trees[i].position.y = 0.5+(12*i)/32;
			trees[i].texture = "tree.png";
			trees[i].is_player = false;
			trees[i].size = 3;
		}
		for(int i = array_size(trees)/2; i < array_size(trees); i++) {
			trees[i].position.x = 0.5+(12*i)%32;
			trees[i].position.y = 0.5+(16*i)/32;
			trees[i].texture = "tree.png";
			trees[i].is_player = false;
			trees[i].size = 3;
		}

	}
}

void game(WindowData * window_data, Keyboard * keyboard, GraphicsBuffer * graphics_buffer, float dt) {

	m_graphics_buffer = graphics_buffer;
	m_window_data = window_data;


	// Title Screen
	{
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
	}

	// Handle player movement
	{
		float speed = 0.005f;

		if(keyboard->key_shift) speed = 0.02f;

		float position_delta = speed * dt;

		//
		// @Incomplete Need to use and normalize velocity vector
		//
		if(keyboard->key_left) 	player.position.x -= position_delta;
		if(keyboard->key_right)	player.position.x += position_delta;
		if(keyboard->key_up)	player.position.y -= position_delta;
		if(keyboard->key_down) 	player.position.y += position_delta;



		// Clamp player position
		if(player.position.x < 0)  {
			player.position.x = 0;
		}
		if(player.position.x >  current_room->width - 1)  {
			player.position.x = current_room->width - 1;
		}

		if(player.position.y < 0) {
			player.position.y = 0;
		}	
		if(player.position.y >  current_room->height - 1)  {
			player.position.y = current_room->height - 1;
		}

		buffer_player();
	}

	// Camera logic
	{
		camera_offset.x = player.position.x;
		camera_offset.y = player.position.y;
	}


	buffer_tiles(current_room);
	
	buffer_trees();	
}

void buffer_trees() {
	for(int i = 0; i< array_size(trees); i++) {
		VertexBuffer vb;
		IndexBuffer ib;

		Entity tree = trees[i];

		Vector2f screen_pos;
		screen_pos.x = tree.position.x - camera_offset.x + TILES_PER_ROW/2;
		screen_pos.y = tree.position.y - camera_offset.y + ROWS_PER_SCREEN/2;

		if(screen_pos.x + tree.size < 0) continue;
		if(screen_pos.x > TILES_PER_ROW) continue;
		if(screen_pos.y + tree.size < 0) continue;
		if(screen_pos.y > ROWS_PER_SCREEN) continue;


		Vertex v1 = {tile_width * (screen_pos.x + 0), tile_height * (screen_pos.y  + 0), 
					 1.0f - (tree.position.y + tree.size)/ROWS_PER_SCREEN, 0.0f, 0.0f, 0};

		Vertex v2 = {tile_width * (screen_pos.x + trees[i].size), tile_height * (screen_pos.y + tree.size),
					 1.0f - (tree.position.y + tree.size)/ROWS_PER_SCREEN, 1.0f, 1.0f, 0};

		Vertex v3 = {tile_width * (screen_pos.x + 0), tile_height * (screen_pos.y  + tree.size), 
					 1.0f - (tree.position.y + tree.size)/ROWS_PER_SCREEN, 0.0f, 1.0f, 0};

		Vertex v4 = {tile_width * (screen_pos.x + tree.size), tile_height * (screen_pos.y  + 0), 
					 1.0f - (tree.position.y + tree.size)/ROWS_PER_SCREEN, 1.0f, 0.0f, 0};

		
		convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);
		buffer_quad(v1, v2, v3, v4, &vb, &ib);

		std::vector<char *> texture_ids;	
		texture_ids.push_back(trees[i].texture);	
		m_graphics_buffer->texture_ids.push_back(texture_ids);

		m_graphics_buffer->vertex_buffers.push_back(vb);
		m_graphics_buffer->index_buffers.push_back(ib);
	}
}

void buffer_player() {
	VertexBuffer vb;
	IndexBuffer ib;


	// Vertex v1 = {tile_width * (player.position.x + 0), tile_height * (player.position.y + 0), 
	// 			 1.0f - (player.position.y + player.size)/ROWS_PER_SCREEN, 0.0f, 0.0f, 0};

	// Vertex v2 = {tile_width * (player.position.x + player.size), tile_height * (player.position.y + player.size),
	// 			 1.0f - (player.position.y + player.size)/ROWS_PER_SCREEN, 1.0f, 1.0f, 0};

	// Vertex v3 = {tile_width * (player.position.x + 0), tile_height * (player.position.y + player.size), 
	// 			 1.0f - (player.position.y + player.size)/ROWS_PER_SCREEN, 0.0f, 1.0f, 0};

	// Vertex v4 = {tile_width * (player.position.x + player.size), tile_height * (player.position.y + 0), 
	// 			 1.0f - (player.position.y + player.size)/ROWS_PER_SCREEN, 1.0f, 0.0f, 0};

	Vertex v1 = {tile_width * (TILES_PER_ROW/2 + 0), tile_height * (ROWS_PER_SCREEN/2 + 0), 
				 1.0f - (player.position.y + player.size)/ROWS_PER_SCREEN, 0.0f, 0.0f, 0};

	Vertex v2 = {tile_width * (TILES_PER_ROW/2 + player.size), tile_height * (ROWS_PER_SCREEN/2 + player.size),
				 1.0f - (player.position.y + player.size)/ROWS_PER_SCREEN, 1.0f, 1.0f, 0};

	Vertex v3 = {tile_width * (TILES_PER_ROW/2 + 0), tile_height * (ROWS_PER_SCREEN/2 + player.size), 
				 1.0f - (player.position.y + player.size)/ROWS_PER_SCREEN, 0.0f, 1.0f, 0};

	Vertex v4 = {tile_width * (TILES_PER_ROW/2 + player.size), tile_height * (ROWS_PER_SCREEN/2 + 0), 
				 1.0f - (player.position.y + player.size)/ROWS_PER_SCREEN, 1.0f, 0.0f, 0};
	
	convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);
	buffer_quad(v1, v2, v3, v4, &vb, &ib);

	//Vector2f position;
	//position.x = player.position.x * tile_width;
	//position.y = player.position.y * tile_height;

	//buffer_quad_centered_at(position, player.radius, 0.0f, &vb, &ib);

	std::vector<char *> texture_ids;	
	texture_ids.push_back(player.texture);	
	m_graphics_buffer->texture_ids.push_back(texture_ids);

	m_graphics_buffer->vertex_buffers.push_back(vb);
	m_graphics_buffer->index_buffers.push_back(ib);
}

void buffer_tiles(Room * room) {
	VertexBuffer vb;
	IndexBuffer ib;

	std::vector<char *> texture_ids;	

	for(int row = camera_offset.y - ROWS_PER_SCREEN/2; row < camera_offset.y + ROWS_PER_SCREEN/2; row++) {

		if(row < 0) continue;
		if(row >= room->height) continue;


		for(int col = camera_offset.x - TILES_PER_ROW/2; col < camera_offset.x + TILES_PER_ROW/2; col++) {

			if(col < 0) continue;
			if(col >= room->width) continue;

			int tile_index = row * room->width + col;

			Tile tile = room->tiles[tile_index];

			float texture_depth = texture_ids.size();

			bool texture_already_in_buffer = false;
			for(int j = 0; j<texture_ids.size(); j++) {
				char * id = texture_ids[j];
				if(strcmp(id, tile.texture) == 0) {
					texture_already_in_buffer = true;
					texture_depth = j;
				}
			}

			if (!texture_already_in_buffer) texture_ids.push_back(tile.texture);

			Vertex v1 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2 + 0), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2 + 0), 0.99f, 0.0f, 0.0f, texture_depth};
			Vertex v2 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2 + 1), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2 + 1), 0.99f, 1.0f, 1.0f, texture_depth};
			Vertex v3 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2 + 0), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2 + 1), 0.99f, 0.0f, 1.0f, texture_depth};
			Vertex v4 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2 + 1), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2 + 0), 0.99f, 1.0f, 0.0f, texture_depth};


			convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);
			buffer_quad(v1, v2, v3, v4, &vb, &ib);
		}
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
	Vertex v1 = {center.x - radius, center.y + radius*m_window_data->aspect_ratio, depth, 0.0f, 0.0f, 0};
	Vertex v2 = {center.x + radius, center.y - radius*m_window_data->aspect_ratio, depth, 1.0f, 1.0f, 0};
	Vertex v3 = {center.x - radius, center.y - radius*m_window_data->aspect_ratio, depth, 0.0f, 1.0f, 0};
	Vertex v4 = {center.x + radius, center.y + radius*m_window_data->aspect_ratio, depth, 1.0f, 0.0f, 0};

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

void convert_top_left_coords_to_centered(Vertex * v1, Vertex * v2, Vertex * v3, Vertex * v4) {

	v1->position.x *= 2.0f;
	v2->position.x *= 2.0f;
	v3->position.x *= 2.0f;
	v4->position.x *= 2.0f;

	v1->position.y *= -2.0f;
	v2->position.y *= -2.0f;
	v3->position.y *= -2.0f;
	v4->position.y *= -2.0f;

	v1->position.x -= 1.0f;
	v2->position.x -= 1.0f;
	v3->position.x -= 1.0f;
	v4->position.x -= 1.0f;

	v1->position.y += 1.0f;
	v2->position.y += 1.0f;
	v3->position.y += 1.0f;
	v4->position.y += 1.0f;
}

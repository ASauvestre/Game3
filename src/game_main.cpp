#include "game_main.h"

enum GameMode {
	TITLE_SCREEN,
	GAME
};

enum TileType {
	DEFAULT,
	BLOCK,
	SWITCH_ROOM
};

struct Rectangle {
	float x, y;
	float width, height;
};

struct Entity {
	Entity() {}
	Vector2f position;
	Vector2f velocity;
	char * texture;
	bool is_player;
	float size; // relative to tile
};

struct Tile {
	char * texture;
	int local_x;
	int local_y;

	TileType type = DEFAULT;
	int room_target_id; 		 // Used if type is SWITCH_ROOM
	Vector2 target_tile_coords; // Used if type is SWITCH_ROOM

	Rectangle collision_box;

	bool collision_enabled = false;
};

struct Room {
	Room(int w, int h) {

		width = w;
		height = h;

		num_tiles = w*h;
		num_outer_tiles = 2 * (w + h);

		tiles = (Tile *) malloc(num_tiles*sizeof(Tile));
		outer_tiles = (Tile *) malloc(num_outer_tiles * sizeof(Tile));
	}

	~Room() {
		free(tiles);
	}

	int width;
	int height;
	int num_tiles;
	Tile * tiles;

	int num_outer_tiles;
	Tile * outer_tiles; // Border tiles, used for teleport triggers
};


// Constants
const int TILES_PER_ROW		= 32;
const int ROWS_PER_SCREEN	= 18;
const int TILES_PER_SCREEN	= TILES_PER_ROW * ROWS_PER_SCREEN;

// Extern Globals
extern Shader * textured_shader;
extern Shader * colored_shader;

// Globals
static WindowData 		* m_window_data;
static Keyboard m_keyboard;
static GraphicsBuffer 	* m_graphics_buffer;

static float square_size = 0.5f;

static Entity player;
static Entity trees[TILES_PER_SCREEN/60];

Room * rooms[2];

static Room * current_room;

static Vector2f camera_offset;

static float tile_width = 1.0f/TILES_PER_ROW;
static float tile_height = 1.0f/ROWS_PER_SCREEN; // @Incomplete, handle aspect ratios

int find_tile_index_from_coords(int x, int y, Room * room) {
	return (x + 1) * room->width + y - 1;
}

int find_outer_tile_index_from_coords(int x, int y, Room * room) {
	if(y == -1) {
		return x;
	} else if(y == room->height) {
		return room->width + 2*room->height + x;
	} else if (x == -1) {
		return room->width + 2*(y - 1);
	}  else if (x == room->width) {
		return room->width + 2*(y - 1) + 1;
	} else {
		return -1;
	}
}

void init_game() {
	// Init player
	{
		player.position.x = 0;
		player.position.y = 0;
		player.texture = "megaperson.png";
		player.is_player = true;
		player.size = 1.0; 
	}

	{
		// TEST Create test rooms
		{
			Room * room = new Room(50, 30);

			for(int i=0; i<(room->num_tiles); i++) {
				Tile tile;
				tile.texture = "grass.png";
				tile.local_x = i % room->width;
				tile.local_y = i / room->width;

				/*if(tile.local_x % 2 == 1 && tile.local_y % 2 == 1) {
					tile.texture = "dirt.png";
				}*/

				room->tiles[i] = tile;

			}

			for(int i=0; i<(room->num_outer_tiles); i++) {
				Tile tile;

				if(i<room->width) {
					tile.local_x = i;
					tile.local_y = -1;

					tile.collision_box.x = tile.local_x+0.75;
					tile.collision_box.y = tile.local_y;
					tile.collision_box.width = -0.5;
					tile.collision_box.height = 1;
				} else if(i > (room->num_outer_tiles - room->width - 1)) {
					tile.local_x = (i - (room->num_outer_tiles - room->width));
					tile.local_y = room->height;

					tile.collision_box.x = tile.local_x+0.75;
					tile.collision_box.y = tile.local_y;
					tile.collision_box.width = -0.5;
					tile.collision_box.height = 1;
				} else {
					tile.local_x = room->width - (room->width + 1)*((i - room->width + 1)%2);
					tile.local_y = (i - room->width)/2 + 1;

					tile.collision_box.x = tile.local_x;
					tile.collision_box.y = tile.local_y + 0.75;
					tile.collision_box.width = 1;
					tile.collision_box.height = -0.5;
				}

				tile.type = BLOCK;
				tile.collision_enabled = true;

				// log_print("outer_tiles_generation", "Created tile %d at (%d, %d)", i, tile.local_x, tile.local_y);

				room->outer_tiles[i] = tile;
			}

			room->outer_tiles[5].type = SWITCH_ROOM;
			room->outer_tiles[5].room_target_id = 1;
			room->outer_tiles[5].target_tile_coords = Vector2(5, 13);

			room->tiles[5].texture = "dirt.png";

			rooms[0] = room;
		}

		// ----------------------------
		{
			Room * room = new Room(28, 14);

			for(int i=0; i<(room->num_tiles); i++) {
				Tile tile;
				tile.texture = "grass.png";
				tile.local_x = i % room->width;
				tile.local_y = i / room->width;

				/*if(tile.local_x % 2 == 1 && tile.local_y % 2 == 1) {
					tile.texture = "dirt.png";
				}*/

				room->tiles[i] = tile;
			}

			for(int i=0; i<(room->num_outer_tiles); i++) {
				Tile tile;

				if(i<room->width) {
					tile.local_x = i;
					tile.local_y = -1;

					tile.collision_box.x = tile.local_x+0.75;
					tile.collision_box.y = tile.local_y;
					tile.collision_box.width = -0.5;
					tile.collision_box.height = 1;
				} else if(i > (room->num_outer_tiles - room->width - 1)) {
					tile.local_x = (i - (room->num_outer_tiles - room->width));
					tile.local_y = room->height;

					tile.collision_box.x = tile.local_x+0.75;
					tile.collision_box.y = tile.local_y;
					tile.collision_box.width = -0.5;
					tile.collision_box.height = 1;
				} else {
					tile.local_x = room->width + 1 - (room->width + 2)*((i - room->width + 1)%2);
					tile.local_y = (i - room->width)/2 + 1;

					tile.collision_box.x = tile.local_x;
					tile.collision_box.y = tile.local_y + 0.75;
					tile.collision_box.width = 1;
					tile.collision_box.height = -0.5;
				}

				tile.type = BLOCK;
				tile.collision_enabled = true;

				// log_print("outer_tiles_generation", "Created tile %d at (%d, %d)", i, tile.local_x, tile.local_y);

				room->outer_tiles[i] = tile;
			}

			room->outer_tiles[61].type = SWITCH_ROOM;
			room->outer_tiles[61].room_target_id = 0;
			room->outer_tiles[61].target_tile_coords = Vector2(5, 0);



			room->tiles[369].texture = "dirt.png";


			rooms[1] = room;
		}

		current_room = rooms[0];
	}



	// TEST Init trees
	{
		for(int i = 0; i < array_size(trees)/2; i++) {
			trees[i].position.x = (int)(8.9*i)%(current_room->width-2);
			trees[i].position.y = (int)(2.3*i)%(current_room->height-2);
			trees[i].texture = "tree.png";
			trees[i].is_player = false;
			trees[i].size = 3;
		}
		for(int i = array_size(trees)/2; i < array_size(trees); i++) {
			trees[i].position.x = (int)(5.5*i)%(current_room->width-2);
			trees[i].position.y = (int)(4.1*i)%(current_room->height-2);
			trees[i].texture = "tree.png";
			trees[i].is_player = false;
			trees[i].size = 3;
		}

	}
}

void game(WindowData * window_data, Keyboard * keyboard, GraphicsBuffer * graphics_buffer, float dt) {

	m_window_data = window_data;
	m_keyboard = *keyboard;
	m_graphics_buffer = graphics_buffer;


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
		float speed = 0.006f;

		if(keyboard->key_shift) speed = 0.02f;

		float position_delta = speed * dt;

		//
		// @Incomplete Need to use and normalize velocity vector
		//
		if(keyboard->key_left) 	player.position.x -= position_delta;
		if(keyboard->key_right)	player.position.x += position_delta;
		if(keyboard->key_up)	player.position.y -= position_delta;
		if(keyboard->key_down) 	player.position.y += position_delta;
	}

	// Early collision detection, find current tile
	{
		// @Optimisation We can probably infer the tiles we collide with from the player's x and y coordinates and information about the room's size
		// for(int i = 0; i < current_room->num_tiles; i++) {
		// 	Tile tile = current_room->tiles[i];

		// 	bool should_compute_collision = tile.collision_enabled;

		// 	if(should_compute_collision) {


		// 		if((player.position.x < tile.collision_box.x + tile.collision_box.width) && (tile.collision_box.x < player.position.x + player.size) &&	// X tests
		// 		   (player.position.y < tile.collision_box.y + tile.collision_box.height) && (tile.collision_box.y < player.position.y + player.size)) {	// Y tests

		// 			if(tile.type == SWITCH_ROOM) {
		// 				log_print("collision", "Player is on a tile with type SWITCH_ROOM, switching to room %d", tile.room_target_id);
		// 				current_room = rooms[tile.room_target_id];

		// 				player.position = tile.target_tile_coords;
		// 			}

		// 		}

		// 	}
		// }

		// @Optimisation We can probably infer the tiles we collide with from the player's x and y coordinates and information about the room's size
		for(int i = 0; i < 2 * (current_room->width + current_room->height - 2); i++) {
			Tile tile = current_room->outer_tiles[i];

			bool should_compute_collision = tile.collision_enabled;

			if(should_compute_collision) {


				if((player.position.x < tile.collision_box.x + tile.collision_box.width) && (tile.collision_box.x < player.position.x + player.size) &&	// X tests
				   (player.position.y < tile.collision_box.y + tile.collision_box.height) && (tile.collision_box.y < player.position.y + player.size)) {	// Y tests

					if(tile.type == SWITCH_ROOM) {
						// log_print("collision", "Player is on a tile with type SWITCH_ROOM, switching to room %d", tile.room_target_id);
						current_room = rooms[tile.room_target_id];

						player.position.x = tile.target_tile_coords.x + (player.position.x - tile.local_x);
						player.position.y = tile.target_tile_coords.y + (player.position.y - tile.local_y);

						break;
					}

				}

			}
		}
	}


	// Clamp player position
	{
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
	}

	// Camera logic
	{
		camera_offset.x = player.position.x;
		camera_offset.y = player.position.y;

		if(current_room->width <= TILES_PER_ROW) {
			camera_offset.x = current_room->width/2.0f;
		}
		else if(camera_offset.x < TILES_PER_ROW/2.0f) {
			camera_offset.x = TILES_PER_ROW/2.0f;
		}
		else if(camera_offset.x > current_room->width - TILES_PER_ROW/2.0f) {
			camera_offset.x = current_room->width - TILES_PER_ROW/2.0f;
		}

		if(current_room->height <= ROWS_PER_SCREEN) {
			camera_offset.y = current_room->height/2.0f;
		}
		else if(camera_offset.y < ROWS_PER_SCREEN/2.0f) {
			camera_offset.y = ROWS_PER_SCREEN/2.0f;
		}
		else if(camera_offset.y > current_room->height - ROWS_PER_SCREEN/2.0f) {
			camera_offset.y = current_room->height - ROWS_PER_SCREEN/2.0f;
		}
			
		// log_print("[game_camera]", "Camera offset is (%f, %f)", camera_offset.x, camera_offset.y);

	}

	buffer_tiles(current_room);
	
	buffer_trees();	

	buffer_player();
}

void buffer_trees() {
	for(int i = 0; i< array_size(trees); i++) {
		buffer_entity(trees[i]);
	}
}

void buffer_player() {
	buffer_entity(player);
}

void buffer_entity(Entity entity) {
		VertexBuffer vb;
		IndexBuffer ib;

		Vector2f screen_pos;
		screen_pos.x = entity.position.x - camera_offset.x + TILES_PER_ROW/2;
		screen_pos.y = entity.position.y - camera_offset.y + ROWS_PER_SCREEN/2;

		if(screen_pos.x + entity.size < 0) return;
		if(screen_pos.x > TILES_PER_ROW) return;
		if(screen_pos.y + entity.size < 0) return;
		if(screen_pos.y > ROWS_PER_SCREEN) return;


		Vertex v1 = {tile_width * (screen_pos.x + 0), tile_height * (screen_pos.y  + 0), 
					 1.0f - (entity.position.y + entity.size)/current_room->height, 0.0f, 0.0f, 0};

		Vertex v2 = {tile_width * (screen_pos.x + entity.size), tile_height * (screen_pos.y + entity.size),
					 1.0f - (entity.position.y + entity.size)/current_room->height, 1.0f, 1.0f, 0};

		Vertex v3 = {tile_width * (screen_pos.x + 0), tile_height * (screen_pos.y  + entity.size), 
					 1.0f - (entity.position.y + entity.size)/current_room->height, 0.0f, 1.0f, 0};

		Vertex v4 = {tile_width * (screen_pos.x + entity.size), tile_height * (screen_pos.y  + 0), 
					 1.0f - (entity.position.y + entity.size)/current_room->height, 1.0f, 0.0f, 0};

		
		convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);
		buffer_quad(v1, v2, v3, v4, &vb, &ib);

		std::vector<char *> texture_ids;	
		texture_ids.push_back(entity.texture);	
		m_graphics_buffer->texture_ids.push_back(texture_ids);

		m_graphics_buffer->vertex_buffers.push_back(vb);
		m_graphics_buffer->index_buffers.push_back(ib);

		m_graphics_buffer->shaders.push_back(textured_shader);
}

void buffer_tiles(Room * room) {
	VertexBuffer vb;
	IndexBuffer ib;

	std::vector<char *> texture_ids;	

	for(int row = camera_offset.y - ROWS_PER_SCREEN/2.0f; row < camera_offset.y + ROWS_PER_SCREEN/2.0f; row++) {

		if(row < 0) continue;
		if(row >= room->height) continue;


		for(int col = camera_offset.x - TILES_PER_ROW/2.0f; col < camera_offset.x + TILES_PER_ROW/2.0f; col++) {

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

			Vertex v1 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 0), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 0), 0.99f, 0.0f, 0.0f, texture_depth};
			Vertex v2 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 1), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 1), 0.99f, 1.0f, 1.0f, texture_depth};
			Vertex v3 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 0), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 1), 0.99f, 0.0f, 1.0f, texture_depth};
			Vertex v4 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 1), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 0), 0.99f, 1.0f, 0.0f, texture_depth};

			convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);
			buffer_quad(v1, v2, v3, v4, &vb, &ib);
		}
	}

	m_graphics_buffer->texture_ids.push_back(texture_ids);
	m_graphics_buffer->vertex_buffers.push_back(vb);
	m_graphics_buffer->index_buffers.push_back(ib);

	m_graphics_buffer->shaders.push_back(textured_shader);
}

void buffer_title_block() {
	VertexBuffer vb;
	IndexBuffer ib;

	std::vector<char *> texture_ids;
	texture_ids.push_back("title_screen_logo.png");
	m_graphics_buffer->texture_ids.push_back(texture_ids);

	buffer_quad_centered_at(square_size, 0.0f, &vb, &ib);

	m_graphics_buffer->shaders.push_back(textured_shader);
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

#include "game_main.h"

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

enum GameMode {
	TITLE_SCREEN,
	GAME,
	EDITOR
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
	int room_target_id; 		// Used if type is SWITCH_ROOM
	Vector2 target_tile_coords; // Used if type is SWITCH_ROOM

	Rectangle collision_box;

	bool collision_enabled = false;
};

struct Room {
	Room(char * n, int w, int h) {
		name = n;
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

	char * name;

	int width;
	int height;
	int num_tiles;
	Tile * tiles;

	int num_outer_tiles;
	Tile * outer_tiles; // Border tiles, used for teleport triggers
};

struct Font {
	char * name;
	Texture * texture;
	stbtt_bakedchar char_data[96]; // 96 ASCII characters
};

// Constants
const int TILES_PER_ROW		= 32;
const int ROWS_PER_SCREEN	= 18;
const int TILES_PER_SCREEN	= TILES_PER_ROW * ROWS_PER_SCREEN;

const float DEBUG_OVERLAY_Z  = 0.0f;
const float EDITOR_OVERLAY_Z = 0.001f;

// Extern Globals
extern Shader * font_shader;
extern Shader * textured_shader;
extern Shader * colored_shader;

// Globals
static WindowData 		* m_window_data;
static Keyboard 		* m_keyboard;
static GraphicsBuffer 	* m_graphics_buffer;
static TextureManager 	* m_texture_manager;

static Font * my_font;

static float square_size = 0.5f;

static Entity player;
static Entity trees[TILES_PER_SCREEN/60];

Room * rooms[2];

static Room * current_room;

static Vector2f camera_offset;

static float tile_width = 1.0f/TILES_PER_ROW;
static float tile_height = 1.0f/ROWS_PER_SCREEN; // @Incomplete, handle aspect ratios

static GameMode game_mode;
static bool debug_overlay_enabled = false;

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

Vector2f find_tile_coords_from_index(int index, Room * room) {
	Vector2f result;
	result.x = room->tiles[index].local_x;
	result.y = room->tiles[index].local_y;
	return result;
}

Vector2f find_outer_tile_coords_from_index(int index, Room * room) {
	Vector2f result;
	result.x = room->outer_tiles[index].local_x;
	result.y = room->outer_tiles[index].local_y;
	return result;
}

Room * generate_room(char * name, int width, int height) {
	Room * room = new Room(name, width, height);

	for(int i=0; i<(room->num_tiles); i++) {
		Tile tile;
		tile.texture = "grass.png";
		tile.local_x = i % room->width;
		tile.local_y = i / room->width;

		room->tiles[i] = tile;
	}

	for(int i=0; i<(room->num_outer_tiles); i++) {
		Tile tile;

		if(i<room->width) {
			tile.local_x = i;
			tile.local_y = -1;

			tile.collision_box.x = tile.local_x + 0.75;
			tile.collision_box.y = tile.local_y;
			tile.collision_box.width = -0.5;
			tile.collision_box.height = 1;
		} else if(i > (room->num_outer_tiles - room->width - 1)) {
			tile.local_x = (i - (room->num_outer_tiles - room->width));
			tile.local_y = room->height;

			tile.collision_box.x = tile.local_x + 0.75;
			tile.collision_box.y = tile.local_y;
			tile.collision_box.width = -0.5;
			tile.collision_box.height = 1;
		} else {
			tile.local_x = room->width - (room->width + 1)*((i - room->width)%2);
			tile.local_y = (i - room->width)/2;

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

	return room;
}

void init_game(TextureManager * texture_manager) {
	m_texture_manager = texture_manager;
	
	// Load textures
	init_textures();

	init_fonts();

	// Init player
	{
		player.position.x = 0;
		player.position.y = 0;
		player.texture = "megaperson.png";
		player.is_player = true;
		player.size = 1.0; 
	}

	// TEST Create test rooms
	{
		
		Room * room = generate_room("main_room", 50, 30);

		room->outer_tiles[5].type = SWITCH_ROOM;
		room->outer_tiles[5].room_target_id = 1;
		room->outer_tiles[5].target_tile_coords = Vector2(5, 13);

		room->tiles[5].texture = "dirt.png";

		rooms[0] = room;

		// ----------------------------

		room = generate_room("small_room", 28, 14);

		room->outer_tiles[61].type = SWITCH_ROOM;
		room->outer_tiles[61].room_target_id = 0;
		room->outer_tiles[61].target_tile_coords = Vector2(5, 0);

		room->tiles[369].texture = "dirt.png";

		rooms[1] = room;
	
	}

	current_room = rooms[0];
	game_mode = GAME;

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

Font * load_font(char * name) {
	Font * font = (Font *) malloc(sizeof(Font));

	font->name = name;

	char path[512];
	snprintf(path, 512, "fonts/%s", font->name);

	FILE * font_file = fopen(path, "rb");

	if(font_file == NULL) {
		log_print("load_font", "Font %s not found at %s", name, path)
		free(font);
		return NULL;
	}

	int font_file_size = get_file_size(font_file);

	unsigned char * font_file_buffer = (unsigned char *) malloc(font_file_size);

	// log_print("font_loading", "Font file for %s is %d bytes long", my_font->name, font_file_size);

	fread(font_file_buffer, 1, font_file_size, font_file);

	// We no longer need the file
	fclose(font_file);

	unsigned char * bitmap = (unsigned char *) malloc(512*512*4); // Our bitmap is 512x512 pixels and each pixel takes 4 bytes

	int result = stbtt_BakeFontBitmap(font_file_buffer, 0, 16.0, bitmap, 512, 512, 32, 96, font->char_data); // no guarantee this fits!

	if(result <= 0) {
		log_print("load_font", "The font %s could not be loaded, it is too large to fit in a 512x512 bitmap", name);
	}

	font->texture = create_texture(font->name, bitmap, 512, 512, 1);

	free(font_file_buffer);

	log_print("load_font", "Loaded font %s", font->name);

	return font;
}

int get_file_size(FILE * file) {
  	fseek (file , 0 , SEEK_END);
  	int size = ftell (file);
  	fseek (file , 0 , SEEK_SET);
  	return size;
}

// Texture loading
void do_load_texture(Texture * texture) {

	char path[512];
	snprintf(path, 512, "textures/%s", texture->name);
	
	int x,y,n;
	texture->bitmap = stbi_load(path, &x, &y, &n, 4);

	if(texture->bitmap == NULL) {
		log_print("do_load_texture", "Failed to load texture \"%s\"", texture->name);
		return;
	}

	if(n != 4) {
		log_print("do_load_texture", "Loaded texture \"%s\", it has %d bit depth, please convert to 32 bit depth", texture->name, n*8);
		n = 4;
	} else {
		log_print("do_load_texture", "Loaded texture \"%s\"", texture->name);

	}

	texture->width 				= x;
	texture->height 			= y;
	texture->bytes_per_pixel 	= n;
	texture->width_in_bytes		= x * n;
	texture->num_bytes	 		= texture->width_in_bytes * y;
}

Texture * create_texture(char * name, unsigned char * data, int width, int height, int bytes_per_pixel) {
	Texture * texture = m_texture_manager->get_new_texture_slot();

	texture->name 				= name;
	texture->bitmap 			= data;
	texture->width 				= width;
	texture->height 			= height;
	texture->bytes_per_pixel 	= bytes_per_pixel;
	texture->width_in_bytes		= texture->width * texture->bytes_per_pixel;
	texture->num_bytes	 		= texture->width_in_bytes * texture->height;

	m_texture_manager->register_texture(texture);

	return texture;
}

Texture * create_texture(char * name, unsigned char * data, int width, int height) {
	return create_texture(name, data, width, height, 4); // Default is 32-bit bitmap.
}

void load_texture(char * name) {
	Texture * texture = m_texture_manager->get_new_texture_slot();

	texture->name = name;

	do_load_texture(texture);

	m_texture_manager->register_texture(texture);
}

void init_textures() {
	load_texture("title_screen_logo.png");
	load_texture("grass.png");
	load_texture("dirt.png");
	load_texture("megaperson.png");
	load_texture("tree.png");
	load_texture("tree_window.png");
}

void init_fonts() {
	my_font = load_font("Inconsolata.ttf");
}

void buffer_string(char * text, float x, float y, float z,  Font * font) {
	VertexBuffer vb;
	IndexBuffer ib;

	while(*text) {
		// Make sure it's an ascii character
		if (*text >= 32 && *text < 128) {
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(font->char_data, 512,512, *text-32, &x,&y,&q,1);
		
			Vertex v1 = {q.x0/m_window_data->width, q.y0/m_window_data->height, z, q.s0, q.t0, 0};
			Vertex v2 = {q.x1/m_window_data->width, q.y1/m_window_data->height, z, q.s1, q.t1, 0};
			Vertex v3 = {q.x0/m_window_data->width, q.y1/m_window_data->height, z, q.s0, q.t1, 0};
			Vertex v4 = {q.x1/m_window_data->width, q.y0/m_window_data->height, z, q.s1, q.t0, 0};

			convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);

			buffer_quad(v1, v2, v3, v4, &vb, &ib);

		}
		++text;
   }

   	m_graphics_buffer->vertex_buffers.push_back(vb);
	m_graphics_buffer->index_buffers.push_back(ib);
	m_graphics_buffer->texture_id_buffer.push_back(font->texture->name);
	m_graphics_buffer->shaders.push_back(font_shader);
}

void game(WindowData * window_data, Keyboard * keyboard, Keyboard * previous_keyboard, GraphicsBuffer * graphics_buffer, TextureManager * texture_manager, float dt) { // @Redundant dt is now also in WindowData
	
	Keyboard *  m_previous_keyboard = previous_keyboard;

	m_window_data 	  = window_data;
	m_keyboard 		  = keyboard;
	m_graphics_buffer = graphics_buffer;
	m_texture_manager = texture_manager;

	if(m_keyboard->key_F1 && !m_previous_keyboard->key_F1) {
		if(game_mode == GAME) {
			game_mode = EDITOR;
		}
		else if(game_mode == EDITOR) {
			game_mode = GAME;
		}
	}

	if(m_keyboard->key_F2 && !m_previous_keyboard->key_F2) {
		debug_overlay_enabled = !debug_overlay_enabled;
	}

	if(m_keyboard->key_F3 && !m_previous_keyboard->key_F3) {
		m_window_data->locked_fps = !m_window_data->locked_fps;
	}

	if(game_mode == GAME) {
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
	}

	// Camera logic
	{

		if(game_mode == GAME) {
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
		} else if (game_mode == EDITOR) {
			//	@Refactor Same code as player movement

			float speed = 0.006f;

			if(keyboard->key_shift) speed = 0.02f;

			float position_delta = speed * dt;

			//
			// @Incomplete Need to use and normalize velocity vector
			//
			if(keyboard->key_left) 	camera_offset.x -= position_delta;
			if(keyboard->key_right)	camera_offset.x += position_delta;
			if(keyboard->key_up)	camera_offset.y -= position_delta;
			if(keyboard->key_down) 	camera_offset.y += position_delta;
		}
		// log_print("[game_camera]", "Camera offset is (%f, %f)", camera_offset.x, camera_offset.y);
	}

	buffer_tiles(current_room);
	
	buffer_trees();	

	buffer_player();

	// Editor overlays
	if(game_mode == EDITOR) {
		buffer_editor_tile_overlay(current_room);
	}

	// Debug overlay 
	if(debug_overlay_enabled) {
		buffer_debug_overlay();
	}
}

int frame_time_print_counter = 0;

const int FRAME_TIME_UPDATE_DELAY = 15; // in frames

float displayed_frame_time = 0.0f;
float summed_frame_rate = 0.0f;

void buffer_debug_overlay() {
	if(frame_time_print_counter == 0) { // @Temporary Updated every 15 frames and average of those 15 frames, maybe find a better system.
		displayed_frame_time = summed_frame_rate / FRAME_TIME_UPDATE_DELAY;

		// log_print("perf_counter", "Average of %d frames is %f", FRAME_TIME_UPDATE_DELAY, displayed_frame_time);

		summed_frame_rate = 0.0f;
		frame_time_print_counter = FRAME_TIME_UPDATE_DELAY;
	}

	summed_frame_rate += m_window_data->frame_time;
	frame_time_print_counter--;
	
	// Buffer Frame time
	{
		float x = m_window_data->width - 160;
		float y = 20.0f;

		char buffer[64];

		if(displayed_frame_time < 10.0f) {
			// Add a space
			snprintf(buffer, sizeof(buffer), "Frame Time :  %.3f", displayed_frame_time);
		} else {
			snprintf(buffer, sizeof(buffer), "Frame Time : %.3f", displayed_frame_time);
		}

		buffer_string(buffer, x, y, DEBUG_OVERLAY_Z, my_font);
	}
}

void buffer_editor_tile_overlay(Room * room) {
	VertexBuffer vb;
	IndexBuffer ib;

	// Inner tiles
	for(int tile_index = 0; tile_index < room->num_tiles; tile_index++) {
		Tile tile = room->tiles[tile_index];
		int col = tile.local_x;
		int row = tile.local_y;

		// Don't buffer out of screen tiles
		if (col + 1 < camera_offset.x - TILES_PER_ROW/2.0f)   continue;
		if (col 	> camera_offset.x + TILES_PER_ROW/2.0f)   continue;
		if (row + 1 < camera_offset.y - ROWS_PER_SCREEN/2.0f) continue;
		if (row 	> camera_offset.y + ROWS_PER_SCREEN/2.0f) continue;

		if (col < 0) 			 continue;
		if (col >= room->width)  continue;
		if (row < 0) 			 continue;
		if (row >= room->height) continue;


		Color4f color = Color4f(1.0f, 1.0f, 1.0f, 0.5f);

		if(tile.type == SWITCH_ROOM) {
			color = Color4f(1.0f, 0.0f, 0.0f, 0.5f);
		} else if(tile.type == BLOCK) {
			color = Color4f(0.0f, 1.0f, 1.0f, 0.5f);
		}

		Vertex v1 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 0), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 0), EDITOR_OVERLAY_Z, color};
		Vertex v2 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 1), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 1), EDITOR_OVERLAY_Z, color};
		Vertex v3 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 0), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 1), EDITOR_OVERLAY_Z, color};
		Vertex v4 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 1), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 0), EDITOR_OVERLAY_Z, color};

		convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);
		buffer_quad(v1, v2, v3, v4, &vb, &ib);
	}


	// @Cleanup Refactor those loops ? They're exactly the same.
	// Outer tiles
	for(int tile_index = 0; tile_index < room->num_outer_tiles; tile_index++) {
		Tile tile = room->outer_tiles[tile_index];
		int col = tile.local_x;
		int row = tile.local_y;

		// Don't buffer out of screen tiles
		if (col + 1 < camera_offset.x - TILES_PER_ROW/2.0f)   continue;
		if (col 	> camera_offset.x + TILES_PER_ROW/2.0f)   continue;
		if (row + 1 < camera_offset.y - ROWS_PER_SCREEN/2.0f) continue;
		if (row 	> camera_offset.y + ROWS_PER_SCREEN/2.0f) continue;

		if (col < -1) 			continue;
		if (col > room->width)  continue;
		if (row < -1) 			continue;
		if (row > room->height) continue;


		Color4f color = Color4f(1.0f, 1.0f, 1.0f, 0.5f);

		if(tile.type == SWITCH_ROOM) {
			color = Color4f(1.0f, 0.0f, 0.0f, 0.5f);
		} else if(tile.type == BLOCK) {
			color = Color4f(0.0f, 1.0f, 1.0f, 0.5f);
		}

		Vertex v1 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 0), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 0), EDITOR_OVERLAY_Z, color};
		Vertex v2 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 1), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 1), EDITOR_OVERLAY_Z, color};
		Vertex v3 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 0), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 1), EDITOR_OVERLAY_Z, color};
		Vertex v4 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 1), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 0), EDITOR_OVERLAY_Z, color};

		convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);
		buffer_quad(v1, v2, v3, v4, &vb, &ib);
	}

	m_graphics_buffer->vertex_buffers.push_back(vb);
	m_graphics_buffer->index_buffers.push_back(ib);

	// @Temporary Required because otherwise, the texture buffer is no longer synced with the other buffers
	m_graphics_buffer->texture_id_buffer.push_back("placeholder");
	
	m_graphics_buffer->shaders.push_back(colored_shader);
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

		VertexBuffer vb;
		IndexBuffer ib;

		buffer_quad(v1, v2, v3, v4, &vb, &ib);

		m_graphics_buffer->vertex_buffers.push_back(vb);
		m_graphics_buffer->index_buffers.push_back(ib);
		m_graphics_buffer->texture_id_buffer.push_back(entity.texture);
		m_graphics_buffer->shaders.push_back(textured_shader);
}

void buffer_tiles(Room * room) {

	for(int tile_index = 0; tile_index < room->num_tiles; tile_index++) {
		Tile tile = room->tiles[tile_index];
		int col = tile.local_x;
		int row = tile.local_y;

		// Don't buffer out of screen tiles
		if (col + 1 < camera_offset.x - TILES_PER_ROW/2.0f)   continue;
		if (col > camera_offset.x + TILES_PER_ROW/2.0f)   continue;
		if (row + 1 < camera_offset.y - ROWS_PER_SCREEN/2.0f) continue;
		if (row > camera_offset.y + ROWS_PER_SCREEN/2.0f) continue;

		if (col < 0) 			 continue;
		if (col >= room->width)  continue;
		if (row < 0) 			 continue;
	 	if (row >= room->height) continue;

		int texture_index = m_texture_manager->find_texture_index(tile.texture);

		Vertex v1 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 0), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 0), 0.99f, 0.0f, 0.0f, texture_index};
		Vertex v2 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 1), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 1), 0.99f, 1.0f, 1.0f, texture_index};
		Vertex v3 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 0), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 1), 0.99f, 0.0f, 1.0f, texture_index};
		Vertex v4 = {tile_width * (col - camera_offset.x + TILES_PER_ROW/2.0f + 1), tile_height * (row - camera_offset.y + ROWS_PER_SCREEN/2.0f + 0), 0.99f, 1.0f, 0.0f, texture_index};

		convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);

		VertexBuffer vb;
		IndexBuffer ib;

		buffer_quad(v1, v2, v3, v4, &vb, &ib);

		m_graphics_buffer->vertex_buffers.push_back(vb);
		m_graphics_buffer->index_buffers.push_back(ib);
		m_graphics_buffer->texture_id_buffer.push_back(tile.texture);
		m_graphics_buffer->shaders.push_back(textured_shader);
	}


}

void buffer_title_block() {

}

void buffer_quad_centered_at(float radius, float depth, VertexBuffer * vb, IndexBuffer * ib) {
	Vector2f center = {0.0f, 0.0f};
	buffer_quad_centered_at(center, radius, depth, vb, ib);
}

float ENTITY_MIMINUM_DEPTH = 0.0000f;

inline float max(float a, float b) {
	if(a > b) { return a;}
	return b;
}

void buffer_quad_centered_at(Vector2f center, float radius, float depth, VertexBuffer * vb, IndexBuffer * ib) {
	Vertex v1 = {center.x - radius, center.y + radius*m_window_data->aspect_ratio, max(depth, ENTITY_MIMINUM_DEPTH), 0.0f, 0.0f, 0};
	Vertex v2 = {center.x + radius, center.y - radius*m_window_data->aspect_ratio, max(depth, ENTITY_MIMINUM_DEPTH), 1.0f, 1.0f, 0};
	Vertex v3 = {center.x - radius, center.y - radius*m_window_data->aspect_ratio, max(depth, ENTITY_MIMINUM_DEPTH), 0.0f, 1.0f, 0};
	Vertex v4 = {center.x + radius, center.y + radius*m_window_data->aspect_ratio, max(depth, ENTITY_MIMINUM_DEPTH), 1.0f, 0.0f, 0};

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

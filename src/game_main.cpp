#include <assert.h>

#include "game_main.h"
#include "macros.h"

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

enum Alignement {
    TOP_LEFT, 
    TOP_RIGHT,
    BOTTOM_RIGHT,
    BOTTOM_LEFT
};

struct Font;

struct Entity {
    Entity() {}
    char * name;

    char * texture;

    Vector2f position;
    Vector2f velocity;
    
    bool is_player;
    float size; // relative to tile
};

struct Tile {
    char * texture;
    int local_x;
    int local_y;

    TileType type = DEFAULT;
    int room_target_id;         // Used if type is SWITCH_ROOM
    Vector2 target_tile_coords; // Used if type is SWITCH_ROOM

    Vector2f collision_box;

    bool collision_enabled = false;
};

enum ObjectType {
    TILE,
    ENTITY
};

struct Object {
    ObjectType type;
    union {
        Tile * tile;
        Entity * entity;
    };
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

struct Camera {
    Vector2f size;
    Vector2f offset;
};

// Prototypes
int get_file_size(FILE * file);

void init_textures();

void init_fonts();

void handle_user_input();

void buffer_editor_overlay();

Texture * create_texture(char * name, unsigned char * data, int width, int height, int bytes_per_pixel);
Texture * create_texture(char * name, unsigned char * data, int width, int height);

float buffer_string(char * text, float x, float y, float z,  Font * font, Alignement alignement);
float buffer_string(char * text, float x, float y, float z,  Font * font);

void buffer_player();

void buffer_entities();

void buffer_entity(Entity entity);

void buffer_tiles(Room * room);

void buffer_debug_overlay();

void buffer_editor_tile_overlay(Room * room);

int get_objects_colliding_at(Vector2f position, Object objects[], int max_collisions);

void buffer_colored_quad(Vector2f position, Alignement alignement, float width, float height, float depth, Color4f color);
void buffer_colored_quad(float x, float y, Alignement alignement, float width, float height, float depth, Color4f color);

void convert_top_left_coords_to_centered(Vertex * v1, Vertex * v2, Vertex * v3, Vertex * v4);

// inline float max(float a, float b);

// Constants
const float DEBUG_OVERLAY_Z                = 0.0000000f;
const float DEBUG_OVERLAY_BACKGROUND_Z     = 0.0000001f;
const float EDITOR_OVERLAY_Z               = 0.0000010f;
const float EDITOR_LEFT_PANEL_BACKGROUND_Z = 0.0000009f;
const float EDITOR_LEFT_PANEL_CONTENT_Z    = 0.0000008f;
const float MIN_ENTITY_Z                   = 0.0000100f;

const int MAX_NUMBER_ENTITIES = 1;

// Extern Globals
extern Shader * font_shader;
extern Shader * textured_shader;
extern Shader * colored_shader;

extern Font * my_font;

// Globals
static WindowData * m_window_data;
static Keyboard   * m_keyboard;
static Keyboard   * m_previous_keyboard;

static float square_size = 0.5f;

static Entity player;
static Entity entities[MAX_NUMBER_ENTITIES];

Room * rooms[2];

static Room * current_room;

static Camera main_camera;

static GameMode game_mode;
static bool debug_overlay_enabled = false;

static bool editor_click_menu_open = false;
static bool editor_click_menu_was_open = false;
static Vector2f editor_click_menu_position;

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

        if((i % 7 == 0) || (i % 7 == 2)) {
            tile.texture = "grass1.png";
        } else if ((i % 7 == 1) || (i % 7 == 3) || (i % 7 == 6)) {
            tile.texture = "grass2.png"; 
        } else {
            tile.texture = "grass3.png";
        }

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

void init_game() {

    // Init camera
    main_camera.size.x = 32.0f;
    main_camera.offset.x = 0.0f;
    main_camera.offset.y = 0.0f;

    // Init player
    {
        player.name = "player";
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

        room->tiles[5].texture = "dirt_road.png";
        room->tiles[55].texture = "dirt_road_bottom.png";

        rooms[0] = room;

        // ----------------------------

        room = generate_room("small_room", 28, 14);

        room->outer_tiles[61].type = SWITCH_ROOM;
        room->outer_tiles[61].room_target_id = 0;
        room->outer_tiles[61].target_tile_coords = Vector2(5, 0);

        room->tiles[369].texture = "dirt_road.png";
        room->tiles[341].texture = "dirt_road_top.png";

        rooms[1] = room;
    
    }

    current_room = rooms[0];
    game_mode = GAME;

    // TEST Init trees
    {
        for(int i = 0; i < array_size(entities); i++) {
            char name [64];
            snprintf(name, 64, "tree%d", i);
            entities[i].name = name;
            entities[i].position.x = (int)(8.9*i)%(current_room->width-2) + 5;
            entities[i].position.y = (int)(2.3*i)%(current_room->height-2) + 3;
            entities[i].texture = "tree.png";
            entities[i].is_player = false;
            entities[i].size = 3;
        }
    }
}

void game(WindowData * window_data, Keyboard * keyboard, Keyboard * previous_keyboard) {
    
    m_previous_keyboard = previous_keyboard;

    m_window_data     = window_data;
    m_keyboard        = keyboard;

    main_camera.size.y = main_camera.size.x / m_window_data->aspect_ratio;

    handle_user_input();

    buffer_tiles(current_room);
    
    buffer_entities();  

    buffer_player();

    // Editor overlays
    if(game_mode == EDITOR) {
        buffer_editor_overlay();
    }

    // Debug overlay 
    if(debug_overlay_enabled) {
        buffer_debug_overlay();
    }
}

Object objects[64];
int num_objects = 0;

float EDITOR_MENU_PADDING = 0.004f;

float EDITOR_CLICK_MENU_WIDTH = 0.15f;
float EDITOR_CLICK_MENU_ROW_HEIGHT; // @Temporary Value assigned in buffer_editor_click_menu

enum EditorLeftPanelMode {
    MAIN,
    TILE_INFO,
};

float EDITOR_LEFT_PANEL_WIDTH = 0.18f;
float EDITOR_LEFT_PANEL_PADDING = 0.002f;
float EDITOR_LEFT_PANEL_ROW_HEIGHT; // @Temporary Value assigned in buffer_editor_left_panel

EditorLeftPanelMode editor_left_panel_mode = TILE_INFO;

Object editor_left_panel_displayed_object;

void handle_user_input() {
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

            if(m_keyboard->key_shift) speed = 0.02f;

            float position_delta = speed * m_window_data->frame_time;

            //
            // @Incomplete Need to use and normalize velocity vector
            //
            if(m_keyboard->key_left)  player.position.x -= position_delta;
            if(m_keyboard->key_right) player.position.x += position_delta;
            if(m_keyboard->key_up)    player.position.y -= position_delta;
            if(m_keyboard->key_down)  player.position.y += position_delta;
        }

        // Early collision detection, find current tile
        {
            // @Optimisation We can probably infer the tiles we collide with from the player's x and y coordinates and information about the room's size
            // for(int i = 0; i < current_room->num_tiles; i++) {
            //  Tile tile = current_room->tiles[i];

            //  bool should_compute_collision = tile.collision_enabled;

            //  if(should_compute_collision) {


            //      if((player.position.x < tile.collision_box.x + tile.collision_box.width) && (tile.collision_box.x < player.position.x + player.size) && // X tests
            //         (player.position.y < tile.collision_box.y + tile.collision_box.height) && (tile.collision_box.y < player.position.y + player.size)) {    // Y tests

            //          if(tile.type == SWITCH_ROOM) {
            //              log_print("collision", "Player is on a tile with type SWITCH_ROOM, switching to room %d", tile.room_target_id);
            //              current_room = rooms[tile.room_target_id];

            //              player.position = tile.target_tile_coords;
            //          }

            //      }

            //  }
            // }

            // @Optimisation We can probably infer the tiles we collide with from the player's x and y coordinates and information about the room's size
            for(int i = 0; i < 2 * (current_room->width + current_room->height - 2); i++) {
                Tile tile = current_room->outer_tiles[i];

                bool should_compute_collision = tile.collision_enabled;

                if(should_compute_collision) {


                    if((player.position.x < tile.collision_box.x + tile.collision_box.width) && (tile.collision_box.x < player.position.x + player.size) && // X tests
                       (player.position.y < tile.collision_box.y + tile.collision_box.height) && (tile.collision_box.y < player.position.y + player.size)) {    // Y tests

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
            main_camera.offset = player.position;

            if(current_room->width <= main_camera.size.x) { // If the room is too small for the the camera, center it.
                main_camera.offset.x = current_room->width/2.0f;
            }
            else if(main_camera.offset.x < main_camera.size.x/2.0f) { // If the camera touches an edge of the room, block it
                main_camera.offset.x = main_camera.size.x/2.0f;
            }
            else if(main_camera.offset.x > current_room->width - main_camera.size.x/2.0f) { // See above
                main_camera.offset.x = current_room->width - main_camera.size.x/2.0f;
            }

            if(current_room->height <= main_camera.size.y) { // If the room is too small for the the camera, center it.
                main_camera.offset.y = current_room->height/2.0f;
            }
            else if(main_camera.offset.y < main_camera.size.y/2.0f) { // If the camera touches an edge of the room, block it
                main_camera.offset.y = main_camera.size.y/2.0f;
            }
            else if(main_camera.offset.y > current_room->height - main_camera.size.y/2.0f) { // See above
                main_camera.offset.y = current_room->height - main_camera.size.y/2.0f;
            }   
        } else if (game_mode == EDITOR) {
            //  @Refactor Same code as player movement

            float speed = 0.006f;

            if(m_keyboard->key_shift) speed = 0.02f;

            float position_delta = speed * m_window_data->frame_time;

            //
            // @Incomplete Need to use and normalize velocity vector
            //
            if(m_keyboard->key_left)  main_camera.offset.x -= position_delta;
            if(m_keyboard->key_right) main_camera.offset.x += position_delta;
            if(m_keyboard->key_up)    main_camera.offset.y -= position_delta;
            if(m_keyboard->key_down)  main_camera.offset.y += position_delta;
        }
        // log_print("[game_camera]", "Camera offset is (%f, %f)", camera_offset.x, camera_offset.y);
    }

    // Editor controls
    {
        if(game_mode == EDITOR) {
            if(!m_keyboard->mouse_left && m_previous_keyboard->mouse_left) { // Mouse left up
                if(editor_click_menu_was_open) { // This mouse click should close the menu.
                    editor_click_menu_was_open = false;
                } else {
                    // log_print("mouse_testing", "Mouse left pressed at (%0.6f, %0.6f) and released at (%0.6f, %0.6f)", m_keyboard->mouse_left_pressed_position.x, m_keyboard->mouse_left_pressed_position.y, m_keyboard->mouse_position.x, m_keyboard->mouse_position.y);
                    Vector2f game_space_position;
                    game_space_position.x = main_camera.size.x * (m_keyboard->mouse_left_pressed_position.x - 0.5f) + main_camera.offset.x;
                    game_space_position.y = main_camera.size.y * (m_keyboard->mouse_left_pressed_position.y - 0.5f) + main_camera.offset.y;

                    num_objects = get_objects_colliding_at(game_space_position, objects, 64);

                    editor_click_menu_position = m_keyboard->mouse_position;

                    editor_click_menu_open = true;
                }
            }

            if(m_keyboard->mouse_left && !m_previous_keyboard->mouse_left) { // Mouse left down
                Vector2f position = m_keyboard->mouse_position;

                if(editor_click_menu_open) {
                    if((position.x <= editor_click_menu_position.x + EDITOR_CLICK_MENU_WIDTH) 
                        && (position.x >= editor_click_menu_position.x) 
                        && (position.y <= editor_click_menu_position.y + EDITOR_CLICK_MENU_ROW_HEIGHT * num_objects) 
                        && (position.y >= editor_click_menu_position.y)) {

                        int element_number = 0;
                        float element_y = (position.y - editor_click_menu_position.y);
                        
                        while(true) {
                            element_y -= EDITOR_CLICK_MENU_ROW_HEIGHT;

                            if(element_y > 0) {
                                element_number++;
                            } else {
                                break;
                            }   
                        }
                        
                        // log_print("editor_click_menu", "The element no. %d of the click menu was clicked", element_number);

                        // if(strcmp(objects[element_number].tile->texture, "grass.png") == 0) {
                        //     objects[element_number].tile->texture = "dirt_road.png"; // = m_texture_manager->find_texture("dirt_road.png");
                        // } else {
                        //     objects[element_number].tile->texture = "grass.png"; // = m_texture_manager->find_texture("dirt_road.png");
                        // }

                        editor_left_panel_displayed_object = objects[element_number];
                    } 

                    editor_click_menu_open = false;
                    editor_click_menu_was_open = true;
                }
            }
        }
    }
}

int get_objects_colliding_at(Vector2f position, Object objects[], int max_collisions) {

    int num_objects = 0;

    // @Optimisation We can probably infer the tiles we collide with from the player's x and y coordinates and information about the room's size
    for(int i = 0; i < current_room->num_tiles; i++) {
        Tile tile = current_room->tiles[i];

        if((position.x <= tile.local_x + 1) && (position.x >= tile.local_x) &&
           (position.y <= tile.local_y + 1) && (position.y >= tile.local_y)) {
            objects[num_objects].type = TILE;
            objects[num_objects].tile = &current_room->tiles[i];

            num_objects++;

            if(num_objects == max_collisions) {
                break;
            }
        }
    }

    return num_objects;
}

void buffer_editor_left_panel() {

    EDITOR_LEFT_PANEL_ROW_HEIGHT = EDITOR_LEFT_PANEL_PADDING * 2 * m_window_data->aspect_ratio + 12.0f / m_window_data->height; // @Temporary Current distance from baseline to top of capital letter is 12px

    // Background
    {
        Color4f color = Color4f(0.0f, 0.0f, 0.0f, 0.8f);
        buffer_colored_quad(0.0f, 0.0f, TOP_LEFT, EDITOR_LEFT_PANEL_WIDTH, 1.0f, EDITOR_LEFT_PANEL_BACKGROUND_Z, color);
    }

    if(editor_left_panel_mode == TILE_INFO) {
        if(editor_left_panel_displayed_object.tile != NULL) {

            float y = 0.0f;

            // Texture display
            {
                float texture_display_size = 0.7f * EDITOR_LEFT_PANEL_WIDTH;
                float texture_side_padding = 0.15f * EDITOR_LEFT_PANEL_WIDTH;
                float texture_top_padding = texture_side_padding / 2.0f;

                float aspect_ratio = m_window_data->aspect_ratio;

                Vertex v1 = {texture_side_padding                       , y + texture_top_padding                                      , EDITOR_LEFT_PANEL_CONTENT_Z, 0.0f, 0.0f};
                Vertex v2 = {texture_side_padding + texture_display_size, y + texture_top_padding + texture_display_size * aspect_ratio, EDITOR_LEFT_PANEL_CONTENT_Z, 1.0f, 1.0f};
                Vertex v3 = {texture_side_padding                       , y + texture_top_padding + texture_display_size * aspect_ratio, EDITOR_LEFT_PANEL_CONTENT_Z, 0.0f, 1.0f};
                Vertex v4 = {texture_side_padding + texture_display_size, y + texture_top_padding                                      , EDITOR_LEFT_PANEL_CONTENT_Z, 1.0f, 0.0f};

                convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);

                set_texture(editor_left_panel_displayed_object.tile->texture);
                set_shader(textured_shader);

                start_buffer();

                add_to_buffer(v1);
                add_to_buffer(v2);
                add_to_buffer(v3);
                add_to_buffer(v4);

                end_buffer();

                y += texture_display_size * aspect_ratio + 2 * texture_top_padding;
            }

            y += EDITOR_LEFT_PANEL_PADDING;

            buffer_string("Texture :", EDITOR_LEFT_PANEL_PADDING, y, EDITOR_LEFT_PANEL_CONTENT_Z, my_font, TOP_LEFT);

            y += EDITOR_LEFT_PANEL_ROW_HEIGHT;

            buffer_string(editor_left_panel_displayed_object.tile->texture, EDITOR_LEFT_PANEL_WIDTH - EDITOR_LEFT_PANEL_PADDING, y, EDITOR_LEFT_PANEL_CONTENT_Z, my_font, TOP_RIGHT);

        }
    }

}

void buffer_editor_click_menu() {
    EDITOR_CLICK_MENU_ROW_HEIGHT = EDITOR_MENU_PADDING * 2 * m_window_data->aspect_ratio + 12.0f / m_window_data->height; // @Temporary Current distance from baseline to top of capital letter is 12px

    float x = editor_click_menu_position.x;
    float y = editor_click_menu_position.y;


    for(int i = 0; i < num_objects; i++) {
        // Buffer menu background
        {
            Color4f color;
            if(i % 2 == 0) {
                color = Color4f(0.1f, 0.1f, 0.1f, 0.8f);
            } else {
                color = Color4f(0.3f, 0.3f, 0.3f, 0.7f);
            }

            buffer_colored_quad(x, y, TOP_LEFT, EDITOR_CLICK_MENU_WIDTH, EDITOR_CLICK_MENU_ROW_HEIGHT, DEBUG_OVERLAY_BACKGROUND_Z, color);
        }

        // Buffer strings
        {
            if(objects[i].type == TILE) {
                // log_print("editor_mouse_collision", "Colliding with object of type TILE at (%d, %d)", objects[i].tile->local_x, objects[i].tile->local_y);
                char tile_name[64];
                snprintf(tile_name, 64, "Tile%d_%d", objects[i].tile->local_x, objects[i].tile->local_y);

                buffer_string(tile_name, x + EDITOR_MENU_PADDING, y + EDITOR_MENU_PADDING * m_window_data->aspect_ratio, DEBUG_OVERLAY_Z, my_font, TOP_LEFT);

            }
            else if(objects[i].type == ENTITY) {
                // log_print("editor_mouse_collision", "Colliding with object of type ENTITY named %s at (%0.3f, %0.3f)", objects[i].entity->name, objects[i].entity->position.x, objects[i].entity->position.y);
                buffer_string(objects[i].entity->name, x, y, DEBUG_OVERLAY_Z, my_font, TOP_LEFT);
            }
        }

        y += EDITOR_CLICK_MENU_ROW_HEIGHT;
    }
}

void buffer_editor_overlay() {

    buffer_editor_tile_overlay(current_room);
    buffer_editor_left_panel();

    if(editor_click_menu_open) {
        buffer_editor_click_menu();
    }
}

int frame_time_print_counter = 0;

const int FRAME_TIME_UPDATE_DELAY = 15; // in frames

float displayed_frame_time = 0.0f;
float summed_frame_rate = 0.0f;

void buffer_debug_overlay() {
    float DEBUG_OVERLAY_PADDING = 0.004f;

    float DEBUG_OVERLAY_WIDTH = 0.15f;
    float DEBUG_OVERLAY_ROW_HEIGHT = DEBUG_OVERLAY_PADDING * 2 * m_window_data->aspect_ratio + 12.0f / m_window_data->height; // @Temporary Current distance from baseline to top of capital letter is 12px

    float DEBUG_OVERLAY_NUM_ROWS = 4;

    // Buffer debug overlay background
    {
        for(int i = 0; i < DEBUG_OVERLAY_NUM_ROWS; i++) {
            Vector2f position;

            position.x = 1.0f - DEBUG_OVERLAY_WIDTH;
            position.y = i * DEBUG_OVERLAY_ROW_HEIGHT;

            Color4f color;

            if(i % 2 == 0) {
                color = Color4f(0.1f, 0.1f, 0.1f, 0.8f);
            } else {
                color = Color4f(0.3f, 0.3f, 0.3f, 0.7f);
            }

            buffer_colored_quad(position, TOP_LEFT, DEBUG_OVERLAY_WIDTH, DEBUG_OVERLAY_ROW_HEIGHT, DEBUG_OVERLAY_BACKGROUND_Z, color);
        }

    }

    // Compute frame time average
    if(frame_time_print_counter == 0) {
        displayed_frame_time = summed_frame_rate / FRAME_TIME_UPDATE_DELAY;

        // log_print("perf_counter", "Average of %d frames is %f", FRAME_TIME_UPDATE_DELAY, displayed_frame_time);

        summed_frame_rate = 0.0f;
        frame_time_print_counter = FRAME_TIME_UPDATE_DELAY;
    }

    summed_frame_rate += m_window_data->frame_time;
    frame_time_print_counter--;

    float left_x = 1.0f - DEBUG_OVERLAY_WIDTH + DEBUG_OVERLAY_PADDING;
    float right_x = 1.0f - DEBUG_OVERLAY_PADDING;
    float y = 0.0f;
    // Buffer Frame time (text must always be bufferd last if it has AA / transparency);
    {
        char buffer[64];

        snprintf(buffer, sizeof(buffer), "%.3f", displayed_frame_time);

        buffer_string("Frame Time :", left_x, y + DEBUG_OVERLAY_PADDING * m_window_data->aspect_ratio, DEBUG_OVERLAY_Z, my_font, TOP_LEFT); 

        y += DEBUG_OVERLAY_ROW_HEIGHT;

        buffer_string(buffer, right_x, y  + DEBUG_OVERLAY_PADDING * m_window_data->aspect_ratio, DEBUG_OVERLAY_Z, my_font, TOP_RIGHT);

        y += DEBUG_OVERLAY_ROW_HEIGHT;
    }

    // Buffer Current world (text must always be bufferd last if it has AA / transparency);
    {
        char buffer[64];

        snprintf(buffer, sizeof(buffer), "%s", current_room->name);

        buffer_string("Current World :", left_x, y + DEBUG_OVERLAY_PADDING * m_window_data->aspect_ratio, DEBUG_OVERLAY_Z, my_font, TOP_LEFT); 

        y += DEBUG_OVERLAY_ROW_HEIGHT;

        buffer_string(buffer, right_x, y  + DEBUG_OVERLAY_PADDING * m_window_data->aspect_ratio, DEBUG_OVERLAY_Z, my_font, TOP_RIGHT);

        y += DEBUG_OVERLAY_ROW_HEIGHT;
    }
}

void do_buffer_editor_tile_overlay(Tile * tiles, int num_tiles) {
    for(int tile_index = 0; tile_index < num_tiles; tile_index++) {
        Tile tile = tiles[tile_index];
        int col = tile.local_x;
        int row = tile.local_y;

        // Don't buffer out of screen tiles
        if (col + 1 < main_camera.offset.x - main_camera.size.x * 0.5f) continue;
        if (col     > main_camera.offset.x + main_camera.size.x * 0.5f) continue;
        if (row + 1 < main_camera.offset.y - main_camera.size.y * 0.5f) continue;
        if (row     > main_camera.offset.y + main_camera.size.y * 0.5f) continue;

        Color4f color = Color4f(1.0f, 1.0f, 1.0f, 0.0f); // transparent, for now @Temporary

        if(tile.type == SWITCH_ROOM) {
            color = Color4f(1.0f, 0.0f, 0.0f, 0.5f);
        } else if(tile.type == BLOCK) {
            color = Color4f(0.0f, 1.0f, 1.0f, 0.5f);
        }

        Vector2f tile_size;

        tile_size.x = 1.0f/main_camera.size.x;
        tile_size.y = 1.0f/main_camera.size.y; // @Optimization, use multiplication here ?

        Vector2f tile_offset;

        tile_offset.x = tile_size.x * (col - main_camera.offset.x) + 0.5f;
        tile_offset.y = tile_size.y * (row - main_camera.offset.y) + 0.5f;

        Vertex v1 = {tile_offset.x              , tile_offset.y              , EDITOR_OVERLAY_Z, color};
        Vertex v2 = {tile_offset.x + tile_size.x, tile_offset.y + tile_size.y, EDITOR_OVERLAY_Z, color};
        Vertex v3 = {tile_offset.x              , tile_offset.y + tile_size.y, EDITOR_OVERLAY_Z, color};
        Vertex v4 = {tile_offset.x + tile_size.x, tile_offset.y              , EDITOR_OVERLAY_Z, color};

        convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);

        set_shader(colored_shader);

        start_buffer();

        add_to_buffer(v1);
        add_to_buffer(v2);
        add_to_buffer(v3);
        add_to_buffer(v4);

        end_buffer();
    }
}

void buffer_editor_tile_overlay(Room * room) {
    do_buffer_editor_tile_overlay(room->tiles, room->num_tiles);
    do_buffer_editor_tile_overlay(room->outer_tiles, room->num_outer_tiles);
}

void buffer_entities() {
    for(int i = 0; i< array_size(entities); i++) {
        buffer_entity(entities[i]);
    }
}

void buffer_player() {
    buffer_entity(player);
}

void buffer_entity(Entity entity) {
        // Don't buffer is the entity is out of screen
        if (entity.position.x + entity.size < main_camera.offset.x - main_camera.size.x * 0.5f) return;
        if (entity.position.x               > main_camera.offset.x + main_camera.size.x * 0.5f) return;
        if (entity.position.y + entity.size < main_camera.offset.y - main_camera.size.y * 0.5f) return;
        if (entity.position.y               > main_camera.offset.y + main_camera.size.y * 0.5f) return;

        Vector2f tile_size;

        tile_size.x = 1.0f/main_camera.size.x;
        tile_size.y = 1.0f/main_camera.size.y; // @Optimization, use multiplication here ?

        Vector2f screen_pos;

        screen_pos.x = tile_size.x * (entity.position.x - main_camera.offset.x) + 0.5f;
        screen_pos.y = tile_size.y * (entity.position.y - main_camera.offset.y) + 0.5f;

        Vector2f screen_size;

        screen_size.x = tile_size.x * entity.size;
        screen_size.y = tile_size.y * entity.size;

        float z = (1.0f - (entity.position.y + entity.size)/current_room->height) * 0.99f;

        Vertex v1 = {screen_pos.x                , screen_pos.y                , z, 0.0f, 0.0f};
        Vertex v2 = {screen_pos.x + screen_size.x, screen_pos.y + screen_size.y, z, 1.0f, 1.0f};
        Vertex v3 = {screen_pos.x                , screen_pos.y + screen_size.y, z, 0.0f, 1.0f};
        Vertex v4 = {screen_pos.x + screen_size.x, screen_pos.y                , z, 1.0f, 0.0f};

        convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);

        set_texture(entity.texture);
        set_shader(textured_shader);

        start_buffer();

        add_to_buffer(v1);
        add_to_buffer(v2);
        add_to_buffer(v3);
        add_to_buffer(v4);

        end_buffer();
}

void buffer_tiles(Room * room) {
    for(int tile_index = 0; tile_index < room->num_tiles; tile_index++) {
        Tile tile = room->tiles[tile_index];
        int col = tile.local_x;
        int row = tile.local_y;

        // Don't buffer out of screen tiles
        if (col + 1 < main_camera.offset.x - main_camera.size.x * 0.5f) continue;
        if (col     > main_camera.offset.x + main_camera.size.x * 0.5f) continue;
        if (row + 1 < main_camera.offset.y - main_camera.size.y * 0.5f) continue;
        if (row     > main_camera.offset.y + main_camera.size.y * 0.5f) continue;

        Vector2f tile_size;

        tile_size.x = 1.0f/main_camera.size.x;
        tile_size.y = 1.0f/main_camera.size.y; // @Optimization, use multiplication here ?

        Vector2f tile_offset;

        tile_offset.x = tile_size.x * (col - main_camera.offset.x) + 0.5f;
        tile_offset.y = tile_size.y * (row - main_camera.offset.y) + 0.5f;
    
        Vertex v1 = {tile_offset.x              , tile_offset.y              , 0.99f, 0.0f, 0.0f};
        Vertex v2 = {tile_offset.x + tile_size.x, tile_offset.y + tile_size.y, 0.99f, 1.0f, 1.0f};
        Vertex v3 = {tile_offset.x              , tile_offset.y + tile_size.y, 0.99f, 0.0f, 1.0f};
        Vertex v4 = {tile_offset.x + tile_size.x, tile_offset.y              , 0.99f, 1.0f, 0.0f};

        convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);

        set_texture(tile.texture);
        set_shader(textured_shader);

        start_buffer();

        add_to_buffer(v1);
        add_to_buffer(v2);
        add_to_buffer(v3);
        add_to_buffer(v4);

        end_buffer();
    }
}

float buffer_string(char * text, float x, float y, float z,  Font * font) {
    return buffer_string(text, x, y, z, font, TOP_LEFT);
}

float buffer_string(char * text, float x, float y, float z,  Font * font, Alignement alignement) {
    float pixel_x = x * m_window_data->width;
    float pixel_y = y * m_window_data->height;

    
    float text_width = 0.0f;
    float text_height = 0.0f;

    // Compute height
    if(alignement == TOP_RIGHT || alignement == TOP_LEFT) {
        float zero_x = 0.0f;
        float zero_y = 0.0f;

        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(font->char_data, font->texture->width, font->texture->height, 'A' - 32, &zero_x, &zero_y, &q, 1);
        
        text_height = (float) ((int)(-q.y0 + 0.5f)) / m_window_data->height;

        pixel_y += text_height * m_window_data->height;
    }

    // Compute width
    {
        char * original_text = text;
        float original_pixel_x = pixel_x;
        float original_pixel_y = pixel_y;

        // First pass to determine width and height
        while (*text) {
            // Make sure it's an ascii character
            if (*text >= 32 && *text < 128) {
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(font->char_data, font->texture->width, font->texture->height, *text - 32, &pixel_x, &pixel_y, &q, 1);
            }
            ++text;
        }

        text_width = (float) ((int)(pixel_x - original_pixel_x + 0.5f)) / m_window_data->width;

        text = original_text;
        pixel_x = original_pixel_x;
        pixel_y = original_pixel_y;
    }

    set_texture(font->texture->name);
    set_shader(font_shader);

    start_buffer();

    while(*text) {
        // Make sure it's an ascii character
        if (*text >= 32 && *text < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font->char_data, font->texture->width, font->texture->height, *text-32, &pixel_x, &pixel_y, &q, 1);
        
            Vertex v1 = {q.x0/m_window_data->width, q.y0/m_window_data->height, z, q.s0, q.t0};
            Vertex v2 = {q.x1/m_window_data->width, q.y1/m_window_data->height, z, q.s1, q.t1};
            Vertex v3 = {q.x0/m_window_data->width, q.y1/m_window_data->height, z, q.s0, q.t1};
            Vertex v4 = {q.x1/m_window_data->width, q.y0/m_window_data->height, z, q.s1, q.t0};

            if(alignement == TOP_RIGHT) {
                v1.position.x -= text_width;
                v2.position.x -= text_width;
                v3.position.x -= text_width;
                v4.position.x -= text_width;
            }

            convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);

            add_to_buffer(v1);
            add_to_buffer(v2);
            add_to_buffer(v3);
            add_to_buffer(v4);
        }
        ++text;
    }

    end_buffer();

    return text_width;
}

void buffer_colored_quad(Vector2f position, Alignement alignement, float width, float height, float depth, Color4f color) {
    buffer_colored_quad(position.x, position.y, alignement, width, height, depth, color);
}

void buffer_colored_quad(float x, float y, Alignement alignement, float width, float height, float depth, Color4f color) {

    Vertex v1;
    Vertex v2;
    Vertex v3;
    Vertex v4;

    if(alignement == TOP_LEFT) {
        v1 = {x        , y         , depth, color};
        v2 = {x + width, y + height, depth, color};
        v3 = {x        , y + height, depth, color};
        v4 = {x + width, y         , depth, color};
    }
    else if(alignement == TOP_RIGHT) {
        v1 = {x - width, y         , depth, color};
        v2 = {x        , y + height, depth, color};
        v3 = {x - width, y + height, depth, color};
        v4 = {x        , y         , depth, color};
    }
    convert_top_left_coords_to_centered(&v1, &v2, &v3, &v4);

    set_shader(colored_shader);

    start_buffer();

    add_to_buffer(v1);
    add_to_buffer(v2);
    add_to_buffer(v3);
    add_to_buffer(v4);

    end_buffer(); 
}

float ENTITY_MIMINUM_DEPTH = 0.0000f;

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

// inline float max(float a, float b) {
//     if(a > b) return a;
//     return b;
// }

#include <assert.h>

#include "game_main.h"

#include "os/layer.h"
#include "os/win32/hotloader.h" // @Temporary, I'd like to move the hotloader out of win32.

#include "macros.h"

#include "renderer.h"

#include "texture_manager.h"
#include "font_manager.h"
#include "shader_manager.h"
#include "room_manager.h"

// Structs
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

enum GameMode {
    TITLE_SCREEN,
    GAME,
    EDITOR
};

enum Alignement {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_RIGHT,
    BOTTOM_LEFT
};

struct Entity {
    Entity() {}
    char * name;

    char * texture;

    Vector2f position;
    Vector2f velocity;

    bool is_player;
    float size; // relative to tile
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

struct Camera {
    Vector2f size;
    Vector2f offset;
};

// Prototypes
void init_game();

void init_shaders();

void game();

void handle_user_input();

void buffer_editor_overlay();

float buffer_string(char * text, float x, float y, float z,  Font * font, Alignement alignement = BOTTOM_LEFT);

void buffer_player();

void buffer_entities();

void buffer_entity(Entity entity);

void buffer_tiles(Array<Tile> tile);

void buffer_debug_overlay();

void buffer_editor_blocks_overlay(Room * room);
// void buffer_editor_tile_overlay(Room * room);

void get_objects_colliding_at(Vector2f position, Array<Object> * _objects); // @Cleanup name collision here.

void buffer_textured_quad(float x, float y, Alignement alignement, float width, float height, float depth, char * texture);

void buffer_colored_quad(Vector2f position, Alignement alignement, float width, float height, float depth, Color4f color);
void buffer_colored_quad(float x, float y, Alignement alignement, float width, float height, float depth, Color4f color);
void buffer_colored_quad(Quad quad, float depth, Color4f color);

void convert_top_left_coords_to_centered(Vector3f * v1, Vector3f * v2, Vector3f * v3, Vector3f * v4);

// Constants
const float DEBUG_OVERLAY_Z                = 0.9999999f;
const float DEBUG_OVERLAY_BACKGROUND_Z     = 0.9999998f;

const float EDITOR_LEFT_PANEL_CONTENT_Z    = 0.9999989f;
const float EDITOR_LEFT_PANEL_BACKGROUND_Z = 0.9999988f;
const float EDITOR_OVERLAY_Z               = 0.9999987f;

const float TILES_Z                        = 0.0000001f;
const float RANGE_ENTITY_Z                 = 0.9999900f;
const float MIN_ENTITY_Z                   = 0.0000002f;

const int TARGET_FPS = 60;

const int MAX_NUMBER_ENTITIES = 1;

// Globals
static Shader * font_shader;
static Shader * textured_shader;
static Shader * colored_shader;


static WindowData window_data;

static Keyboard keyboard;
static Keyboard previous_keyboard;

static float last_time = 0.0f;

static Entity player;
static Entity entities[MAX_NUMBER_ENTITIES];

static Room * rooms[2];

static Room * current_room;

static Camera main_camera;

static GameMode game_mode;
static bool debug_overlay_enabled = false;

static bool editor_click_menu_open = false;
static bool editor_click_menu_was_open = false;
static Vector2f editor_click_menu_position;

static TextureManager texture_manager;
static FontManager font_manager;
static ShaderManager shader_manager;
static RoomManager room_manager;

static Array<AssetManager *> managers;

static Font * my_font;

static void init_shaders() {
    font_shader     = shader_manager.table.find("font.shader");
    textured_shader = shader_manager.table.find("textured.shader");
    colored_shader  = shader_manager.table.find("colored.shader");
}

Room * generate_room(char * name, int width, int height) {
    Room * room = (Room *) malloc(sizeof(Room));

    // @Cleanup.
    Room default;
    *room = default;

    room->name   = name;
    room->dimensions.width  = width;
    room->dimensions.height = height;

    for(int i = 0; i < room->dimensions.width * room->dimensions.height; i++) {
        Tile tile;

        if((i % 7 == 0) || (i % 7 == 2)) {
            tile.texture = "grass1.png";
        } else if ((i % 7 == 1) || (i % 7 == 3) || (i % 7 == 6)) {
            tile.texture = "grass2.png";
        } else {
            tile.texture = "grass3.png";
        }

        tile.position.x = i % room->dimensions.width;
        tile.position.y = i / room->dimensions.width;

        room->tiles.add(tile);
    }

    return room;
}

void init_game() {

    // Init camera
    {
        main_camera.size.y  = 18.0f;
        main_camera.offset.x = 0.0f;
        main_camera.offset.y = 0.0f;
    }

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

        room->tiles.data[5].texture = "dirt_road.png";
        room->tiles.data[55].texture = "dirt_road_bottom.png";

        rooms[0] = room;

        // ----------------------------

        room = generate_room("small_room", 28, 14);

        room->tiles.data[369].texture = "dirt_road.png";
        room->tiles.data[341].texture = "dirt_road_top.png";

        rooms[1] = room;

    }

    // current_room = rooms[0];
    current_room = room_manager.table.find("main-room.room");
    game_mode = GAME;

    // TEST Init trees
    {
        for(int i = 0; i < array_size(entities); i++) {
            char name [64];
            snprintf(name, 64, "tree%d", i);
            entities[i].name = name;
            entities[i].position.x = (int)(8.9*i)%(current_room->dimensions.width-2) + 5;
            entities[i].position.y = (int)(2.3*i)%(current_room->dimensions.height-2) + 3;
            entities[i].texture = "tree.png";
            entities[i].is_player = false;
            entities[i].size = 3;
        }
    }
}

// :CoordConversion Small comment about why I needed +1s, I believe it is
// because I now need to take into account the size of the entity/size.

void game() {
    perf_monitor();

    main_camera.size.x = main_camera.size.y * window_data.aspect_ratio;

    handle_user_input();

    buffer_tiles(current_room->tiles);

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

Array<Object> objects;

float EDITOR_MENU_PADDING = 0.004f;

float EDITOR_CLICK_MENU_WIDTH = 0.15f;
float EDITOR_CLICK_MENU_ROW_HEIGHT; // @Temporary Value assigned in buffer_editor_click_menu

enum EditorLeftPanelMode {
    MAIN,
    TILE_INFO,
};

float EDITOR_LEFT_PANEL_WIDTH = 0.18f;
float EDITOR_LEFT_PANEL_PADDING = 0.004f;
float EDITOR_LEFT_PANEL_ROW_HEIGHT; // @Temporary Value assigned in buffer_editor_left_panel

EditorLeftPanelMode editor_left_panel_mode = TILE_INFO;

Object editor_left_panel_displayed_object;

void handle_user_input() {
    if(keyboard.key_F1 && !previous_keyboard.key_F1) {
        if(game_mode == GAME) {
            game_mode = EDITOR;
        }
        else if(game_mode == EDITOR) {
            game_mode = GAME;
        }
    }

    if(keyboard.key_F2 && !previous_keyboard.key_F2) {
        debug_overlay_enabled = !debug_overlay_enabled;
    }

    if(keyboard.key_F3 && !previous_keyboard.key_F3) {
        window_data.locked_fps = !window_data.locked_fps;
    }

    if(game_mode == GAME) {
        // Handle player movement
        {
            float speed = 6.0f;

            if(keyboard.key_shift) speed = 20.0f;

            float position_delta = speed * window_data.current_dt;

            //
            // @Incomplete Need to use and normalize velocity vector
            //
            if(keyboard.key_left)  player.position.x -= position_delta;
            if(keyboard.key_right) player.position.x += position_delta;
            if(keyboard.key_up)    player.position.y -= position_delta;
            if(keyboard.key_down)  player.position.y += position_delta;
        }

        // // Early collision detection, find current tile
        // {
        //     // @Optimisation We can probably infer the tiles we collide with from the player's x and y coordinates and information about the room's size
        //     for(int i = 0; i < current_room->num_outer_tiles; i++) {
        //         Tile tile = current_room->outer_tiles[i];

        //         bool should_compute_collision = tile.collision_enabled;

        //         if(should_compute_collision) {
        //             if((player.position.x < tile.local_x + tile.collision_box.width) && (tile.local_x < player.position.x + player.size) && // X tests
        //                (player.position.y < tile.local_y + tile.collision_box.height) && (tile.local_y < player.position.y + player.size)) {    // Y tests

        //                 if(tile.type == SWITCH_ROOM) {
        //                     // log_print("collision", "Player is on a tile with type SWITCH_ROOM, switching to room %d", tile.room_target_id);
        //                     current_room = rooms[tile.room_target_id];

        //                     player.position.x = tile.target_tile_coords.x + (player.position.x - tile.local_x);
        //                     player.position.y = tile.target_tile_coords.y + (player.position.y - tile.local_y);

        //                     break;
        //                 }
        //             }
        //         }
        //     }
        // }

        // Clamp player position
        {
            if(player.position.x < 0)  {
                player.position.x = 0;
            }

            if(player.position.x >  current_room->dimensions.width - 1)  {
                player.position.x = current_room->dimensions.width - 1;
            }

            if(player.position.y < 0) {
                player.position.y = 0;
            }

            if(player.position.y >  current_room->dimensions.height - 1)  {
                player.position.y = current_room->dimensions.height - 1;
            }
        }
    }

    // Camera logic
    {
        if(game_mode == GAME) {
            main_camera.offset = player.position;

            if(current_room->dimensions.width <= main_camera.size.x) { // If the room is too small for the the camera, center it.
                main_camera.offset.x = current_room->dimensions.width/2.0f;
            }
            else if(main_camera.offset.x < main_camera.size.x/2.0f) { // If the camera touches an edge of the room, block it
                main_camera.offset.x = main_camera.size.x/2.0f;
            }
            else if(main_camera.offset.x > current_room->dimensions.width - main_camera.size.x/2.0f) { // See above
                main_camera.offset.x = current_room->dimensions.width - main_camera.size.x/2.0f;
            }

            if(current_room->dimensions.height <= main_camera.size.y) { // If the room is too small for the the camera, center it.
                main_camera.offset.y = current_room->dimensions.height/2.0f;
            }
            else if(main_camera.offset.y < main_camera.size.y/2.0f) { // If the camera touches an edge of the room, block it
                main_camera.offset.y = main_camera.size.y/2.0f;
            }
            else if(main_camera.offset.y > current_room->dimensions.height - main_camera.size.y/2.0f) { // See above
                main_camera.offset.y = current_room->dimensions.height - main_camera.size.y/2.0f;
            }
        } else if (game_mode == EDITOR) {
            //  @Refactor Same code as player movement

            float speed = 6.0f;

            if(keyboard.key_shift) speed = 20.0f;

            float position_delta = speed * window_data.current_dt
            ;

            //
            // @Incomplete Need to use and normalize velocity vector
            //
            if(keyboard.key_left)  main_camera.offset.x -= position_delta;
            if(keyboard.key_right) main_camera.offset.x += position_delta;
            if(keyboard.key_up)    main_camera.offset.y -= position_delta;
            if(keyboard.key_down)  main_camera.offset.y += position_delta;
        }
        // log_print("[game_camera]", "Camera offset is (%f, %f)", camera_offset.x, camera_offset.y);
    }

    // Editor controls
    {
        if(game_mode == EDITOR) {
            if(!keyboard.mouse_left && previous_keyboard.mouse_left) { // Mouse left up
                if(editor_click_menu_was_open) { // This mouse click should close the menu.
                    editor_click_menu_was_open = false;
                } else {
                    // log_print("mouse_testing", "Mouse left pressed at (%0.6f, %0.6f) and released at (%0.6f, %0.6f)", keyboard.mouse_left_pressed_position.x, keyboard.mouse_left_pressed_position.y, keyboard.mouse_position.x, keyboard.mouse_position.y);
                    Vector2f game_space_position;
                    game_space_position.x = main_camera.size.x * (keyboard.mouse_left_pressed_position.x - 0.5f) + main_camera.offset.x;
                    game_space_position.y = main_camera.size.y * (keyboard.mouse_left_pressed_position.y - 0.5f) + main_camera.offset.y;


                    get_objects_colliding_at(game_space_position, &objects);

                    editor_click_menu_position = keyboard.mouse_position;

                    editor_click_menu_open = true;
                }
            }

            if(keyboard.mouse_left && !previous_keyboard.mouse_left) { // Mouse left down
                Vector2f position = keyboard.mouse_position;

                if(editor_click_menu_open) {
                    if((position.x <= editor_click_menu_position.x + EDITOR_CLICK_MENU_WIDTH)
                    && (position.x >= editor_click_menu_position.x)
                    && (position.y >= editor_click_menu_position.y - EDITOR_CLICK_MENU_ROW_HEIGHT * objects.count)
                    && (position.y <= editor_click_menu_position.y)) {

                        int element_number = 0;
                        float element_y = editor_click_menu_position.y - position.y;

                        while(true) {
                            element_y -= EDITOR_CLICK_MENU_ROW_HEIGHT;

                            if(element_y < 0) break;

                            element_number++;
                        }

                        // log_print("editor_click_menu", "The element no. %d of the click menu was clicked", element_number);

                        // if(strcmp(objects[element_number].tile->texture, "grass.png") == 0) {
                        //     objects[element_number].tile->texture = "dirt_road.png"; // = m_texture_manager->find_texture("dirt_road.png");
                        // } else {
                        //     objects[element_number].tile->texture = "grass.png"; // = m_texture_manager->find_texture("dirt_road.png");
                        // }

                        editor_left_panel_displayed_object = objects.data[element_number];
                    }

                    objects.reset();
                    editor_click_menu_open     = false;
                    editor_click_menu_was_open = true;
                }
            }
        }
    }
}

void get_objects_colliding_at(Vector2f point, Array<Object> * _objects) {
    // @Optimisation We can probably infer the tiles we collide with from the player's x and y coordinates and information about the room's size
    for(int i = 0; i < current_room->tiles.count; i++) {
        Tile * tile = &current_room->tiles.data[i];
        Vector2 position = tile->position;

        if((point.x <= position.x + 1) && (point.x >= position.x) &&
           (point.y <= position.y + 1) && (point.y >= position.y)) {
            Object obj;

            obj.type = TILE;
            obj.tile = tile;

            _objects->add(obj);
        }
    }
}

void buffer_editor_left_panel() {

    EDITOR_LEFT_PANEL_ROW_HEIGHT = EDITOR_LEFT_PANEL_PADDING * 2 * window_data.aspect_ratio + 14.0f / window_data.height; // @Temporary Current distance from baseline to top of capital letter is 12px

    // Background
    {
        Color4f color = { 0.1f, 0.1f, 0.2f, 0.8f };
        buffer_colored_quad(0.0f, 0.0f, BOTTOM_LEFT, EDITOR_LEFT_PANEL_WIDTH, 1.0f, EDITOR_LEFT_PANEL_BACKGROUND_Z, color);
    }

    if(editor_left_panel_mode == TILE_INFO) {
        if(editor_left_panel_displayed_object.tile != NULL) {

            float y = 1.0f;

            // Texture display
            {
                float texture_display_size = 0.7f * EDITOR_LEFT_PANEL_WIDTH;
                float texture_side_padding = 0.15f * EDITOR_LEFT_PANEL_WIDTH;
                float texture_top_padding = texture_side_padding / 2.0f;

                float aspect_ratio = window_data.aspect_ratio;

                char * texture = editor_left_panel_displayed_object.tile->texture;

                y -= texture_top_padding;

                buffer_textured_quad(texture_side_padding,  y, TOP_LEFT, texture_display_size, texture_display_size * aspect_ratio, EDITOR_LEFT_PANEL_CONTENT_Z, texture);

                y -= texture_display_size * aspect_ratio + texture_top_padding;
            }

            buffer_string("Texture :", EDITOR_LEFT_PANEL_PADDING, y, EDITOR_LEFT_PANEL_CONTENT_Z, my_font, TOP_LEFT);

            y -= EDITOR_LEFT_PANEL_ROW_HEIGHT;

            buffer_string(editor_left_panel_displayed_object.tile->texture,
                          EDITOR_LEFT_PANEL_WIDTH - EDITOR_LEFT_PANEL_PADDING, y,
                          EDITOR_LEFT_PANEL_CONTENT_Z, my_font, BOTTOM_RIGHT);

//            y -= EDITOR_LEFT_PANEL_ROW_HEIGHT;
        }
    }

}

void buffer_editor_click_menu() {
    EDITOR_CLICK_MENU_ROW_HEIGHT = EDITOR_MENU_PADDING * 2 * window_data.aspect_ratio + 14.0f / window_data.height; // @Temporary Current distance from baseline to top of capital letter is 12px

    float x = editor_click_menu_position.x;
    float y = editor_click_menu_position.y - EDITOR_CLICK_MENU_ROW_HEIGHT;

    for(int i = 0; i < objects.count; i++) {
        // Buffer menu background
        {
            Color4f color;
            if(i % 2 == 0) {
                color = { 0.2f, 0.2f, 0.3f, 0.8f };
            } else {
                color = { 0.3f, 0.3f, 0.4f, 0.7f };
            }

            buffer_colored_quad(x, y, BOTTOM_LEFT, EDITOR_CLICK_MENU_WIDTH, EDITOR_CLICK_MENU_ROW_HEIGHT, DEBUG_OVERLAY_BACKGROUND_Z, color);
        }

        // Buffer strings
        {
            if(objects.data[i].type == TILE) {
                // log_print("editor_mouse_collision", "Colliding with object of type TILE at (%d, %d)", objects[i].tile->local_x, objects[i].tile->local_y);
                char tile_name[64];

                Vector2 tile_position = objects.data[i].tile->position;

                snprintf(tile_name, 64, "Tile%d_%d", tile_position.x, tile_position.y);

                buffer_string(tile_name, x + EDITOR_MENU_PADDING, y + EDITOR_MENU_PADDING * window_data.aspect_ratio, DEBUG_OVERLAY_Z, my_font, BOTTOM_LEFT);
            }
        }

        y -= EDITOR_CLICK_MENU_ROW_HEIGHT;
    }
}

void buffer_editor_overlay() {

    buffer_editor_blocks_overlay(current_room);
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
    float DEBUG_OVERLAY_ROW_HEIGHT = DEBUG_OVERLAY_PADDING * 2 * window_data.aspect_ratio + 14.0f / window_data.height; // @Temporary Current distance from baseline to top of capital letter is 12px

    float DEBUG_OVERLAY_NUM_ROWS = 4; // @Harcoded

    // Buffer debug overlay background
    {
        for(int i = 0; i < DEBUG_OVERLAY_NUM_ROWS; i++) {
            Vector2f position;

            position.x = 1.0f - DEBUG_OVERLAY_WIDTH;
            position.y = 1.0f - i * DEBUG_OVERLAY_ROW_HEIGHT;

            Color4f color;

            if(i % 2 == 0) {
                color = { 0.2f, 0.2f, 0.3f, 0.8f };
            } else {
                color = { 0.3f, 0.3f, 0.4f, 0.7f };
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

    summed_frame_rate += window_data.current_dt;
    frame_time_print_counter--;

    float left_x  = 1.0f - DEBUG_OVERLAY_WIDTH + DEBUG_OVERLAY_PADDING;
    float right_x = 1.0f - DEBUG_OVERLAY_PADDING;
    float y       = 1.0f - DEBUG_OVERLAY_ROW_HEIGHT + DEBUG_OVERLAY_PADDING * window_data.aspect_ratio;
    // Buffer Frame time (text must always be buffered last if it has AA / transparency);
    {
        char buffer[64];

        snprintf(buffer, sizeof(buffer), "%.3f", displayed_frame_time * 1000);

        buffer_string("Frame Time (ms):", left_x, y, DEBUG_OVERLAY_Z, my_font, BOTTOM_LEFT);
        y -= DEBUG_OVERLAY_ROW_HEIGHT;

        buffer_string(buffer, right_x, y, DEBUG_OVERLAY_Z, my_font, BOTTOM_RIGHT);
        y -= DEBUG_OVERLAY_ROW_HEIGHT;
    }

    // Buffer Current world (text must always be buffered last if it has AA / transparency);
    {
        char buffer[64];

        snprintf(buffer, sizeof(buffer), "%s", current_room->name);

        buffer_string("Current World :", left_x, y, DEBUG_OVERLAY_Z, my_font, BOTTOM_LEFT);

        y -= DEBUG_OVERLAY_ROW_HEIGHT;

        buffer_string(buffer, right_x, y, DEBUG_OVERLAY_Z, my_font, BOTTOM_RIGHT);

        y -= DEBUG_OVERLAY_ROW_HEIGHT;
    }
}

void buffer_editor_blocks_overlay(Room * room) {
    for_array(room->collision_blocks.data, room->collision_blocks.count) {
        Quad quad = it->quad;

        // Don't buffer out of screen blocks
        if (quad.x0 > main_camera.offset.x + main_camera.size.x * 0.5f) continue;
        if (quad.x1 < main_camera.offset.x - main_camera.size.x * 0.5f) continue;
        if (quad.y0 > main_camera.offset.y + main_camera.size.y * 0.5f) continue;
        if (quad.y1 < main_camera.offset.y - main_camera.size.y * 0.5f) continue;

        Color4f color = { 0.0f, 1.0f, 1.0f, 0.5f };

        if(it->action_type == TELEPORT) {
            color = { 1.0f, 0.0f, 1.0f, 0.5f };
        }

        Vector2f tile_size;

        tile_size.x = 1.0f/main_camera.size.x;
        tile_size.y = 1.0f/main_camera.size.y;

        Vector2f offset;

        quad.x0 *=   tile_size.x;
        quad.x1 *=   tile_size.x;
        quad.y0 *= - tile_size.y;
        quad.y1 *= - tile_size.y;

        offset.x = 0.5f - tile_size.x * main_camera.offset.x;
        offset.y = 0.5f + tile_size.y * main_camera.offset.y;


        if(it->action_type == TELEPORT) {
            TeleportCollisionAction * teleport_action = (TeleportCollisionAction *) it->action;

            buffer_string(teleport_action->target_room_name, offset.x, offset.y, EDITOR_OVERLAY_Z , my_font);
        }


        quad.x0 += offset.x;
        quad.x1 += offset.x;
        quad.y0 += offset.y;
        quad.y1 += offset.y;

        swap(quad.y0, quad.y1);

        // printf("Drawing at %f, %f, %f, %f\n", quad.x0, quad.x1, quad.y0, quad.y1);

        buffer_colored_quad(quad, EDITOR_OVERLAY_Z, color);
    }
}

void buffer_entities() {
    for(int i = 0; i< array_size(entities); i++) {
        buffer_entity(entities[i]);
    }
}

void buffer_player() {
    buffer_entity(player);
}

// @Refactor with buffer_tiles
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
        screen_pos.y = 0.5f - tile_size.y * (entity.position.y + 1 - main_camera.offset.y);  // @Temporary see other one and also figure out why we needed a +1 here. :CoordsConversion

        Vector2f screen_size;

        screen_size.x = tile_size.x * entity.size;
        screen_size.y = tile_size.y * entity.size;

        float z = entity.position.y * tile_size.y * RANGE_ENTITY_Z * (main_camera.size.height / current_room->dimensions.height) + MIN_ENTITY_Z;

        buffer_textured_quad(screen_pos.x, screen_pos.y, BOTTOM_LEFT, screen_size.x, screen_size.y, z, entity.texture);
}

void buffer_tiles(Array<Tile> tiles) {
    for(int tile_index = 0; tile_index < tiles.count; tile_index++) {
        Tile * tile = &tiles.data[tile_index];
        int col = tile->position.x;
        int row = tile->position.y;

        // Don't buffer out of screen tiles
        if (col + 1 <= main_camera.offset.x - main_camera.size.x * 0.5f) continue;
        if (col     >= main_camera.offset.x + main_camera.size.x * 0.5f) continue;
        if (row + 1 <= main_camera.offset.y - main_camera.size.y * 0.5f) continue;
        if (row     >= main_camera.offset.y + main_camera.size.y * 0.5f) continue;

        Vector2f tile_size;

        tile_size.x = 1.0f/main_camera.size.x;
        tile_size.y = 1.0f/main_camera.size.y; // @Optimization, use multiplication here ?

        Vector2f tile_offset;

        tile_offset.x = tile_size.x * (col - main_camera.offset.x) + 0.5f;
        tile_offset.y = 0.5f - tile_size.y * (row + 1 - main_camera.offset.y); // @Temporary see other one and also figure out why we needed a +1 here. :CoordsConversion

        buffer_textured_quad(tile_offset.x, tile_offset.y, BOTTOM_LEFT, tile_size.x, tile_size.y, TILES_Z, tile->texture);
    }
}

float buffer_string(char * text, float x, float y, float z,  Font * font, Alignement alignement) { // Default : alignement = BOTTOM_LEFT
    float pixel_x = x * window_data.width;
    float pixel_y = y * window_data.height;

    float text_width = 0.0f;
    float text_height = 0.0f;

    // Compute height
    if(alignement == TOP_RIGHT || alignement == TOP_LEFT) {
        float zero_x = 0.0f;
        float zero_y = 0.0f;

        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(font->char_data, font->texture->width, font->texture->height, 'A' - 32, &zero_x, &zero_y, &q, 1); // 'A' is used as a reference character

        text_height = (float) ((int)(q.y0 + 0.5f)) / window_data.height;

        pixel_y += text_height * window_data.height;
    }

    // Compute width
    {
        float zero_x = 0.0f;
        float zero_y = 0.0f;

        char * cursor = text;

        while (*cursor) {
            // Make sure it's an ascii character // @Incomplete
            if (*cursor >= 32 && *cursor < 128) {
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(font->char_data, font->texture->width, font->texture->height, *cursor - ' ', &zero_x, &zero_y, &q, 1);
            }
            ++cursor;
        }

        text_width = (float) ((int)(zero_x + 0.5f)) / window_data.width;
    }

    set_texture(font->texture);
    set_shader(font_shader);

    start_buffer();

    while(*text) {
        // Make sure it's an ascii character
        if (*text >= 32 && *text < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font->char_data, font->texture->width, font->texture->height, *text-32, &pixel_x, &pixel_y, &q, 1);


            // top > bottom
            float height_above_bl = q.y1 - pixel_y;
            float height_below_bl = q.y0 - pixel_y;

            float inverted_y_top   = (int)((pixel_y - height_below_bl) + 0.5f);
            float inverted_y_below = (int)((pixel_y - height_above_bl) + 0.5f);

            float x0 = q.x0            /window_data.width;
            float y0 = inverted_y_below/window_data.height;
            float x1 = q.x1            /window_data.width;
            float y1 = inverted_y_top  /window_data.height;

            if((alignement == TOP_RIGHT) || (alignement == BOTTOM_RIGHT)) {
                x0 -= text_width;
                x1 -= text_width;
            }

            add_vertex(x0, y0, z, q.s0, q.t1);
            add_vertex(x0, y1, z, q.s0, q.t0);
            add_vertex(x1, y0, z, q.s1, q.t1);
            add_vertex(x1, y1, z, q.s1, q.t0);
        }
        ++text;
    }

    end_buffer();

    return text_width;
}


// Incomplete, handle uv coordinates and consider using texture pointer instead of name as an argument
void buffer_textured_quad(float x, float y, Alignement alignement, float width, float height, float depth, char * texture_name) {
    float x0 = x;
    float y0 = y;
    float x1 = x + width;
    float y1 = y + height;

    if(alignement == TOP_LEFT) {
        y0 -= height;
        y1 -= height;
    }

    Texture * texture = texture_manager.table.find(texture_name);

    if(!texture) {
      log_print("Drawing", "Could not find texture %s", texture_name);
    }

    set_shader(textured_shader);
    set_texture(texture);

    start_buffer();

    add_vertex(x0, y0, depth, 0.0f, 1.0f);
    add_vertex(x0, y1, depth, 0.0f, 0.0f);
    add_vertex(x1, y0, depth, 1.0f, 1.0f);
    add_vertex(x1, y1, depth, 1.0f, 0.0f);

    end_buffer();
}

void buffer_colored_quad(Quad quad, float depth, Color4f color) {
    set_shader(colored_shader);

    start_buffer();

    add_vertex(quad.x0, quad.y0, depth, color);
    add_vertex(quad.x0, quad.y1, depth, color);
    add_vertex(quad.x1, quad.y0, depth, color);
    add_vertex(quad.x1, quad.y1, depth, color);

    end_buffer();
}

void buffer_colored_quad(Vector2f position, Alignement alignement, float width, float height, float depth, Color4f color) {
    buffer_colored_quad(position.x, position.y, alignement, width, height, depth, color);
}

void buffer_colored_quad(float x, float y, Alignement alignement, float width, float height, float depth, Color4f color) {
    Quad quad;

    quad.x0 = x;
    quad.y0 = y;
    quad.x1 = x + width;
    quad.y1 = y + height;

    if(alignement == TOP_LEFT) {
        quad.y0 -= height;
        quad.y1 -= height;
    }

    buffer_colored_quad(quad, depth, color);
}

void update_time() {
    double now = os_specific_get_time();
    float current_dt = now - last_time;

    // @Incomplete, needs to sync up to GPU maybe. Or just use usual Vsync.
    if (window_data.locked_fps) {
        while (current_dt < 1.0f / TARGET_FPS) {
            os_specific_sleep(0);

            now = os_specific_get_time();
            current_dt = now - last_time;
        }
    }

    window_data.current_dt = current_dt;
    window_data.current_time = now;

    last_time = now;
}

void init_managers(){
    texture_manager.init();
    shader_manager.init();
    font_manager.init(&texture_manager);
    room_manager.init();

    register_manager(&texture_manager);
    register_manager(&font_manager);
    register_manager(&shader_manager);
    register_manager(&room_manager);

    managers.add(&texture_manager);
    managers.add(&shader_manager);
    managers.add(&font_manager);
    managers.add(&room_manager);
}

void main() {
    scope_exit(printf("Exiting."));

    os_specific_init_clock();

    // Init window
    {
        window_data.width  = 960;
        window_data.height = 720;

        // window_data.width  = 1920;
        // window_data.height = 1080;

        window_data.aspect_ratio = (float) window_data.width/window_data.height;
        char * window_name = "Game3";

        window_data.handle = os_specific_create_window(window_data.width, window_data.height, window_name);
    }

    init_renderer(window_data.width, window_data.height, window_data.handle); // Has to happen before we load the shaders

    init_managers();

    init_hotloader();

    hotloader_register_loose_files();

    for_array(managers.data, managers.count) {
        (*it)->perform_reloads();
    }

    init_shaders();

    init_game();

    my_font = font_manager.table.find("Inconsolata.ttf");

    log_print("perf_counter", "Startup time : %.3f seconds", os_specific_get_time());

    bool should_quit = false;
    while(!should_quit) {
        update_time();

        should_quit = os_specific_update_window_events(window_data.handle);

        previous_keyboard = keyboard;
        keyboard = os_specific_update_keyboard();

        game();

        draw_frame();

        check_hotloader_modifications();
        texture_manager.perform_reloads();
        // font_manager.perform_reloads(); // @Incomplete: make sure this works
        shader_manager.perform_reloads();
        room_manager.perform_reloads();
    }
}
